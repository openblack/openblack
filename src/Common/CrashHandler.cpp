/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CrashHandler.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <exception>
#include <filesystem>
#include <functional>
#include <string>
#include <thread>
#include <typeinfo>

#include <spdlog/spdlog.h>

#ifdef _WIN32
// clang-format off
// windows.h has to come before the headers that build on it, and without its min and max macros
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <DbgHelp.h>
#include <crtdbg.h>
// clang-format on
#else
#include <unistd.h>
#if __has_include(<execinfo.h>) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
#include <execinfo.h>
#define OPENBLACK_HAS_EXECINFO 1
#endif
#endif

namespace openblack::crash_handler
{
namespace
{
using crash_report::CrashInfo;
using crash_report::CrashKind;
using crash_report::StackFrame;

constexpr size_t k_MaxFrames = 64;
constexpr size_t k_ReportReserve = 64 * 1024;
#ifdef _WIN32
/// The system's code for an application ending itself after a fatal error, given to the minidump's exception record
/// when the crash wasn't a system exception
constexpr DWORD k_FatalAppExitCode = 0x40000015;
#endif

/// Everything the handlers share. Crash hooks are called by the operating system and the C runtime with no way to pass
/// them anything, so this is the one place the game keeps process-wide state; it is set up once at start-up, and the
/// buffers are reserved then so a crash in a broken heap needs as little allocation as possible.
struct HandlerState
{
	bool installed = false;
	bool copyToLog = false;
	std::filesystem::path reportDirectory;
	std::string report;
	std::string reportPath;
	std::string dumpPath;
	std::array<StackFrame, k_MaxFrames> frames {};
	std::array<std::array<char, 256>, k_MaxFrames> symbolNames {};
	std::array<std::array<char, 64>, k_MaxFrames> moduleNames {};
	std::array<std::array<char, 260>, k_MaxFrames> fileNames {};
	std::array<char, 1024> message {};
	std::array<char, 512> detail {};
	std::array<char, 512> file {};
};

HandlerState& State()
{
	static HandlerState state;
	return state;
}

/// Set by the first thread to crash; any other thread that crashes meanwhile waits for it to finish and exit
std::atomic<bool> s_Handling {false};
/// Set while this thread writes a report, so a crash inside the handler exits at once instead of recursing
thread_local bool t_InHandler = false;

bool DebuggerAttached()
{
#ifdef _WIN32
	return IsDebuggerPresent() != FALSE;
#else
	return false;
#endif
}

uint64_t CurrentThreadId()
{
#ifdef _WIN32
	return GetCurrentThreadId();
#else
	return std::hash<std::thread::id> {}(std::this_thread::get_id());
#endif
}

uint32_t CurrentProcessId()
{
#ifdef _WIN32
	return GetCurrentProcessId();
#else
	return static_cast<uint32_t>(getpid());
#endif
}

std::tm LocalTime()
{
	const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm result {};
#ifdef _WIN32
	localtime_s(&result, &now);
#else
	localtime_r(&now, &result);
#endif
	return result;
}

/// Copies text into a fixed buffer, cutting it short if it doesn't fit, and returns a view of the copy
template <size_t N>
std::string_view CopyInto(std::array<char, N>& buffer, std::string_view text)
{
	const auto length = std::min(text.size(), N - 1);
	std::copy_n(text.data(), length, buffer.data());
	buffer[length] = '\0';
	return {buffer.data(), length};
}

// Only Windows debug builds report the C runtime's messages, which end in line breaks
[[maybe_unused]] std::string_view TrimLineEnds(std::string_view text)
{
	while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
	{
		text.remove_suffix(1);
	}
	return text;
}

void WriteFile(const std::filesystem::path& path, std::string_view text)
{
#ifdef _WIN32
	std::FILE* file = _wfopen(path.c_str(), L"wb");
#else
	std::FILE* file = std::fopen(path.c_str(), "wb");
#endif
	if (file != nullptr)
	{
		std::fwrite(text.data(), 1, text.size(), file);
		std::fclose(file);
	}
}

std::filesystem::path ExecutableDirectory()
{
	std::error_code ec;
#ifdef _WIN32
	std::array<wchar_t, MAX_PATH> buffer {};
	const auto length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
	if (length > 0 && length < buffer.size())
	{
		return std::filesystem::path(std::wstring_view(buffer.data(), length)).parent_path();
	}
#else
	const auto self = std::filesystem::read_symlink("/proc/self/exe", ec);
	if (!ec)
	{
		return self.parent_path();
	}
#endif
	auto current = std::filesystem::current_path(ec);
	return ec ? std::filesystem::path(".") : current;
}

#ifdef _WIN32

std::string_view NarrowInto(std::span<char> buffer, const wchar_t* text)
{
	if (text == nullptr || buffer.empty())
	{
		return {};
	}
	const int length =
	    WideCharToMultiByte(CP_UTF8, 0, text, -1, buffer.data(), static_cast<int>(buffer.size()), nullptr, nullptr);
	if (length <= 0)
	{
		buffer[0] = '\0';
		return {};
	}
	return {buffer.data(), static_cast<size_t>(length - 1)};
}

/// The machine type and the starting frame of a stack walk from a thread's registers
DWORD PrepareStackWalk(const CONTEXT& context, STACKFRAME64& frame)
{
	frame = {};
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrStack.Mode = AddrModeFlat;
#if defined(_M_X64)
	frame.AddrPC.Offset = context.Rip;
	frame.AddrFrame.Offset = context.Rbp;
	frame.AddrStack.Offset = context.Rsp;
	return IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_ARM64)
	frame.AddrPC.Offset = context.Pc;
	frame.AddrFrame.Offset = context.Fp;
	frame.AddrStack.Offset = context.Sp;
	return IMAGE_FILE_MACHINE_ARM64;
#else
	frame.AddrPC.Offset = context.Eip;
	frame.AddrFrame.Offset = context.Ebp;
	frame.AddrStack.Offset = context.Esp;
	return IMAGE_FILE_MACHINE_I386;
#endif
}

uint64_t ProgramCounter(const CONTEXT& context)
{
#if defined(_M_X64)
	return context.Rip;
#elif defined(_M_ARM64)
	return context.Pc;
#else
	return context.Eip;
#endif
}

/// Walks the stack from the given registers and resolves each frame against the PDBs beside the executable
std::span<const StackFrame> CaptureStack(HANDLE thread, CONTEXT context)
{
	auto& state = State();
	const HANDLE process = GetCurrentProcess();
	SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_FAIL_CRITICAL_ERRORS);
	const auto searchPath = ExecutableDirectory().string();
	const bool symbols = SymInitialize(process, searchPath.c_str(), TRUE) != FALSE;

	STACKFRAME64 frame;
	const DWORD machine = PrepareStackWalk(context, frame);
	size_t count = 0;
	while (count < k_MaxFrames && StackWalk64(machine, process, thread, &frame, &context, nullptr, SymFunctionTableAccess64,
	                                          SymGetModuleBase64, nullptr) != FALSE)
	{
		if (frame.AddrPC.Offset == 0)
		{
			break;
		}
		auto& out = state.frames.at(count);
		out = {.address = frame.AddrPC.Offset};
		if (symbols)
		{
			IMAGEHLP_MODULE64 module {.SizeOfStruct = sizeof(IMAGEHLP_MODULE64)};
			if (SymGetModuleInfo64(process, frame.AddrPC.Offset, &module) != FALSE)
			{
				out.module = CopyInto(state.moduleNames.at(count), module.ModuleName);
			}

			std::array<std::byte, sizeof(SYMBOL_INFO) + 256> symbolStorage {};
			auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbolStorage.data());
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			symbol->MaxNameLen = 255;
			DWORD64 displacement = 0;
			if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol) != FALSE)
			{
				out.symbol = CopyInto(state.symbolNames.at(count), std::string_view(symbol->Name, symbol->NameLen));
				out.symbolOffset = displacement;
			}

			IMAGEHLP_LINE64 line {.SizeOfStruct = sizeof(IMAGEHLP_LINE64)};
			DWORD lineDisplacement = 0;
			if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &line) != FALSE)
			{
				out.file = CopyInto(state.fileNames.at(count), line.FileName);
				out.line = line.LineNumber;
			}
		}
		++count;
	}
	if (symbols)
	{
		SymCleanup(process);
	}
	return {state.frames.data(), count};
}

void WriteMinidump(const std::filesystem::path& path, EXCEPTION_POINTERS* exception, DWORD threadId)
{
	const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE)
	{
		return;
	}
	MINIDUMP_EXCEPTION_INFORMATION exceptionInfo {
	    .ThreadId = threadId,
	    .ExceptionPointers = exception,
	    .ClientPointers = FALSE,
	};
	// Every thread's stack and what it points at, with thread and module details: enough to inspect variables in a
	// debugger while staying a few megabytes rather than a copy of the whole heap
	const auto type = static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithThreadInfo |
	                                             MiniDumpWithUnloadedModules | MiniDumpWithHandleData |
	                                             MiniDumpWithProcessThreadData | MiniDumpWithDataSegs);
	const bool written = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, type,
	                                       exception != nullptr ? &exceptionInfo : nullptr, nullptr, nullptr) != FALSE;
	CloseHandle(file);
	if (!written)
	{
		DeleteFileW(path.c_str());
		std::fprintf(stderr, "Couldn't write the minidump (error %lu)\n", GetLastError());
	}
}

#else

std::span<const StackFrame> CaptureStack()
{
#ifdef OPENBLACK_HAS_EXECINFO
	auto& state = State();
	std::array<void*, k_MaxFrames> addresses {};
	const int count = backtrace(addresses.data(), static_cast<int>(addresses.size()));
	char** names = backtrace_symbols(addresses.data(), count);
	for (int i = 0; i < count; ++i)
	{
		auto& out = state.frames.at(i);
		out = {.address = reinterpret_cast<uint64_t>(addresses.at(i))};
		if (names != nullptr)
		{
			out.symbol = CopyInto(state.symbolNames.at(i), names[i]);
		}
	}
	std::free(names);
	return {state.frames.data(), static_cast<size_t>(std::max(count, 0))};
#else
	return {};
#endif
}

#endif

/// Writes the report everywhere and exits; never returns. context is the faulting thread's registers and
/// exceptionPointers the system exception, on Windows, when the crash came from one.
[[noreturn]] void HandleCrash(CrashInfo info, [[maybe_unused]] const void* context = nullptr,
                              [[maybe_unused]] void* exceptionPointers = nullptr)
{
	const int exitCode = crash_report::ExitCodeFor(info.kind, info.code,
#ifdef _WIN32
	                                               true
#else
	                                               false
#endif
	);

	if (t_InHandler)
	{
		// Crashed again while writing a report: there is nothing safe left to do but leave
		std::_Exit(exitCode);
	}
	t_InHandler = true;
	if (s_Handling.exchange(true))
	{
		// Another thread is already reporting its crash and will end the process
		std::this_thread::sleep_for(std::chrono::seconds(30));
		std::_Exit(exitCode);
	}

	if (DebuggerAttached())
	{
#ifdef _WIN32
		DebugBreak();
#endif
	}

	auto& state = State();
	if (info.threadId == 0)
	{
		info.threadId = CurrentThreadId();
	}
	info.time = LocalTime();

	std::error_code ec;
	std::filesystem::create_directories(state.reportDirectory, ec);
	const auto baseName = crash_report::ReportBaseName(info.time, CurrentProcessId());
	const auto reportPath = state.reportDirectory / (baseName + ".txt");
	state.reportPath = reportPath.string();

#ifdef _WIN32
	CONTEXT capturedContext {};
	const CONTEXT* threadContext = static_cast<const CONTEXT*>(context);
	auto* exception = static_cast<EXCEPTION_POINTERS*>(exceptionPointers);
	EXCEPTION_RECORD record {};
	EXCEPTION_POINTERS selfPointers {};
	if (threadContext == nullptr)
	{
		// No system exception: describe the crash from here, so the minidump's exception points at the handler's caller
		RtlCaptureContext(&capturedContext);
		threadContext = &capturedContext;
		record.ExceptionCode = k_FatalAppExitCode;
		record.ExceptionAddress = reinterpret_cast<PVOID>(ProgramCounter(capturedContext));
		selfPointers = {.ExceptionRecord = &record, .ContextRecord = &capturedContext};
		exception = &selfPointers;
	}
	auto frames = CaptureStack(GetCurrentThread(), *threadContext);
	if (threadContext == &capturedContext)
	{
		frames = crash_report::DropHandlerFrames(frames);
	}
	const auto dumpPath = state.reportDirectory / (baseName + ".dmp");
	state.dumpPath = dumpPath.string();
	info.dumpPath = state.dumpPath;
#else
	const auto frames = crash_report::DropHandlerFrames(CaptureStack());
#endif
	crash_report::AttributeAbort(info, frames);
	crash_report::FormatReport(info, frames, state.report);

	// The text report goes out before the minidump, which takes longer and does more that could fail
	std::fflush(stdout);
	std::fputs("\n", stderr);
	std::fwrite(state.report.data(), 1, state.report.size(), stderr);
	std::fprintf(stderr, "Crash report written to %s\n", state.reportPath.c_str());
	std::fflush(stderr);
	WriteFile(reportPath, state.report);

#ifdef _WIN32
	WriteMinidump(dumpPath, exception, static_cast<DWORD>(info.threadId));
#endif

	// The log comes last: its locks may be held by the very code that crashed
	if (state.copyToLog)
	{
		if (auto logger = spdlog::get("game"))
		{
			logger->critical("The game crashed; report written to {}\n{}", state.reportPath, state.report);
		}
	}
	spdlog::apply_all([](const std::shared_ptr<spdlog::logger>& logger) { logger->flush(); });

	std::_Exit(exitCode);
}

/// Fills in the exception's type and message, if std::terminate was called with one in flight
void DescribeCurrentException(CrashInfo& info)
{
	auto& state = State();
	const auto exception = std::current_exception();
	if (!exception)
	{
		return;
	}
	try
	{
		std::rethrow_exception(exception);
	}
	catch (const std::exception& e)
	{
		info.detail = CopyInto(state.detail, typeid(e).name());
		info.message = CopyInto(state.message, e.what());
	}
	catch (...)
	{
		info.detail = "unknown type (not derived from std::exception)";
	}
}

void OnTerminate()
{
	CrashInfo info {.kind = CrashKind::Terminate};
	DescribeCurrentException(info);
	HandleCrash(info);
}

#ifdef _WIN32

/// Runs the report on a fresh thread, for a stack overflow, which leaves the crashed thread too little stack to write it
struct OverflowReport
{
	CrashInfo info;
	EXCEPTION_POINTERS* exception;
};

DWORD WINAPI ReportOverflow(LPVOID parameter)
{
	const auto* report = static_cast<OverflowReport*>(parameter);
	HandleCrash(report->info, report->exception->ContextRecord, report->exception);
}

[[noreturn]] void ReportSystemException(EXCEPTION_POINTERS* exception)
{
	const auto* record = exception->ExceptionRecord;
	CrashInfo info {
	    .kind = CrashKind::StructuredException,
	    .code = static_cast<uint32_t>(record->ExceptionCode),
	    .faultAddress = reinterpret_cast<uint64_t>(record->ExceptionAddress),
	    .threadId = GetCurrentThreadId(),
	};
	if ((record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || record->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) &&
	    record->NumberParameters >= 2)
	{
		switch (record->ExceptionInformation[0])
		{
		case 0:
			info.accessKind = "reading";
			break;
		case 1:
			info.accessKind = "writing";
			break;
		case 8:
			info.accessKind = "executing";
			break;
		default:
			break;
		}
		info.accessAddress = record->ExceptionInformation[1];
	}

	if (record->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
	{
		OverflowReport report {.info = info, .exception = exception};
		const HANDLE thread = CreateThread(nullptr, 1024 * 1024, ReportOverflow, &report, 0, nullptr);
		if (thread != nullptr)
		{
			WaitForSingleObject(thread, INFINITE);
		}
		std::_Exit(crash_report::ExitCodeFor(info.kind, info.code, true));
	}
	HandleCrash(info, exception->ContextRecord, exception);
}

LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS* exception)
{
	if (DebuggerAttached())
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	ReportSystemException(exception);
}

void OnSignal(int signal)
{
	// The C runtime turns access violations, illegal instructions and floating point faults into signals when a handler
	// is set, and keeps the exception that caused them; reporting that exception keeps its faulting address and registers
	if (signal != SIGABRT && _pxcptinfoptrs != nullptr)
	{
		ReportSystemException(static_cast<EXCEPTION_POINTERS*>(_pxcptinfoptrs));
	}
	const auto name = crash_report::SignalName(signal);
	HandleCrash({
	    .kind = signal == SIGABRT ? CrashKind::Abort : CrashKind::Signal,
	    .detail = name,
	    .code = static_cast<uint32_t>(signal),
	});
}

#ifdef _DEBUG
int ReportCrtMessage(int type, std::string_view message, int* returnValue)
{
	if (type == _CRT_WARN)
	{
		// Warnings carry on as the runtime would handle them
		return FALSE;
	}
	if (DebuggerAttached())
	{
		// Break at the assertion itself, where the debugger can step past it
		*returnValue = 1;
		return TRUE;
	}
	HandleCrash({
	    .kind = type == _CRT_ASSERT ? CrashKind::Assertion : CrashKind::RuntimeError,
	    .message = CopyInto(State().message, TrimLineEnds(message)),
	});
}

int __cdecl OnCrtReport(int type, char* message, int* returnValue)
{
	return ReportCrtMessage(type, message != nullptr ? message : "", returnValue);
}

int __cdecl OnCrtReportWide(int type, wchar_t* message, int* returnValue)
{
	std::array<char, 1024> narrow {};
	return ReportCrtMessage(type, NarrowInto(narrow, message), returnValue);
}

#endif // _DEBUG

void __cdecl OnInvalidParameter(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line,
                                uintptr_t /*reserved*/)
{
	auto& state = State();
	// The runtime names the expression, function and file only in debug builds
	CrashInfo info {.kind = CrashKind::InvalidParameter, .line = line};
	info.message = NarrowInto(state.message, expression);
	info.detail = NarrowInto(state.detail, function);
	info.file = NarrowInto(state.file, file);
	HandleCrash(info);
}

void __cdecl OnPureCall()
{
	HandleCrash({.kind = CrashKind::PureVirtualCall});
}

#else

/// An alternate stack for the signal handler, so a stack overflow can still be reported
std::array<std::byte, 64 * 1024> s_SignalStack {};

void OnSignal(int signal, siginfo_t* signalInfo, void* /*context*/)
{
	CrashInfo info {
	    .kind = signal == SIGABRT ? CrashKind::Abort : CrashKind::Signal,
	    .detail = crash_report::SignalName(signal),
	    .code = static_cast<uint32_t>(signal),
	};
	if (signalInfo != nullptr && (signal == SIGSEGV || signal == SIGBUS || signal == SIGILL || signal == SIGFPE))
	{
		info.accessAddress = reinterpret_cast<uint64_t>(signalInfo->si_addr);
	}
	HandleCrash(info);
}

#endif

} // namespace

bool WantsCrashDialogs(std::span<char*> args)
{
	return std::ranges::any_of(args, [](const char* arg) { return arg != nullptr && arg == k_CrashDialogsOption; });
}

void Install()
{
	auto& state = State();
	if (state.installed)
	{
		return;
	}
	state.installed = true;
	state.reportDirectory = ExecutableDirectory() / "crashes";
	state.report.reserve(k_ReportReserve);

	std::set_terminate(OnTerminate);

#ifdef _WIN32
	// No "abort() has been called" message box and no Windows Error Reporting for abort; abort raises SIGABRT, which is
	// reported below
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	// Release-build assertions print to stderr instead of opening a message box
	_set_error_mode(_OUT_TO_STDERR);
#ifdef _DEBUG
	// Debug-build assertions and runtime errors come here instead of the Abort/Retry/Ignore dialog
	_CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, OnCrtReport);
	_CrtSetReportHookW2(_CRT_RPTHOOK_INSTALL, OnCrtReportWide);
#endif
	_set_invalid_parameter_handler(OnInvalidParameter);
	_set_purecall_handler(OnPureCall);
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
	SetUnhandledExceptionFilter(OnUnhandledException);
	// Keep enough stack in reserve on this thread for the handler to start after a stack overflow
	ULONG stackGuarantee = 64 * 1024;
	SetThreadStackGuarantee(&stackGuarantee);

	for (const int signal : {SIGABRT, SIGSEGV, SIGFPE, SIGILL})
	{
		std::signal(signal, OnSignal);
	}
#else
	stack_t stack {};
	stack.ss_sp = s_SignalStack.data();
	stack.ss_size = s_SignalStack.size();
	sigaltstack(&stack, nullptr);

	struct sigaction action = {};
	action.sa_sigaction = OnSignal;
	action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
	sigemptyset(&action.sa_mask);
	for (const int signal : {SIGABRT, SIGSEGV, SIGFPE, SIGILL, SIGBUS})
	{
		sigaction(signal, &action, nullptr);
	}
#ifdef OPENBLACK_HAS_EXECINFO
	// The first backtrace loads the unwinder; doing it now keeps that out of the crash
	std::array<void*, 1> warmUp {};
	backtrace(warmUp.data(), static_cast<int>(warmUp.size()));
#endif
#endif
}

bool IsInstalled()
{
	return State().installed;
}

void SetLogFile(std::string_view logFile)
{
	auto& state = State();
	if (logFile.empty() || logFile == "stdout" || logFile == "logcat")
	{
		state.copyToLog = false;
		return;
	}
	std::error_code ec;
	const auto absolute = std::filesystem::absolute(std::filesystem::path(logFile), ec);
	if (!ec)
	{
		state.reportDirectory = absolute.parent_path() / "crashes";
	}
	state.copyToLog = true;
}

void ReportFatal(CrashKind kind, std::string_view message, std::string_view file, uint32_t line, std::string_view detail)
{
	auto& state = State();
	HandleCrash({
	    .kind = kind,
	    .message = CopyInto(state.message, message),
	    .detail = CopyInto(state.detail, detail),
	    .file = CopyInto(state.file, file),
	    .line = line,
	});
}

} // namespace openblack::crash_handler
