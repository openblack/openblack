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

#include <filesystem>
#include <istream>
#include <optional>
#include <span>
#include <string>
#include <vector>

/// The text file that places sounds on the frames of the people's, animals' and birds' clips: a version, then for each
/// clip its name, its kind of sound and the sounds on it, until the word END
namespace openblack::sas
{

/// A sound on a clip's frame: when in the clip, in milliseconds from its start, which sound and how it plays
struct FrameSound
{
	int32_t time {0};
	int32_t action {0};
	int32_t mode {0};
};

/// The sounds placed on one clip, named as the clip is in the animation pack
struct ClipSounds
{
	std::string clip;
	/// The kind of thing the clip's sounds belong to (people, a kind of animal or bird); 0 when the file has no kinds
	int32_t soundType {0};
	std::vector<FrameSound> sounds;
};

struct SASFile
{
	int32_t version {0};
	std::vector<ClipSounds> clips;
};

/// Reads the text from a stream: each clip's lines of four numbers (time, sound, mode and whether another line
/// follows), reading stopping at END or at anything it can't read. None when the version can't be read.
[[nodiscard]] std::optional<SASFile> Parse(std::istream& stream);
/// Reads the text held in a buffer
[[nodiscard]] std::optional<SASFile> Parse(std::span<const uint8_t> buffer);
/// Reads a file; none when it can't be opened or read
[[nodiscard]] std::optional<SASFile> Open(const std::filesystem::path& path);

} // namespace openblack::sas
