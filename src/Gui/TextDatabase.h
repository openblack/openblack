/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

namespace openblack::gui
{

/// The game's text, looked up by the names its scripts give it (HELP_TEXT_DIALOG_CONTINUEGAME and so on).
///
/// Black & White keeps its text in the ADD_TEXT lines of UTF-16 scripts: InfoScript2.txt, InfoScriptPatch2.txt and
/// InfoScriptMultiplayer2.txt. The game numbers the texts in the order the lines come in (HelpTextDataBase and
/// MultiplayerTextDataBase); the names say the same and don't depend on the order.
// NOLINTNEXTLINE(bugprone-exception-escape): MSVC's unordered_map allocates when it is moved
class TextDatabase
{
public:
	/// Adds the ADD_TEXT lines of a script, little endian UTF-16 with or without a byte order mark. A text replaces any
	/// earlier one of the same name. Returns how many texts the script has.
	size_t AddScript(std::span<const uint8_t> script);

	/// The text of a name, empty when there is none. "\n" in the script is a line break.
	[[nodiscard]] std::u16string_view Get(std::string_view name) const;
	[[nodiscard]] size_t GetCount() const noexcept { return _texts.size(); }

private:
	std::unordered_map<std::string, std::u16string> _texts;
};

/// Converts text for logs and the like
std::string ToUtf8(std::u16string_view text);
/// Converts text to show in the game
std::u16string ToUtf16(std::string_view text);

} // namespace openblack::gui
