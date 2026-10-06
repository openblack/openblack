/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CrashReport.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <string>
#include <utility>

#include <spdlog/fmt/fmt.h>

namespace openblack::crash_report
{

std::string_view KindName(CrashKind kind)
{
	switch (kind)
	{
	case CrashKind::Assertion:
		return "Assertion failed";
	case CrashKind::RuntimeError:
		return "C runtime error";
	case CrashKind::InvalidParameter:
		return "Invalid parameter passed to a C runtime function";
	case CrashKind::PureVirtualCall:
		return "Pure virtual function call";
	case CrashKind::Abort:
		return "abort() called";
	case CrashKind::Terminate:
		return "std::terminate called";
	case CrashKind::UncaughtException:
		return "Uncaught exception";
	case CrashKind::Signal:
		return "Fatal signal";
	case CrashKind::StructuredException:
		return "Unhandled system exception";
	case CrashKind::GraphicsFatal:
		return "Fatal graphics error";
	}
	return "Unknown crash";
}

std::string_view StructuredExceptionName(uint32_t code)
{
	// The values of the Windows exception codes, written out so this stays free of windows.h
	constexpr std::array<std::pair<uint32_t, std::string_view>, 20> k_Names = {{
	    {0xC0000005, "EXCEPTION_ACCESS_VIOLATION"},         {0xC0000006, "EXCEPTION_IN_PAGE_ERROR"},
	    {0xC0000008, "EXCEPTION_INVALID_HANDLE"},           {0xC000001D, "EXCEPTION_ILLEGAL_INSTRUCTION"},
	    {0xC0000025, "EXCEPTION_NONCONTINUABLE_EXCEPTION"}, {0xC0000026, "EXCEPTION_INVALID_DISPOSITION"},
	    {0xC000008C, "EXCEPTION_ARRAY_BOUNDS_EXCEEDED"},    {0xC000008D, "EXCEPTION_FLT_DENORMAL_OPERAND"},
	    {0xC000008E, "EXCEPTION_FLT_DIVIDE_BY_ZERO"},       {0xC000008F, "EXCEPTION_FLT_INEXACT_RESULT"},
	    {0xC0000090, "EXCEPTION_FLT_INVALID_OPERATION"},    {0xC0000091, "EXCEPTION_FLT_OVERFLOW"},
	    {0xC0000092, "EXCEPTION_FLT_STACK_CHECK"},          {0xC0000093, "EXCEPTION_FLT_UNDERFLOW"},
	    {0xC0000094, "EXCEPTION_INT_DIVIDE_BY_ZERO"},       {0xC0000095, "EXCEPTION_INT_OVERFLOW"},
	    {0xC0000096, "EXCEPTION_PRIV_INSTRUCTION"},         {0xC00000FD, "EXCEPTION_STACK_OVERFLOW"},
	    {0xC0000409, "STATUS_STACK_BUFFER_OVERRUN"},        {0xE06D7363, "Microsoft C++ exception"},
	}};
	for (const auto& [value, name] : k_Names)
	{
		if (value == code)
		{
			return name;
		}
	}
	return {};
}

std::string_view SignalName(int signal)
{
	// The numbers that are the same on Windows, Linux and macOS
	switch (signal)
	{
	case 2:
		return "SIGINT";
	case 4:
		return "SIGILL";
	case 6:
		return "SIGABRT";
	case 8:
		return "SIGFPE";
	case 11:
		return "SIGSEGV";
	case 15:
		return "SIGTERM";
	case 22: // Windows' own number for it
		return "SIGABRT";
	default:
		return {};
	}
}

int ExitCodeFor(CrashKind kind, uint32_t code, bool windows)
{
	switch (kind)
	{
	case CrashKind::StructuredException:
		// Windows reports a crashed process's exit code as the exception code
		return code != 0 ? static_cast<int>(code) : k_AbortExitCode;
	case CrashKind::Signal:
		// Shells report a process killed by a signal as 128 plus the signal
		return windows || code == 0 ? k_AbortExitCode : 128 + static_cast<int>(code);
	case CrashKind::UncaughtException:
		return 1;
	default:
		return k_AbortExitCode;
	}
}

std::string ReportBaseName(const std::tm& time, uint32_t processId)
{
	return fmt::format("openblack-crash-{:04}{:02}{:02}-{:02}{:02}{:02}-{}", time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
	                   time.tm_hour, time.tm_min, time.tm_sec, processId);
}

std::span<const StackFrame> DropHandlerFrames(std::span<const StackFrame> frames)
{
	const auto first = std::ranges::find_if_not(
	    frames, [](const StackFrame& frame) { return frame.symbol.find("crash_handler") != std::string_view::npos; });
	const auto dropped = static_cast<size_t>(std::distance(frames.begin(), first));
	// A stack made only of the handler's frames is better shown whole than not at all
	return dropped == frames.size() ? frames : frames.subspan(dropped);
}

void AttributeAbort(CrashInfo& info, std::span<const StackFrame> frames)
{
	if (info.kind != CrashKind::Abort)
	{
		return;
	}
	// The C runtimes' functions behind assert(): Microsoft's, glibc's and Apple's
	constexpr std::array<std::string_view, 5> k_AssertFunctions = {"wassert", "_wassert", "_assert", "__assert_fail",
	                                                               "__assert_rtn"};
	const auto isAssertFunction = [&](const StackFrame& frame) {
		return std::ranges::any_of(k_AssertFunctions, [&](std::string_view name) {
			// Resolved names are bare; Unix backtraces give whole lines such as "libc.so.6(__assert_fail+0x40) [0x...]"
			return frame.symbol == name || frame.symbol.find(fmt::format("({}+", name)) != std::string_view::npos;
		});
	};
	const auto assertFrame = std::ranges::find_if(frames, isAssertFunction);
	if (assertFrame == frames.end())
	{
		const auto isTerminate = [](const StackFrame& frame) {
			return frame.symbol == "terminate" || frame.symbol == "std::terminate" ||
			       frame.symbol.find("(_ZSt9terminatev+") != std::string_view::npos;
		};
		if (std::ranges::any_of(frames, isTerminate))
		{
			info.kind = CrashKind::Terminate;
			info.detail = {};
			info.message = "usually an exception nothing caught; the frames below the runtime's show where it was thrown";
		}
		return;
	}
	info.kind = CrashKind::Assertion;
	info.detail = {};
	info.message = "assert() failed; its expression is printed on stderr just before this report";
	const auto caller = std::ranges::find_if(assertFrame, frames.end(), [](const StackFrame& frame) {
		return !frame.file.empty() && frame.symbol.find("assert") == std::string_view::npos;
	});
	if (caller != frames.end())
	{
		info.file = caller->file;
		info.line = caller->line;
	}
}

void FormatReport(const CrashInfo& info, std::span<const StackFrame> frames, std::string& out)
{
	out.clear();
	auto it = std::back_inserter(out);
	const auto& t = info.time;
	fmt::format_to(it, "==================== openblack crash report ====================\n");
	fmt::format_to(it, "Time:     {:04}-{:02}-{:02} {:02}:{:02}:{:02}\n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
	               t.tm_min, t.tm_sec);
	fmt::format_to(it, "Crash:    {}\n", KindName(info.kind));
	if (!info.detail.empty())
	{
		const auto* label = info.kind == CrashKind::Signal ? "Signal:   " : "Type:     ";
		fmt::format_to(it, "{}{}\n", label, info.detail);
	}
	if (!info.message.empty())
	{
		fmt::format_to(it, "Message:  {}\n", info.message);
	}
	if (!info.file.empty())
	{
		fmt::format_to(it, "Location: {}:{}\n", info.file, info.line);
	}
	if (info.kind == CrashKind::StructuredException)
	{
		const auto name = StructuredExceptionName(info.code);
		fmt::format_to(it, "Code:     0x{:08X}{}{}\n", info.code, name.empty() ? "" : " ", name);
	}
	if (info.faultAddress)
	{
		fmt::format_to(it, "Address:  0x{:016X}\n", *info.faultAddress);
	}
	if (info.accessAddress)
	{
		fmt::format_to(it, "Access:   {} 0x{:016X}\n", info.accessKind.empty() ? "touching" : info.accessKind,
		               *info.accessAddress);
	}
	fmt::format_to(it, "Thread:   {}\n", info.threadId);
	if (!info.dumpPath.empty())
	{
		fmt::format_to(it, "Minidump: {}\n", info.dumpPath);
	}

	fmt::format_to(it, "Stack trace:\n");
	if (frames.empty())
	{
		fmt::format_to(it, "  (not available)\n");
	}
	for (size_t i = 0; i < frames.size(); ++i)
	{
		const auto& frame = frames[i];
		fmt::format_to(it, "  #{:<2} 0x{:016X}", i, frame.address);
		if (!frame.module.empty())
		{
			fmt::format_to(it, " {}", frame.module);
		}
		if (!frame.symbol.empty())
		{
			fmt::format_to(it, "{}{}", frame.module.empty() ? " " : "!", frame.symbol);
			if (frame.symbolOffset != 0)
			{
				fmt::format_to(it, "+0x{:X}", frame.symbolOffset);
			}
		}
		if (!frame.file.empty())
		{
			fmt::format_to(it, " ({}:{})", frame.file, frame.line);
		}
		out.push_back('\n');
	}
	fmt::format_to(it, "================================================================\n");
}

} // namespace openblack::crash_report
