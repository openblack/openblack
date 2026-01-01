/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "InfoFile.h"

#include <PackFile.h>
#include <spdlog/spdlog.h>

#include "FileSystem/FileSystemInterface.h"
#include "Locator.h"

namespace openblack
{

std::unique_ptr<const InfoConstants> InfoFile::LoadFromFile(const std::filesystem::path& path) noexcept
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading Info Pack from file: {}", path.generic_string());

	auto infos = std::make_unique<InfoConstants>();
	std::vector<uint8_t> data;
	pack::PackFile pack;
	const auto result = pack.ReadFile(*Locator::filesystem::value().GetData(path));
	if (result != pack::PackResult::Success)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open {}: {}", path.generic_string(), pack::ResultToStr(result));
		return nullptr;
	}

	data = pack.GetBlock("Info");
	if (data.size() == sizeof(v100::InfoConstants))
	{
		auto oldInfos = std::make_unique<v100::InfoConstants>();
		std::memcpy(oldInfos.get(), data.data(), sizeof(v100::InfoConstants));
		// Only 4 bytes in CreatureActionInfo needs to be filled
		UpdateInfo(*infos, *oldInfos);
	}
	else if (data.size() == sizeof(v120::InfoConstants))
	{
		std::memcpy(infos.get(), data.data(), sizeof(*infos));
	}
	else
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("game"),
		                       "Info block size does not match that of GInfo of version 1.0 or 1.2: {} != {} or {}",
		                       data.size(), sizeof(v100::InfoConstants), sizeof(v120::InfoConstants));
		return nullptr;
	}

	return infos;
}

} // namespace openblack
