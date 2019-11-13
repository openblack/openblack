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

#pragma once

#include "Enums.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace openblack
{

class IStream;
class FileSystem;

void load_variables(FileSystem& filesystem);

template <class T>
void LoadMagic(IStream& stream, const std::string& name, std::size_t count, bool saveload);

template <class T>
void LoadIt(IStream& stream, const std::string& name, std::size_t count, bool saveload);

class GBaseInfo
{
protected:
	void setInfoID();
};

class GMagicInfo: public GBaseInfo
{
public:
	virtual void LoadBinary(IStream&);

private:
	struct
	{
		MagicType magicType;                   // ENUM_MAGIC_TYPE
		ImmersionEffectType immersionTypeEnum; // ENUM_IMMERSION_EFFECT_TYPE
		uint32_t stopImmersion;
		float perceivedPower;
		ParticleType particleTypeEnum;     // ENUM_PARTICLE_TYPE
		ImpressiveType impressiveTypeEnum; // ENUM_IMPRESSIVE_TYPE
		SpellSeedType spellSeedTypeEnum;   // ENUM_SPELL_SEED_TYPE
		GestureType gestureType;           // ENUM_GESTURE_TYPE
		PowerUpType powerupType;           // ENUM_POWER_UP_TYPE
		CastRuleType castRuleType;         // ENUM_CAST_RULE_TYPE
		uint32_t isSpellSeedDrawnInHand;
		uint32_t isSpellRecharged;
		uint32_t isCreatureCastFromAbove;
		uint32_t oneOffSpellIsPlayful;
		uint32_t oneOffSpellIsAggressive;
		uint32_t oneOffSpellIsCompassionate;
		uint32_t oneOffSpellIsToRestoreHealth;
		ParticleType particleType; // ENUM_PARTICLE_TYPE
	} info;
	static_assert(sizeof(info) == 0x48, "size is incorrect");
};

class GMagicHealInfo: public GMagicInfo
{
public:
	virtual void LoadBinary(IStream&);

private:
	struct
	{
		uint32_t dummyVar;
		uint32_t maxToHeal;
	} heal_info;
	static_assert(sizeof(heal_info) == 0x08, "size is incorrect");
};

class GMagicTeleportInfo: public GMagicInfo
{
public:
	virtual void LoadBinary(IStream&);

private:
	struct
	{
		float costPerKilometer;
	} teleport_info;
	static_assert(sizeof(teleport_info) == 0x04, "size is incorrect");
};

} // namespace openblack
