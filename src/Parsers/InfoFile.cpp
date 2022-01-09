/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "InfoFile.h"

#include <spdlog/spdlog.h>

#include <PackFile.h>

#include "Common/FileSystem.h"
#include "Game.h"

namespace openblack
{

bool InfoFile::LoadFromFile(const fs::path& path, InfoConstants& infos)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading Info Pack from file: {}", path.generic_string());

	std::vector<uint8_t> data;
	try
	{
		pack::PackFile pack;
		pack.Open(Game::instance()->GetFileSystem().FindPath(path).u8string());
		data = pack.GetBlock("Info");
		if (data.size() != sizeof(InfoConstants))
		{
			// TODO(bwrsandman): Rename current InfoConstants to InfoConstantsV12 and add support to different versions
			throw std::runtime_error(fmt::format("Info block size does not match that of GInfo of version 1.2: {} != {}",
			                                     data.size(), sizeof(InfoConstants)));
		}
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open {}: {}", path.generic_string(), err.what());
		return false;
	}

	std::memcpy(&infos, data.data(), sizeof(infos));

	return true;
}

} // namespace openblack
