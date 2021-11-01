/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Enums.h"
#include <array>
#include <cstdint>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack
{
#pragma pack(push, 1)
struct GTribeInfo
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	float field_0x8;
};

struct GObjectInfo
{
	int type;
	int alignmentType;
	std::array<char, 0x20> description;
	float field_0x28;
	float field_0x2c;
	float field_0x30;
	float field_0x34;
	uint32_t collideSoundType;
	uint32_t immersionTexture;
	uint32_t queryFirstEnumText;
	uint32_t queryLastEnumText;
	uint32_t fovHelpMessageSet;
	uint32_t fovHelpCondition;
	uint32_t handHelpMessageSet;
	uint32_t handHelpCondition;
	float foodValue;
	uint32_t woodValueMultiplier;
	uint32_t foodType;
	float field_0x64;
	float field_0x68;
	float field_0x6c;
	float field_0x70;
	float field_0x74;
	uint32_t field_0x78;
	uint32_t field_0x7c;
	float field_0x80;
	float field_0x84;
	float field_0x88;
	float field_0x8c;
	float field_0x90;
	float field_0x94;
	float field_0x98;
	float weight;
	float heatCapacity;
	float field_0xa4;
	float field_0xa8;
	uint32_t isBuildingMaterial;
	uint32_t suitableForCreatureAction;
	uint32_t field_0xb4;
	uint32_t field_0xb8;
	uint32_t canBeInspectedByCreature;
	uint32_t field_0xc0;
	int field_0xc4;
	uint32_t field_0xc8;
	float field_0xcc;
	float field_0xd0;
	float field_0xd4;
	float field_0xd8;
	float aggressorDamageMultiplier;
	float field_0xe0;
	float field_0xe4;
	float drawImportance;
	float field_0xec;
};

struct GMobileInfo: GObjectInfo
{
	uint32_t field_0xf0;
};

struct GMobileWallHugInfo: GMobileInfo
{
	uint32_t field_0xf4;
	uint32_t field_0xf8;
	uint32_t field_0xfc;
	uint32_t field_0x100;
	uint32_t field_0x104;
	uint32_t field_0x108;
	uint32_t field_0x10c;
};

struct GLivingInfo: GMobileWallHugInfo
{
	uint32_t field_0x110;
	uint32_t field_0x114;
	float life;
	float field_0x11c;
	float field_0x120;
	int field_0x124;
	uint32_t field_0x128;
	uint32_t field_0x12c;
	uint32_t field_0x130;
	uint32_t field_0x134;
	uint32_t field_0x138;
	uint32_t field_0x13c;
	uint32_t field_0x140;
	uint32_t field_0x144;
	uint32_t field_0x148;
	uint32_t field_0x14c;
	uint32_t field_0x150;
	uint32_t field_0x154;
	uint32_t field_0x158;
	uint32_t field_0x15c;
	uint32_t field_0x160;
	uint32_t field_0x164;
	uint32_t field_0x168;
	uint32_t field_0x16c;
	uint32_t field_0x170;
	uint32_t field_0x174;
	uint32_t field_0x178;
	uint32_t field_0x17c;
	uint32_t field_0x180;
	uint32_t field_0x184;
	uint32_t field_0x188;
	uint32_t field_0x18c;
	uint32_t field_0x190;
	uint32_t field_0x194;
	uint32_t field_0x198;
	uint32_t field_0x19c;
	uint32_t field_0x1a0;
	uint32_t field_0x1a4;
	uint32_t field_0x1a8;
	uint32_t field_0x1ac;
	uint32_t field_0x1b0;
	uint32_t field_0x1b4;
	uint32_t field_0x1b8;
	uint32_t field_0x1bc;
	uint32_t field_0x1c0;
	uint32_t field_0x1c4;
	uint32_t field_0x1c8;
	uint32_t field_0x1cc;
	uint32_t field_0x1d0;
	uint32_t field_0x1d4;
	uint32_t field_0x1d8;
	uint32_t field_0x1dc;
	uint32_t field_0x1e0;
};

struct GAnimalInfo: GLivingInfo
{
	uint32_t field_0x1e4;
	uint32_t field_0x1e8;
	uint32_t field_0x1ec;
	uint32_t field_0x1f0;
	uint32_t field_0x1f4;
	uint32_t field_0x1f8;
	uint32_t field_0x1fc;
	uint32_t field_0x200;
	uint32_t field_0x204;
	uint32_t field_0x208;
	float field_0x20c;
	uint32_t field_0x210;
	uint32_t field_0x214;
	uint32_t field_0x218;
	uint32_t field_0x21c;
	uint32_t field_0x220;
	uint32_t field_0x224;
	float field_0x228;
	float field_0x22c;
	float field_0x230;
	uint32_t field_0x234;
	uint32_t field_0x238;
	uint32_t field_0x23c;
	uint32_t field_0x240;
	uint32_t field_0x244;
	uint32_t field_0x248;
	uint32_t field_0x24c;
	uint32_t field_0x250;
	uint32_t field_0x254;
	float field_0x258;
	float field_0x25c;
	float field_0x260;
	float field_0x264;
	float field_0x268;
	float field_0x26c;
	float field_0x270;
	float field_0x274;
	float field_0x278;
	float field_0x27c;
	float field_0x280;
	float field_0x284;
	float field_0x288;
	float field_0x28c;
	float field_0x290;
	float field_0x294;
	float field_0x298;
	float field_0x29c;
	float field_0x2a0;
	float field_0x2a4;
	float field_0x2a8;
	float field_0x2ac;
	float field_0x2b0;
	float field_0x2b4;
	float field_0x2b8;
};

struct GMobileObjectInfo: GMobileInfo
{
	int field_0xf4;
	uint32_t field_0xf8;
	float field_0xfc;
	float field_0x100;
};

struct GPotInfo: GMobileObjectInfo
{
	uint32_t field_0x104;
	uint32_t field_0x108;
	uint32_t field_0x10c;
	uint32_t field_0x110;
	uint32_t field_0x114;
	uint32_t field_0x118;
	uint32_t field_0x11c;
	uint32_t field_0x120;
	uint32_t field_0x124;
	uint32_t field_0x128;
	uint32_t field_0x12c;
	float field_0x130;
};

struct GFootballPositionInfo
{
	float field_0x0;
	float field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
};

struct GBallInfo: GMobileObjectInfo
{
	uint32_t field_0x104;
	uint32_t field_0x108;
	uint32_t field_0x10c;
	float field_0x110;
	float field_0x114;
	float field_0x118;
	float field_0x11c;
	float field_0x120;
	float field_0x124;
	float field_0x128;
	float field_0x12c;
};

struct GSoundInfo
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	float field_0x10;
	float field_0x14;
	float field_0x18;
	float field_0x1c;
	float field_0x20;
	float field_0x24;
	float field_0x28;
	float field_0x2c;
	float field_0x30;
	float field_0x34;
	float field_0x38;
};

struct GMagicFireBallInfo: GObjectInfo
{
	glm::vec3 field_0xf0;
};

struct GDanceInfo
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	std::array<char, 0x40> field_0x8;
	std::array<char, 0x40> field_0x48;
	uint32_t field_0x88;
	float field_0x8c;
	float field_0x90;
	uint32_t field_0x94;
	uint32_t field_0x98;
	float field_0x9c;
};

struct GSpecialVillagerInfo
{
	std::array<char, 0x30> name;
	uint32_t field_0x30;
	uint32_t field_0x34;
	uint32_t field_0x38;
	uint32_t field_0x3c;
	uint32_t field_0x40;
	int field_0x44;
	uint32_t field_0x48;
};

struct CreatureDevelopmentPhaseEntry
{
	std::array<char, 0x30> name;
	float field_0x30;
	float field_0x34;
	float field_0x38;
	uint32_t field_0x3c;
	uint32_t field_0x40;
	uint32_t field_0x44;
	uint32_t field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
	uint32_t field_0x58;
	uint32_t field_0x5c;
	uint32_t field_0x60;
	uint32_t field_0x64;
	uint32_t field_0x68;
	uint32_t field_0x6c;
	uint32_t field_0x70;
};

struct CreatureInitialDesireInfo
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
	uint32_t field_0x28;
	uint32_t field_0x2c;
	uint32_t field_0x30;
	uint32_t field_0x34;
	uint32_t field_0x38;
	float field_0x3c;
	float field_0x40;
	float field_0x44;
	float field_0x48;
	float field_0x4c;
	float field_0x50;
	float field_0x54;
	float field_0x58;
	uint32_t field_0x5c;
	float field_0x60;
	float field_0x64;
	float field_0x68;
	uint32_t field_0x6c;
	std::array<char, 0x40> field_0x70;
	std::array<char, 0x40> field_0xb0;
	std::array<char, 0x40> field_0xf0;
	std::array<char, 0x40> field_0x130;
	std::array<char, 0x40> field_0x170;
};

struct CreatureActionKnownAboutEntry
{
	std::array<char, 0x40> field_0x0;
	uint32_t field_0x40;
	uint32_t field_0x44;
	float field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
};

struct CreatureDesireAttributeEntry
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
};

struct GMultiMapFixedInfo: GObjectInfo
{
	int field_0xf0;
	uint32_t field_0xf4;
	uint32_t field_0xf8;
	uint32_t field_0xfc;
	uint32_t field_0x100;
	float field_0x104;
	float field_0x108;
	float field_0x10c;
};

struct GCitadelPartInfo: GMultiMapFixedInfo
{
	uint32_t field_0x110;
	uint32_t field_0x114;
	float field_0x118;
	uint32_t field_0x11c;
	float field_0x120;
};

struct GWorshipSiteInfo: GCitadelPartInfo
{
	float field_0x124;
	uint32_t field_0x128;
	uint32_t field_0x12c;
	uint32_t field_0x130;
	float field_0x134;
	float field_0x138;
	uint32_t field_0x13c;
	float field_0x140;
	float field_0x144;
	uint32_t field_0x148;
	float field_0x14c;
};

struct GAbodeInfo: GMultiMapFixedInfo
{
	int field_0x110;
	AbodeNumber abodeNumber;
	std::array<char, 0x20> name;
	uint32_t field_0x138;
	uint32_t field_0x13c;
	uint32_t field_0x140;
	uint32_t field_0x144;
	Tribe tribeType;
	uint32_t meshId;
	uint32_t field_0x150;
	float field_0x154;
	uint32_t field_0x158;
	float field_0x15c;
	uint32_t field_0x160;
	int maxCapacity;
	int field_0x168;
	uint32_t field_0x16c;
	uint32_t field_0x170;
	uint32_t field_0x174;
	uint32_t field_0x178;
	uint32_t field_0x17c;
	uint32_t field_0x180;
	uint32_t field_0x184;
	uint32_t field_0x188;
	uint32_t field_0x18c;
	float field_0x190;
	int field_0x194;
	float field_0x198;
	float field_0x19c;
	float field_0x1a0;
	int field_0x1a4;
	float field_0x1a8;
	uint32_t field_0x1ac;
	uint32_t field_0x1b0;
	uint32_t field_0x1b4;

	static AbodeInfo Find(const std::string& name);
	static AbodeInfo Find(Tribe tribe, AbodeNumber abodeNumber);
};

struct GSingleMapFixedInfo: GObjectInfo
{
	uint32_t field_0xf0;
};

struct GMapShieldInfo: GSingleMapFixedInfo
{
	uint32_t field_0xf4;
};

struct GRewardInfo: GMobileObjectInfo
{
	int field_0x104;
	uint32_t field_0x108;
	uint32_t field_0x10c;
	uint32_t field_0x110;
	float field_0x114;
	int field_0x118;
	uint32_t field_0x11c;
};

struct GContainerInfo
{
	uint32_t field_0x0;
};

struct GCitadelInfo: GContainerInfo
{
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
	uint32_t field_0x28;
	uint32_t field_0x2c;
	uint32_t field_0x30;
	float field_0x34;
	float field_0x38;
	uint32_t field_0x3c;
	float field_0x40;
};

struct CreatureDevelopmentDurationEntry
{
	int field_0x0;
	int field_0x4;
	int field_0x8;
	int field_0xc;
	int field_0x10;
	int field_0x14;
	int field_0x18;
	int field_0x1c;
	int field_0x20;
	int field_0x24;
	int field_0x28;
	int field_0x2c;
	int field_0x30;
	int field_0x34;
};

struct GTownInfo: GContainerInfo
{
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
	uint32_t field_0x28;
	uint32_t field_0x2c;
	uint32_t field_0x30;
	uint32_t field_0x34;
	uint32_t field_0x38;
	uint32_t field_0x3c;
	uint32_t field_0x40;
	uint32_t field_0x44;
	uint32_t field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
	uint32_t field_0x58;
	uint32_t field_0x5c;
	uint32_t field_0x60;
	float field_0x64;
	float field_0x68;
	float field_0x6c;
	float field_0x70;
	uint32_t field_0x74;
	float field_0x78;
	float field_0x7c;
	float field_0x80;
	float field_0x84;
	float field_0x88;
	float field_0x8c;
	float field_0x90;
	float field_0x94;
	float field_0x98;
	float field_0x9c;
	uint32_t field_0xa0;
	float field_0xa4;
	float field_0xa8;
	float field_0xac;
	float field_0xb0;
	float field_0xb4;
	float field_0xb8;
	float field_0xbc;
	float field_0xc0;
	float field_0xc4;
	float field_0xc8;
	float field_0xcc;
	float field_0xd0;
	float field_0xd4;
	float field_0xd8;
	float field_0xdc;
	float field_0xe0;
	uint32_t field_0xe4;
	float field_0xe8;
	float field_0xec;
	uint32_t field_0xf0;
	float field_0xf4;
	float field_0xf8;
	float field_0xfc;
	uint32_t field_0x100;
	uint32_t field_0x104;
	uint32_t field_0x108;
	uint32_t field_0x10c;
	float field_0x110;
	float field_0x114;
	float field_0x118;
	uint32_t field_0x11c;
	float field_0x120;
	float field_0x124;
	float field_0x128;
	float field_0x12c;
	float field_0x130;
	float field_0x134;
	float field_0x138;
	float field_0x13c;
	uint32_t field_0x140;
	float field_0x144;
	float field_0x148;
	float field_0x14c;
	uint32_t field_0x150;
	float field_0x154;
	uint32_t field_0x158;
	float field_0x15c;
	float field_0x160;
	float field_0x164;
	float field_0x168;
	float field_0x16c;
	float field_0x170;
	float field_0x174;
	float field_0x178;
};

struct GJobInfo
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	float field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
	uint32_t field_0x28;
	uint32_t field_0x2c;
	uint32_t field_0x30;
	float field_0x34;
	uint32_t field_0x38;
	int field_0x3c;
	int field_0x40;
	int field_0x44;
};

struct GMagicInfo
{
	int typeEnum;
	uint32_t immersionTypeEnum;
	uint32_t stopImmersion;
	float perceivedPower;
	uint32_t particleTypeEnum;
	uint32_t impressiveTypeEnum;
	uint32_t spellSeedTypeEnum;
	uint32_t gestureType;
	uint32_t powerupType;
	uint32_t castRuleType;
	uint32_t isSpellSeedDrawnInHand;
	uint32_t isSpellRecharged;
	uint32_t isCreatureCastFromAbove;
	uint32_t oneOffSpellIsPlayful;
	uint32_t oneOffSpellIsAggressive;
	uint32_t oneOffSpellIsCompassionate;
	uint32_t oneOffSpellIsToRestoreHealth;
	uint32_t field_0x44;
};

struct GMagicResourceInfo: GMagicInfo
{
	uint32_t field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
	uint32_t field_0x58;
};

struct GPBallInfo: GMobileObjectInfo
{
	float field_0x104;
	float field_0x108;
};

struct GAlignmentInfo
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	float field_0x10;
	float field_0x14;
	float field_0x18;
	float field_0x1c;
	float field_0x20;
	float field_0x24;
	float field_0x28;
	float field_0x2c;
	float field_0x30;
	uint32_t field_0x34;
};

struct GClimateInfo
{
	std::array<char, 0x30> field_0x0;
	float field_0x30;
	float field_0x34;
	float field_0x38;
	float field_0x3c;
	float field_0x40;
	float field_0x44;
	float field_0x48;
	float field_0x4c;
	float field_0x50;
	float field_0x54;
	float field_0x58;
	float field_0x5c;
	float field_0x60;
	float field_0x64;
	float field_0x68;
	float field_0x6c;
	float field_0x70;
	float field_0x74;
	float field_0x78;
	float field_0x7c;
	float field_0x80;
	float field_0x84;
	float field_0x88;
	float field_0x8c;
};

struct GCreaturePenInfo: GCitadelPartInfo
{
	uint32_t field_0x124;
	float field_0x128;
	float field_0x12c;
	uint32_t field_0x130;
};

struct GScriptOpposingCreature
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
};

struct GBigForestInfo: GMultiMapFixedInfo
{
	uint32_t field_0x110;
	uint32_t field_0x114;
};

struct GTotemStatueInfo: GMultiMapFixedInfo
{
	float field_0x110;
};

struct CreatureDesireForType
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	float field_0x10;
	float field_0x14;
	float field_0x18;
	float field_0x1c;
	float field_0x20;
	float field_0x24;
	float field_0x28;
	float field_0x2c;
	float field_0x30;
	float field_0x34;
	float field_0x38;
	float field_0x3c;
	float field_0x40;
};

struct GVortexInfo
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	float field_0x18;
	int field_0x1c;
	int field_0x20;
	int field_0x24;
	int field_0x28;
	int field_0x2c;
};

struct GScaffoldInfo: GMobileObjectInfo
{
	float field_0x104;
	uint32_t field_0x108;
	float field_0x10c;
	float field_0x110;
	uint16_t field_0x114;
};

struct GScriptHighlightInfo: GSingleMapFixedInfo
{
	uint32_t field_0xf4;
	uint32_t field_0xf8;
	uint32_t field_0xfc;
};

struct GSpeedThreshold
{
	uint32_t field_0x0;
	uint32_t field_0x4;
};

struct GCreatureInfo: GLivingInfo
{
	std::array<uint8_t, 416> field_0x1e4;
};

struct GMagicRadiusSpellInfo: GMagicInfo
{
	glm::vec3 field_0x48;
};

struct GMagicStormAndTornadoInfo: GMagicRadiusSpellInfo
{
	float field_0x54;
	float field_0x58;
};

struct GHelpSpritesGuidance
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
	uint32_t field_0x28;
	uint32_t field_0x2c;
	uint32_t field_0x30;
	uint32_t field_0x34;
	uint32_t field_0x38;
	uint32_t field_0x3c;
	uint32_t field_0x40;
	uint32_t field_0x44;
	uint32_t field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
	uint32_t field_0x58;
	uint32_t field_0x5c;
	uint32_t field_0x60;
	uint32_t field_0x64;
	uint32_t field_0x68;
	uint32_t field_0x6c;
	uint32_t field_0x70;
	uint32_t field_0x74;
	uint32_t field_0x78;
	uint32_t field_0x7c;
	uint32_t field_0x80;
	uint32_t field_0x84;
};

struct GFeatureInfo: GMultiMapFixedInfo
{
	uint32_t field_0x110;
};

struct GAnimatedStaticInfo: GFeatureInfo
{
	uint32_t field_0x114;
	uint32_t field_0x118;
};

struct GWorshipSiteUpgradeInfo: GFeatureInfo
{
	float field_0x114;
};

struct GArrowInfo: GMobileObjectInfo
{
	uint32_t field_0x104;
	uint32_t field_0x108;
	uint32_t field_0x10c;
	uint32_t field_0x110;
	uint32_t field_0x114;
	uint32_t field_0x118;
	uint32_t field_0x11c;
};

struct GFieldInfo: GMultiMapFixedInfo
{
	float field_0x110;
	float field_0x114;
	uint32_t field_0x118;
	uint32_t field_0x11c;
	uint32_t field_0x120;
	int field_0x124;
	uint32_t field_0x128;
	uint32_t field_0x12c;
	uint32_t field_0x130;
	uint32_t field_0x134;
	float field_0x138;
};

struct GBeliefInfo
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	float field_0x10;
	float field_0x14;
};

struct GShowNeedsInfo: GObjectInfo
{
	uint32_t field_0xf0;
	int field_0xf4;
	float field_0xf8;
	float field_0xfc;
	float field_0x100;
};

struct GTerrainMaterialInfo
{
	uint32_t field_0x0;
	int field_0x4;
	float field_0x8;
	std::array<char, 0x30> name;
	uint32_t field_0x3c;
	uint32_t field_0x40;
	uint32_t field_0x44;
	uint32_t field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
	uint32_t field_0x58;
	uint32_t field_0x5c;
};

struct GFlowersInfo: GFeatureInfo
{
	float field_0x114;
};

struct HelpSystemInfo
{
	float field_0x0;
	float field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
};

struct GMagicForestInfo: GMagicInfo
{
	uint32_t field_0x48;
	float field_0x4c;
	float field_0x50;
	float field_0x54;
	float field_0x58;
};

struct GVillagerStateTableInfo
{
	uint32_t field_0x0;
	int field_0x4;
	float field_0x8;
	int field_0xc;
	int field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	int field_0x20;
	uint32_t field_0x24;
	std::array<char, 0x80> name;
	int field_0xa8;
	uint32_t field_0xac;
	uint32_t field_0xb0;
	uint32_t field_0xb4;
	int field_0xb8;
	uint32_t field_0xbc;
	int field_0xc0;
	int field_0xc4;
	float field_0xc8;
	float field_0xcc;
	uint32_t field_0xd0;
	uint32_t field_0xd4;
	uint32_t field_0xd8;
	int field_0xdc;
	uint32_t field_0xe0;
	uint32_t field_0xe4;
	uint32_t field_0xe8;
	uint32_t field_0xec;
	uint32_t field_0xf0;
	uint32_t field_0xf4;
	float field_0xf8;
	uint32_t field_0xfc;
	uint32_t field_0x100;
};

struct GVillagerInfo: GLivingInfo
{
	Tribe tribeType;
	uint32_t field_0x1e8;
	VillagerNumber villagerNumber;
	uint32_t meshId;
	uint32_t field_0x1f4;
	uint32_t field_0x1f8;
	int field_0x1fc;
	uint32_t field_0x200;
	uint32_t field_0x204;
	uint32_t field_0x208;
	uint32_t field_0x20c;
	uint32_t field_0x210;
	float field_0x214;
	uint32_t field_0x218;
	uint32_t field_0x21c;
	uint32_t field_0x220;
	uint32_t field_0x224;
	int field_0x228;
	uint32_t field_0x22c;
	uint32_t field_0x230;
	uint32_t field_0x234;
	uint32_t field_0x238;
	uint32_t field_0x23c;
	float field_0x240;
	float field_0x244;
	uint32_t field_0x248;
	uint32_t field_0x24c;
	float field_0x250;
	uint32_t field_0x254;
	uint32_t field_0x258;
	uint32_t field_0x25c;
	uint32_t field_0x260;
	uint32_t field_0x264;
	uint32_t field_0x268;
	float field_0x26c;
	float field_0x270;
	uint32_t field_0x274;
	float field_0x278;
	float field_0x27c;
	uint32_t field_0x280;
	uint32_t field_0x284;
	uint32_t field_0x288;
	uint32_t field_0x28c;
	uint32_t field_0x290;
	float field_0x294;
	uint32_t field_0x298;
	uint32_t field_0x29c;
	uint32_t field_0x2a0;
	uint32_t field_0x2a4;
	float field_0x2a8;
	float field_0x2ac;
	float field_0x2b0;
	float field_0x2b4;
	float field_0x2b8;
	float field_0x2bc;
	float field_0x2c0;
	float field_0x2c4;
	uint32_t field_0x2c8;
	uint32_t field_0x2cc;
	uint32_t field_0x2d0;
	uint32_t field_0x2d4;
	float field_0x2d8;
	float field_0x2dc;
	float field_0x2e0;
	float field_0x2e4;
	float field_0x2e8;
	float field_0x2ec;
	float field_0x2f0;
	float field_0x2f4;
	float field_0x2f8;
	float field_0x2fc;
	float field_0x300;
	float field_0x304;
	float field_0x308;
	float field_0x30c;
	float field_0x310;
	float field_0x314;
	float field_0x318;
	float field_0x31c;
	float field_0x320;
	float field_0x324;
	uint32_t field_0x328;
	uint32_t field_0x32c;
	uint32_t field_0x330;
	uint32_t field_0x334;
	uint32_t field_0x338;
	uint32_t field_0x33c;
	float field_0x340;
	float field_0x344;
	uint32_t field_0x348;
	float field_0x34c;
	float field_0x350;
	uint32_t field_0x354;
	float field_0x358;
	float field_0x35c;
	float field_0x360;
	float field_0x364;
	float field_0x368;
	float field_0x36c;
	float field_0x370;
	uint32_t field_0x374;
	uint32_t field_0x378;
	float field_0x37c;
	float field_0x380;
	uint16_t field_0x384;
	uint16_t field_0x386;
	uint16_t field_0x388;
	uint16_t field_0x38a;
	uint16_t field_0x38c;
	uint16_t field_0x38e;
	uint16_t field_0x390;
	uint16_t field_0x392;

	static VillagerInfo Find(Tribe tribe, VillagerNumber villagerNumber);
};

struct CreatureDesireActionEntry
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
	uint32_t field_0x28;
	uint32_t field_0x2c;
	uint32_t field_0x30;
	uint32_t field_0x34;
	uint32_t field_0x38;
	uint32_t field_0x3c;
	uint32_t field_0x40;
	uint32_t field_0x44;
	uint32_t field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
	uint32_t field_0x58;
	uint32_t field_0x5c;
	uint32_t field_0x60;
	uint32_t field_0x64;
	uint32_t field_0x68;
	uint32_t field_0x6c;
	uint32_t field_0x70;
	uint32_t field_0x74;
};

struct GMagicTeleportInfo: GMagicInfo
{
	float field_0x48;
};

struct GPlaytimeInfo
{
	int field_0x0;
	float field_0x4;
	uint32_t field_0x8;
	int field_0xc;
	int field_0x10;
};

struct CreatureMagicActionKnownAboutEntry: CreatureActionKnownAboutEntry
{
	uint32_t field_0x58;
	float field_0x5c;
};

struct CreatureSourceBoundsInfo
{
	glm::vec3 field_0x0;
};

struct GPlayerInfo
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	uint32_t field_0x10;
	float field_0x14;
	float field_0x18;
	float field_0x1c;
	float field_0x20;
	float field_0x24;
	float field_0x28;
	float field_0x2c;
	float field_0x30;
	uint32_t field_0x34;
	float field_0x38;
	float field_0x3c;
};

struct ReactionInfo
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	float field_0x1c;
	float field_0x20;
	uint32_t field_0x24;
	float field_0x28;
	uint32_t field_0x2c;
	float field_0x30;
	float field_0x34;
	float field_0x38;
	float field_0x3c;
	int field_0x40;
	int field_0x44;
	float field_0x48;
	uint32_t field_0x4c;
};

struct GMagicHealInfo: GMagicInfo
{
	float field_0x48;
	uint32_t field_0x4c;
};

struct GMagicFlockGroundInfo: GMagicInfo
{
	uint32_t field_0x48;
	float field_0x4c;
	float field_0x50;
	float field_0x54;
};

struct GAnimalStateTableInfo
{
	uint32_t field_0x0;
	int field_0x4;
	float field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	std::array<char, 0x80> name;
	uint32_t field_0xa4;
};

struct GLeashSelectorInfo: GObjectInfo
{
	uint32_t field_0xf0;
};

struct CreatureInitialSourceInfo
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	float field_0x10;
	float field_0x14;
	float field_0x18;
	float field_0x1c;
	float field_0x20;
	float field_0x24;
	float field_0x28;
	float field_0x2c;
	float field_0x30;
	float field_0x34;
	float field_0x38;
	float field_0x3c;
	float field_0x40;
};

struct GMagicWaterInfo: GMagicInfo
{
};

struct GMobileStaticInfo: GMultiMapFixedInfo
{
	uint32_t field_0x110;
	float field_0x114;
	uint32_t field_0x118;
};

struct GToolTipsInfo
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
};

struct GWeatherInfo
{
	std::array<char, 0x30> field_0x0;
	float field_0x30;
	float field_0x34;
	float field_0x38;
	int field_0x3c;
	int field_0x40;
	int field_0x44;
	uint32_t field_0x48;
	int field_0x4c;
	int field_0x50;
};

struct GPrayerIconInfo: GSingleMapFixedInfo
{
	uint32_t field_0xf4;
};

struct GInfluenceInfo
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
};

struct GTreeInfo: GSingleMapFixedInfo
{
	uint32_t field_0xf4;
	uint32_t field_0xf8;
	uint32_t field_0xfc;
	float field_0x100;
	float field_0x104;
	uint32_t field_0x108;
	float field_0x10c;
	uint32_t field_0x110;
	uint32_t field_0x114;
	float field_0x118;
	float field_0x11c;
	float field_0x120;
	float field_0x124;
	uint32_t field_0x128;
	uint16_t field_0x12c;
};

struct DifferentCreatureInfo
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	float field_0x10;
	float field_0x14;
	float field_0x18;
	float field_0x1c;
	float field_0x20;
	float field_0x24;
	float field_0x28;
	float field_0x2c;
	float field_0x30;
	float field_0x34;
	float field_0x38;
	float field_0x3c;
	float field_0x40;
	float field_0x44;
	float field_0x48;
	uint32_t field_0x4c;
	float field_0x50;
	float field_0x54;
	float field_0x58;
};

struct GEffectInfo
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	uint32_t field_0x10;
	float field_0x14;
	uint32_t field_0x18;
	float field_0x1c;
	uint32_t field_0x20;
};

struct GMagicEffectInfo: GEffectInfo
{
	std::array<char, 0x30> name;
	uint32_t field_0x54;
	uint32_t field_0x58;
	uint32_t field_0x5c;
	uint32_t field_0x60;
	float field_0x64;
	float field_0x68;
	float field_0x6c;
	float field_0x70;
	float field_0x74;
	uint32_t field_0x78;
	uint32_t field_0x7c;
	uint32_t field_0x80;
	int field_0x84;
	uint32_t field_0x88;
	uint32_t field_0x8c;
	int field_0x90;
	uint32_t field_0x94;
	float field_0x98;
	float field_0x9c;
	float field_0xa0;
	float field_0xa4;
	float field_0xa8;
	float field_0xac;
	float field_0xb0;
	float field_0xb4;
	float field_0xb8;
	uint32_t field_0xbc;
	uint32_t field_0xc0;
	uint32_t field_0xc4;
	uint32_t field_0xc8;
	uint32_t field_0xcc;
	uint32_t field_0xd0;
	uint32_t field_0xd4;
	uint32_t field_0xd8;
	uint32_t field_0xdc;
	uint32_t field_0xe0;
	uint32_t field_0xe4;
	uint32_t field_0xe8;
	uint32_t field_0xec;
	uint32_t field_0xf0;
	uint32_t field_0xf4;
	uint32_t field_0xf8;
	uint32_t field_0xfc;
	uint32_t field_0x100;
	float field_0x104;
	float field_0x108;
};

struct GTownDesireInfo
{
	int field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	int field_0x10;
	int field_0x14;
	int field_0x18;
	uint32_t field_0x1c;
	int field_0x20;
	float field_0x24;
	float field_0x28;
	float field_0x2c;
	float field_0x30;
	float field_0x34;
	float field_0x38;
	float field_0x3c;
	float field_0x40;
	uint32_t field_0x44;
	float field_0x48;
	float field_0x4c;
	float field_0x50;
	float field_0x54;
	float field_0x58;
	float field_0x5c;
	float field_0x60;
	float field_0x64;
	float field_0x68;
	uint32_t field_0x6c;
	uint32_t field_0x70;
	uint32_t field_0x74;
	uint32_t field_0x78;
	uint32_t field_0x7c;
};

struct GMagicFlockFlyingInfo: GMagicInfo
{
	uint32_t field_0x48;
	float field_0x4c;
	float field_0x50;
};

struct GSpotVisualInfo
{
	std::array<char, 0x30> name;
	uint32_t field_0x30;
	uint32_t field_0x34;
	uint32_t field_0x38;
	uint32_t field_0x3c;
	uint32_t field_0x40;
};

struct GPFootballInfo: GMultiMapFixedInfo
{
	uint32_t field_0x110;
};

struct GSpellSeedInfo: GObjectInfo
{
	uint32_t field_0xf0;
	uint32_t field_0xf4;
	uint32_t field_0xf8;
	uint32_t field_0xfc;
	uint32_t field_0x100;
	uint32_t field_0x104;
	uint32_t field_0x108;
	uint32_t field_0x10c;
	uint32_t field_0x110;
	uint32_t field_0x114;
	uint32_t field_0x118;
	uint32_t field_0x11c;
	uint32_t field_0x120;
	uint32_t field_0x124;
	uint32_t field_0x128;
	uint32_t field_0x12c;
	uint32_t field_0x130;
	float field_0x134;
	float field_0x138;
	float field_0x13c;
	float field_0x140;
	float field_0x144;
	uint32_t field_0x148;
	uint32_t field_0x14c;
	float field_0x150;
	float field_0x154;
	uint32_t field_0x158;
	float field_0x15c;
	uint32_t field_0x160;
	uint32_t field_0x164;
	uint32_t field_0x168;
	uint32_t field_0x16c;
	uint32_t field_0x170;
	uint32_t field_0x174;
	uint32_t field_0x178;
	uint32_t field_0x17c;
};

struct GCitadelHeartInfo: GCitadelPartInfo
{
	uint32_t field_0x124;
	uint32_t field_0x128;
	uint32_t field_0x12c;
	float field_0x130;
	float field_0x134;
	float field_0x138;
	float field_0x13c;
	float field_0x140;
	float field_0x144;
};

struct GMagicCreatureSpellInfo: GMagicInfo
{
	uint32_t field_0x48;
	std::array<char, 0x30> name;
	float field_0x7c;
	float field_0x80;
	float field_0x84;
	float field_0x88;
};

struct GRewardProgress
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
};

struct GFurnitureInfo: GObjectInfo
{
	uint32_t field_0xf0;
};

struct GSpookyVoiceInfo
{
	uint32_t field_0x0;
	std::array<char, 30> name;
};

struct GFieldTypeInfo: GMultiMapFixedInfo
{
	float field_0x110;
	float field_0x114;
	float field_0x118;
	float field_0x11c;
	float field_0x120;
	uint32_t field_0x124;
	float field_0x128;
	float field_0x12c;
	float field_0x130;
	float field_0x134;
	int field_0x138;
	float field_0x13c;
	float field_0x140;
};

struct CreatureActionInfo
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
	uint32_t field_0xc;
	float field_0x10;
	std::array<char, 0x20> name;
	uint32_t field_0x34;
	uint32_t field_0x38;
	uint32_t field_0x3c;
	uint32_t field_0x40;
	uint32_t field_0x44;
	uint32_t field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
	uint32_t field_0x58;
	uint32_t field_0x5c;
	uint32_t field_0x60;
	uint32_t field_0x64;
	uint32_t field_0x68;
	uint32_t field_0x6c;
	uint32_t field_0x70;
	uint32_t field_0x74;
	uint32_t field_0x78;
	uint32_t field_0x7c;
	uint32_t field_0x80;
	uint32_t field_0x84;
	uint32_t field_0x88;
	uint32_t field_0x8c;
	uint32_t field_0x90;
	uint32_t field_0x94;
	uint32_t field_0x98;
	uint32_t field_0x9c;
	uint32_t field_0xa0;
	uint32_t field_0xa4;
	uint32_t field_0xa8;
	uint32_t field_0xac;
	uint32_t field_0xb0;
	uint32_t field_0xb4;
	uint32_t field_0xb8;
	uint32_t field_0xbc;
	float field_0xc0;
	float field_0xc4;
	uint32_t field_0xc8;
	float field_0xcc;
	float field_0xd0;
	float field_0xd4;
	uint32_t field_0xd8;
	uint32_t field_0xdc;
	uint32_t field_0xe0;
	uint32_t field_0xe4;
	float field_0xe8;
	float field_0xec;
	uint32_t field_0xf0;
	uint32_t field_0xf4;
	uint32_t field_0xf8;
	uint32_t field_0xfc;
};

struct GSpellSystemInfo
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
	float field_0xc;
	float field_0x10;
	float field_0x14;
	uint32_t field_0x18;
	float field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
	uint32_t field_0x28;
	uint32_t field_0x2c;
	uint32_t field_0x30;
	uint32_t field_0x34;
	uint32_t field_0x38;
};

struct GFishFarmInfo: GMultiMapFixedInfo
{
	uint32_t field_0x110;
	uint32_t field_0x114;
};

struct CreatureDesireDependency
{
	uint32_t field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
	uint32_t field_0x28;
	uint32_t field_0x2c;
	uint32_t field_0x30;
	uint32_t field_0x34;
	uint32_t field_0x38;
	uint32_t field_0x3c;
	float field_0x40;
	uint32_t field_0x44;
	uint32_t field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
	uint32_t field_0x58;
	float field_0x5c;
	float field_0x60;
	float field_0x64;
	float field_0x68;
	float field_0x6c;
	float field_0x70;
	float field_0x74;
	float field_0x78;
	float field_0x7c;
	float field_0x80;
	float field_0x84;
	float field_0x88;
	float field_0x8c;
	float field_0x90;
	float field_0x94;
	uint32_t field_0x98;
	uint32_t field_0x9c;
};

struct GPFootballPositionInfo
{
	float field_0x0;
	float field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
};

struct CreatureMimicInfo
{
	std::array<char, 0x80> name;
	float field_0x80;
	uint32_t field_0x84;
	uint32_t field_0x88;
	uint32_t field_0x8c;
	uint32_t field_0x90;
	uint32_t field_0x94;
	uint32_t field_0x98;
	uint32_t field_0x9c;
	uint32_t field_0xa0;
	uint32_t field_0xa4;
	uint32_t field_0xa8;
	uint32_t field_0xac;
};

struct GPrayerSiteInfo: GMultiMapFixedInfo
{
	uint32_t field_0x110;
	float field_0x114;
};

struct HelpSpiritInfo: GLivingInfo
{
	uint32_t field_0x1e4;
};

struct CreatureDesireSourceTable
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	float field_0x8;
	float field_0xc;
	float field_0x10;
	std::array<char, 0x40> field_0x14;
	std::array<char, 0x40> field_0x54;
};

struct GSpellIconInfo: GMultiMapFixedInfo
{
	glm::vec3 field_0x110;
};

struct GMagicShieldInfo: GMagicRadiusSpellInfo
{
	float field_0x54;
	uint32_t field_0x58;
	float field_0x5c;
	float field_0x60;
};

struct InfoConstants
{
	std::array<GMagicInfo, 10> magic;
	std::array<GMagicHealInfo, 2> magicHeal;
	GMagicTeleportInfo magicTeleport;
	GMagicForestInfo magicForest;
	std::array<GMagicResourceInfo, 2> magicResource_1;
	std::array<GMagicStormAndTornadoInfo, 3> magicStormAndTornado;
	std::array<GMagicShieldInfo, 2> magicShield;
	GMagicResourceInfo magicResource_2;
	std::array<GMagicWaterInfo, 2> magicWater;
	GMagicFlockFlyingInfo magicFlockFlying;
	GMagicFlockGroundInfo magicFlockGround;
	std::array<GMagicCreatureSpellInfo, 0x10> magicCreatureSpell;
	std::array<GMagicEffectInfo, 42> magicEffect;
	std::array<GSpellSeedInfo, 30> spellSeed;
	std::array<GAnimalInfo, 31> animal;
	std::array<GCreatureInfo, 17> creature;
	std::array<DifferentCreatureInfo, 17> differentCreature;
	std::array<CreatureDesireForType, 40> creatureDesireForType;
	std::array<CreatureDevelopmentPhaseEntry, 14> creatureDevelopmentPhaseEntry;
	std::array<CreatureDevelopmentDurationEntry, 17> creatureDevelopmentDurationEntry;
	GCitadelInfo citadel;
	GCitadelHeartInfo citadelHeart;
	std::array<GCreaturePenInfo, 5> creaturePen;
	std::array<GWorshipSiteInfo, 9> worshipSite;
	std::array<GSpellIconInfo, 2> spellIcon;
	std::array<GAbodeInfo, static_cast<size_t>(AbodeInfo::_COUNT)> abode;
	std::array<GVillagerInfo, static_cast<size_t>(VillagerInfo::_COUNT)> villager;
	std::array<GSpecialVillagerInfo, 0x30> specialVillager;
	std::array<GTreeInfo, 23> tree;
	std::array<GSingleMapFixedInfo, 4> singleMapFixed;
	std::array<GScriptHighlightInfo, 4> scriptHighlight;
	std::array<GMapShieldInfo, 2> mapShield;
	GBallInfo ball;
	GTownInfo town;
	std::array<GJobInfo, 0x10> job;
	std::array<GFeatureInfo, 76> feature;
	std::array<GFlowersInfo, 2> flowers;
	std::array<GAnimatedStaticInfo, 0x10> animatedStatic;
	std::array<GMobileObjectInfo, 0x20> mobileObject;
	GScaffoldInfo scaffold;
	std::array<GMobileStaticInfo, 61> mobileStatic;
	std::array<GPotInfo, 19> pot;
	GPrayerIconInfo prayerIcon;
	GPrayerSiteInfo prayerSite;
	std::array<GShowNeedsInfo, 4> showNeeds;
	std::array<GTownDesireInfo, 17> townDesire;
	GWorshipSiteUpgradeInfo worshipSiteUpgrade;
	std::array<HelpSpiritInfo, 3> helpSpirit;
	GArrowInfo arrow;
	std::array<GSpotVisualInfo, 50> spotVisual;
	std::array<GEffectInfo, 12> effect;
	std::array<GFieldInfo, 2> field;
	std::array<GFieldTypeInfo, static_cast<size_t>(FieldTypeInfo::_COUNT)> fieldType;
	GFishFarmInfo fishFarm;
	std::array<GFootballPositionInfo, 10> footballPosition;
	std::array<GPlaytimeInfo, 5> playtime;
	GPlayerInfo player;
	GSoundInfo sound;
	GBeliefInfo belief;
	std::array<GHelpSpritesGuidance, 22> helpSpritesGuidance;
	GInfluenceInfo influence;
	HelpSystemInfo helpSystem;
	std::array<GAlignmentInfo, 7> alignment;
	std::array<ReactionInfo, 41> reaction;
	std::array<CreatureActionInfo, 328> creatureAction;
	std::array<CreatureDesireActionEntry, 40> creatureDesireAction_1;
	std::array<CreatureDesireActionEntry, 17> creatureDesireAction_2;
	std::array<CreatureDesireActionEntry, 40> creatureDesireAction_3;
	std::array<CreatureDesireDependency, 40> creatureDesireDependency;
	std::array<CreatureInitialDesireInfo, 40> creatureInitialDesire;
	std::array<CreatureDesireSourceTable, 61> desireSourceTable;
	std::array<CreatureInitialSourceInfo, 61> creatureInitialSource_1;
	std::array<CreatureInitialSourceInfo, 61> creatureInitialSource_2;
	std::array<CreatureSourceBoundsInfo, 61> creatureSourceBounds;
	std::array<CreatureDesireAttributeEntry, 40> creatureDesireAttributeEntry;
	std::array<CreatureActionKnownAboutEntry, 6> creatireActionKnownAboutEntry;
	std::array<CreatureMagicActionKnownAboutEntry, 42> creatureMagicActionKnownAboutEntry;
	std::array<CreatureMimicInfo, 46> creatureMimic;
	std::array<GTerrainMaterialInfo, 43> terrainMaterial;
	std::array<GTribeInfo, 9> tribe;
	std::array<GSpeedThreshold, 10> speedThreshold;
	GPBallInfo pBall;
	GPFootballInfo pFootball;
	std::array<GPFootballPositionInfo, 10> pFootballPosition;
	std::array<GDanceInfo, 25> dance;
	std::array<GVillagerStateTableInfo, 0xFF> villagerStateTable;
	std::array<GAnimalStateTableInfo, 53> animalStateTable;
	std::array<GTotemStatueInfo, 9> totemStatue;
	std::array<GBigForestInfo, 4> bigForest;
	std::array<GFurnitureInfo, 31> furniture;
	std::array<GWeatherInfo, 7> weather;
	std::array<GClimateInfo, 7> climate;
	GSpellSystemInfo spellSystem;
	std::array<GVortexInfo, 3> vortex;
	std::array<GMagicFireBallInfo, 3> magicFireBall;
	std::array<GRewardInfo, 61> reward;
	GLeashSelectorInfo leashSelector;
	std::array<GRewardProgress, 30> rewardProgressGood;
	std::array<GRewardProgress, 30> rewardProgressEvil;
	std::array<GSpookyVoiceInfo, 5> spookyVoice;
	std::array<GScriptOpposingCreature, 17> scriptOpposingCreature;
	std::array<GToolTipsInfo, 170> toolTips;
};
#pragma pack(pop)
static_assert(std::is_trivial<InfoConstants>::value, "GInfo must be trivial to be read properly");
static_assert(sizeof(InfoConstants) == 0x8E186);

} // namespace openblack
