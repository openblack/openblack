/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <string>

#include <Common/CrashHandler.h>
#include <Common/CrashReport.h>
#include <gtest/gtest.h>

using openblack::crash_report::CrashInfo;
using openblack::crash_report::CrashKind;
using openblack::crash_report::StackFrame;
namespace crash_report = openblack::crash_report;

namespace
{
std::tm SampleTime()
{
	std::tm time {};
	time.tm_year = 2026 - 1900;
	time.tm_mon = 9; // October
	time.tm_mday = 6;
	time.tm_hour = 14;
	time.tm_min = 2;
	time.tm_sec = 3;
	return time;
}
} // namespace

TEST(CrashReport, NamesFilesAfterTheTime)
{
	EXPECT_EQ(crash_report::ReportBaseName(SampleTime(), 4120), "openblack-crash-20261006-140203-4120");
}

TEST(CrashReport, AbortLikeCrashesExitWithThree)
{
	for (const auto kind : {CrashKind::Assertion, CrashKind::RuntimeError, CrashKind::InvalidParameter,
	                        CrashKind::PureVirtualCall, CrashKind::Abort, CrashKind::Terminate, CrashKind::GraphicsFatal})
	{
		EXPECT_EQ(crash_report::ExitCodeFor(kind, 0, true), crash_report::k_AbortExitCode);
		EXPECT_EQ(crash_report::ExitCodeFor(kind, 6, false), crash_report::k_AbortExitCode);
	}
}

TEST(CrashReport, SystemExceptionsExitWithTheirCode)
{
	EXPECT_EQ(static_cast<uint32_t>(crash_report::ExitCodeFor(CrashKind::StructuredException, 0xC0000005, true)), 0xC0000005);
	EXPECT_EQ(crash_report::ExitCodeFor(CrashKind::StructuredException, 0, true), crash_report::k_AbortExitCode);
}

TEST(CrashReport, SignalsExitAsShellsReportThem)
{
	EXPECT_EQ(crash_report::ExitCodeFor(CrashKind::Signal, 11, false), 139);
	EXPECT_EQ(crash_report::ExitCodeFor(CrashKind::Signal, 11, true), crash_report::k_AbortExitCode);
}

TEST(CrashReport, ExceptionsCaughtInMainKeepTheOldExitCode)
{
	EXPECT_EQ(crash_report::ExitCodeFor(CrashKind::UncaughtException, 0, true), 1);
}

TEST(CrashReport, NamesExceptionsAndSignals)
{
	EXPECT_EQ(crash_report::StructuredExceptionName(0xC0000005), "EXCEPTION_ACCESS_VIOLATION");
	EXPECT_EQ(crash_report::StructuredExceptionName(0xC00000FD), "EXCEPTION_STACK_OVERFLOW");
	EXPECT_TRUE(crash_report::StructuredExceptionName(0x12345678).empty());
	EXPECT_EQ(crash_report::SignalName(11), "SIGSEGV");
	EXPECT_EQ(crash_report::SignalName(6), "SIGABRT");
	EXPECT_TRUE(crash_report::SignalName(1000).empty());
}

TEST(CrashReport, FormatsAnAccessViolation)
{
	const CrashInfo info {
	    .kind = CrashKind::StructuredException,
	    .code = 0xC0000005,
	    .faultAddress = 0x1234,
	    .accessAddress = 0,
	    .accessKind = "writing",
	    .threadId = 42,
	    .time = SampleTime(),
	    .dumpPath = "crashes/openblack-crash-20261006-140203.dmp",
	};
	const std::array<StackFrame, 2> frames {{
	    {.address = 0x1234,
	     .module = "openblack",
	     .symbol = "openblack::Game::Run",
	     .symbolOffset = 0x10,
	     .file = "Game.cpp",
	     .line = 99},
	    {.address = 0x5678},
	}};
	std::string report;
	crash_report::FormatReport(info, frames, report);

	EXPECT_NE(report.find("Time:     2026-10-06 14:02:03\n"), std::string::npos) << report;
	EXPECT_NE(report.find("Crash:    Unhandled system exception\n"), std::string::npos) << report;
	EXPECT_NE(report.find("Code:     0xC0000005 EXCEPTION_ACCESS_VIOLATION\n"), std::string::npos) << report;
	EXPECT_NE(report.find("Address:  0x0000000000001234\n"), std::string::npos) << report;
	EXPECT_NE(report.find("Access:   writing 0x0000000000000000\n"), std::string::npos) << report;
	EXPECT_NE(report.find("Thread:   42\n"), std::string::npos) << report;
	EXPECT_NE(report.find("Minidump: crashes/openblack-crash-20261006-140203.dmp\n"), std::string::npos) << report;
	EXPECT_NE(report.find("  #0  0x0000000000001234 openblack!openblack::Game::Run+0x10 (Game.cpp:99)\n"), std::string::npos)
	    << report;
	EXPECT_NE(report.find("  #1  0x0000000000005678\n"), std::string::npos) << report;
}

TEST(CrashReport, FormatsAnAssertionWithoutAStack)
{
	const CrashInfo info {
	    .kind = CrashKind::Assertion,
	    .message = "Assertion failed: x > 0",
	    .file = "Thing.cpp",
	    .line = 12,
	    .time = SampleTime(),
	};
	std::string report = "left over";
	crash_report::FormatReport(info, {}, report);

	EXPECT_EQ(report.find("left over"), std::string::npos);
	EXPECT_NE(report.find("Crash:    Assertion failed\n"), std::string::npos) << report;
	EXPECT_NE(report.find("Message:  Assertion failed: x > 0\n"), std::string::npos) << report;
	EXPECT_NE(report.find("Location: Thing.cpp:12\n"), std::string::npos) << report;
	EXPECT_NE(report.find("  (not available)\n"), std::string::npos) << report;
	EXPECT_EQ(report.find("Code:"), std::string::npos) << report;
	EXPECT_EQ(report.find("Minidump:"), std::string::npos) << report;
}

TEST(CrashReport, DropsTheHandlersOwnFrames)
{
	const std::array<StackFrame, 3> frames {{
	    {.address = 1, .symbol = "openblack::crash_handler::`anonymous namespace'::HandleCrash"},
	    {.address = 2, .symbol = "raise"},
	    {.address = 3, .symbol = "openblack::crash_handler::Install"},
	}};
	const auto kept = crash_report::DropHandlerFrames(frames);
	ASSERT_EQ(kept.size(), 2);
	EXPECT_EQ(kept[0].address, 2);
	EXPECT_EQ(crash_report::DropHandlerFrames(std::span(frames).first(1)).size(), 1);
}

TEST(CrashReport, AttributesAnAbortFromAssert)
{
	const std::array<StackFrame, 4> frames {{
	    {.address = 1, .symbol = "abort"},
	    {.address = 2, .symbol = "wassert"},
	    {.address = 3, .symbol = "openblack::Thing::Update", .file = "Thing.cpp", .line = 12},
	    {.address = 4, .symbol = "main", .file = "main.cpp", .line = 3},
	}};
	CrashInfo info {.kind = CrashKind::Abort, .detail = "SIGABRT"};
	crash_report::AttributeAbort(info, frames);
	EXPECT_EQ(info.kind, CrashKind::Assertion);
	EXPECT_EQ(info.file, "Thing.cpp");
	EXPECT_EQ(info.line, 12);
	EXPECT_TRUE(info.detail.empty());

	const std::array<StackFrame, 2> unixFrames {{
	    {.address = 1, .symbol = "/lib/libc.so.6(__assert_fail+0x40) [0x7f00]"},
	    {.address = 2, .symbol = "./openblack(main+0x10) [0x400]"},
	}};
	CrashInfo unixInfo {.kind = CrashKind::Abort};
	crash_report::AttributeAbort(unixInfo, unixFrames);
	EXPECT_EQ(unixInfo.kind, CrashKind::Assertion);
	EXPECT_TRUE(unixInfo.file.empty());

	CrashInfo plainAbort {.kind = CrashKind::Abort};
	crash_report::AttributeAbort(plainAbort, std::span(frames).first(1));
	EXPECT_EQ(plainAbort.kind, CrashKind::Abort);
}

TEST(CrashReport, AttributesAnAbortFromTerminate)
{
	const std::array<StackFrame, 3> frames {{
	    {.address = 1, .symbol = "abort"},
	    {.address = 2, .symbol = "terminate"},
	    {.address = 3, .symbol = "CxxThrowException"},
	}};
	CrashInfo info {.kind = CrashKind::Abort, .detail = "SIGABRT"};
	crash_report::AttributeAbort(info, frames);
	EXPECT_EQ(info.kind, CrashKind::Terminate);

	CrashInfo segfault {.kind = CrashKind::Signal};
	crash_report::AttributeAbort(segfault, frames);
	EXPECT_EQ(segfault.kind, CrashKind::Signal);
}

TEST(CrashReport, FindsTheCrashDialogsSwitch)
{
	std::array<char*, 3> withSwitch {const_cast<char*>("openblack"), const_cast<char*>("-g"),
	                                 const_cast<char*>("--crash-dialogs")};
	std::array<char*, 2> without {const_cast<char*>("openblack"), const_cast<char*>("--crash")};
	EXPECT_TRUE(openblack::crash_handler::WantsCrashDialogs(withSwitch));
	EXPECT_FALSE(openblack::crash_handler::WantsCrashDialogs(without));
}
