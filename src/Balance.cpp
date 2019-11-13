/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Balance.h"

#include "Common/FileSystem.h"

#include <spdlog/spdlog.h>

#include <string>

namespace openblack
{

void load_variables(FileSystem& filesystem)
{
	auto file = filesystem.Open("Scripts/info.dat", FileMode::Read);

	// check magic header
	constexpr char kLionheadMagic[] = "LiOnHeAd";

	struct
	{
		char blockName[32];
		uint32_t blockSize;
		uint32_t position;
	} header;

	char magic[8];
	file->Read(magic, 8);
	if (!std::equal(kLionheadMagic, kLionheadMagic + 8, magic))
		throw std::runtime_error("invalid Lionhead file magic");

	char blockName[32];
	file->Read(blockName, 32);
	uint32_t size = file->ReadValue<uint32_t>();

	spdlog::debug("info.dat: block name = {} = {} bytes", blockName, size);

	bool bReadBinary = true; // true if no source files exist (Info1.txt, etc..)
	                         // if they do exist, we _should_ parse and write out to info.tmp

	LoadMagic<GMagicInfo>(*file, "DETAIL_MAGIC_GENERAL_INFO", 10, bReadBinary);
	LoadMagic<GMagicHealInfo>(*file, "DETAIL_MAGIC_HEAL_INFO", 2, bReadBinary);
	LoadMagic<GMagicTeleportInfo>(*file, "DETAIL_MAGIC_TELEPORT_INFO", 1, bReadBinary);
	// LoadMagic<GMagicForestInfo>(*file, "DETAIL_MAGIC_FOREST_INFO", 1, bReadBinary);
	// LoadMagic<GMagicResourceInfo>(*file, "DETAIL_MAGIC_FOOD_INFO", 2, bReadBinary);
	// LoadMagic<GMagicStormAndTornadoInfo>(*file, "DETAIL_MAGIC_STORM_AND_TORNADO_INFO", 3, bReadBinary);
	// LoadMagic<GMagicShieldInfo>(*file, "DETAIL_MAGIC_SHIELD_ONE_INFO", 2, bReadBinary);
	// LoadMagic<GMagicResourceInfo>(*file, "DETAIL_MAGIC_WOOD_INFO", 1, bReadBinary);
	// LoadMagic<GMagicWaterInfo>(*file, "DETAIL_MAGIC_WATER_INFO", 2, bReadBinary);
	// LoadMagic<GMagicFlockFlyingInfo>(*file, "DETAIL_MAGIC_FLOCK_FLYING_INFO", 1, bReadBinary);
	// LoadMagic<GMagicFlockGroundInfo>(*file, "DETAIL_MAGIC_FLOCK_GROUND_INFO", 1, bReadBinary);
	// LoadMagic<GMagicCreatureSpellInfo>(*file, "DETAIL_MAGIC_CREATURE_SPELL_INFO", 16, bReadBinary);

	// GMagicEffectInfo::GetInfo(void) // returns a global (makes it, kinda like singleton)
	// LoadIt<GMagicEffectInfo>(LoaderAnon*, char*, GMagicEffectInfo*, unsigned long, bool, LHFile*)

	// GSpellSeedInfo::GetInfo(void)
	// LoadIt<GSpellSeedInfo>(LoaderAnon*, char*, GSpellSeedInfo*, unsigned long, bool, LHFile*)
}

// must extend GBaseInfo
template <class T>
void LoadMagic(IStream& stream, const std::string& name, std::size_t count, bool saveload)
{
	static_assert(std::is_base_of<GMagicInfo, T>::value, "T not derived from GMagicInfo");

	for (std::size_t i = 0; i < count; i++)
	{
		GMagicInfo* magicInfo = new T(); // Derived from Base -> GBaseInfo -> GMagicInfo
		magicInfo->LoadBinary(stream);

		delete magicInfo;
	}
}

void GBaseInfo::setInfoID() {}

void GMagicInfo::LoadBinary(IStream& stream)
{
	stream.Read(&this->info, 0x48);
	GBaseInfo::setInfoID();
}

void GMagicHealInfo::LoadBinary(IStream& stream)
{
	GMagicInfo::LoadBinary(stream);
	stream.Read(&this->heal_info, sizeof(this->heal_info));
}

void GMagicTeleportInfo::LoadBinary(IStream& stream)
{
	GMagicInfo::LoadBinary(stream);
	stream.Read(&this->teleport_info, sizeof(this->teleport_info));
}

// LoadMagic<GMagicInfo>(GMagicInfo*, LoaderAnon*, char*, unsigned long, GMagicInfo*, unsigned long, bool, LHFile*)
/*
    for each {
        new Base 0x58

        GMagicInfo::get_size(void) -> 0x48
        new ^ (buffer)
        LHFile::GetSegmentData(void*, unsigned long, long)

        dest = GMagicInfo::get_start(ptr) -> base + 0x10
        memcpy(buffer -> dest, size)
        ~buffer()

        GBaseInfo::SetInfoID(base)
    }
*/

// LoadMagic<GMagicHealInfo>(GMagicHealInfo*, LoaderAnon*, char*, unsigned long, GMagicHealInfo*, unsigned long, bool, LHFile*)
/*
    for each {
        new Base 0x60

        GMagicInfo::LoadBinary(^, LHFile*)

        GMagicInfo::get_size(void) -> 8
        new ^ (buffer)
        LHFile::GetSegmentData(void*, unsigned long, long)

        dest = GMagicInfo::get_start(ptr) -> base + 0x58
        memcpy(buffer -> dest, size)
        ~buffer()

        GBaseInfo::SetInfoID(base)
    }
*/

} // namespace openblack
