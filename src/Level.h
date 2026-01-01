/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <filesystem>
#include <string>

namespace openblack
{
class Level
{
public:
	enum class LandType : bool
	{
		Skirmish,
		Campaign,
	};

	Level(std::string name, std::filesystem::path path, std::string description, LandType landType, bool isValid = false);

	[[nodiscard]] const std::string& GetName() const;
	[[nodiscard]] const std::filesystem::path& GetScriptPath() const;
	[[nodiscard]] LandType GetType() const;
	[[nodiscard]] const std::string& GetDescription() const;
	[[nodiscard]] bool IsValid() const;

	static bool IsLevelFile(const std::filesystem::path& path);
	static Level ParseLevel(const std::filesystem::path& path, Level::LandType landType);

private:
	std::string _name;
	std::filesystem::path _scriptPath;
	std::string _description;
	LandType _landType;
	/// @brief The loaded level script have a landscape that can be loaded
	bool _isValid;
};

} // namespace openblack
