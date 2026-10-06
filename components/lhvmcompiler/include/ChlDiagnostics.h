/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <string>
#include <string_view>
#include <vector>

namespace openblack::lhvm::chl
{

/// A place in a source file. Lines and columns start at 1.
struct SourceLocation
{
	uint32_t line {0};
	uint32_t column {0};
};

enum class CompileSeverity : uint8_t
{
	Note,
	Warning,
	Error
};

struct CompileDiagnostic
{
	CompileSeverity severity {CompileSeverity::Error};
	/// Name of the source file, as given to the compiler
	std::string file;
	SourceLocation location;
	std::string message;

	/// "file:line:column: error: message"
	[[nodiscard]] std::string ToString() const;
};

/// Collects the diagnostics of one compilation
class DiagnosticSink
{
public:
	void Report(CompileSeverity severity, std::string_view file, SourceLocation location, std::string message);
	void Error(std::string_view file, SourceLocation location, std::string message)
	{
		Report(CompileSeverity::Error, file, location, std::move(message));
	}
	void Warning(std::string_view file, SourceLocation location, std::string message)
	{
		Report(CompileSeverity::Warning, file, location, std::move(message));
	}

	[[nodiscard]] const std::vector<CompileDiagnostic>& Diagnostics() const { return _diagnostics; }
	[[nodiscard]] std::vector<CompileDiagnostic> Take() { return std::move(_diagnostics); }
	[[nodiscard]] size_t ErrorCount() const { return _errorCount; }

private:
	std::vector<CompileDiagnostic> _diagnostics;
	size_t _errorCount {0};
};

} // namespace openblack::lhvm::chl
