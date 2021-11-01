/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "MeshLookup.h"

using namespace openblack;
using namespace openblack::ecs::components;

MeshLookup<MobileStatic::Info> openblack::mobileStaticMeshLookup {
    {MobileStatic::Info::Rock, MeshPackId::ObjectFlatRockChalk},
    {MobileStatic::Info::Bonfire, MeshPackId::BuildingCampfire},
    {MobileStatic::Info::Boulder1Chalk, MeshPackId::Boulder1Chalk},
    {MobileStatic::Info::Boulder1Lime, MeshPackId::Boulder1Lime},
    {MobileStatic::Info::Boulder1Sand, MeshPackId::Boulder1Sand},
    {MobileStatic::Info::Boulder1Volcanic, MeshPackId::Boulder1Volcanic},
    {MobileStatic::Info::Boulder2Chalk, MeshPackId::Boulder2Chalk},
    {MobileStatic::Info::Boulder2Lime, MeshPackId::Boulder2Lime},
    {MobileStatic::Info::Boulder2Sand, MeshPackId::Boulder2Sand},
    {MobileStatic::Info::Boulder2Volcanic, MeshPackId::Boulder2Volcanic},
    {MobileStatic::Info::Boulder3Chalk, MeshPackId::Boulder3Chalk},
    {MobileStatic::Info::Boulder3Lime, MeshPackId::Boulder3Lime},
    {MobileStatic::Info::Boulder3Sand, MeshPackId::Boulder3Sand},
    {MobileStatic::Info::Boulder3Volcanic, MeshPackId::Boulder3Volcanic},
    {MobileStatic::Info::CeltFenceShort, MeshPackId::BuildingCelticFenceShort},
    {MobileStatic::Info::CeltFenceTall, MeshPackId::BuildingCelticFenceTall},
    {MobileStatic::Info::CountryLantern, MeshPackId::ObjectTownLight},
    {MobileStatic::Info::DeadTree, MeshPackId::ObjectBurntTree},
    {MobileStatic::Info::ToyBall, MeshPackId::ObjectToyBall},
    {MobileStatic::Info::Idol, MeshPackId::FeatureIdol},
    {MobileStatic::Info::ToyDie, MeshPackId::ObjectToyDice},
    {MobileStatic::Info::ToyCuddly, MeshPackId::ObjectToyCuddly},
    {MobileStatic::Info::ToySkittle, MeshPackId::ObjectToySkittle},
    {MobileStatic::Info::Vortex, MeshPackId::SpellVortexCylinder},
    {MobileStatic::Info::GateTotemApe, MeshPackId::ObjectGateTotemApe},
    {MobileStatic::Info::GateTotemCow, MeshPackId::ObjectGateTotemCow},
    {MobileStatic::Info::GateTotemTiger, MeshPackId::ObjectGateTotemTiger},
    {MobileStatic::Info::GateTotemBlank, MeshPackId::ObjectGateTotemBlank},
    {MobileStatic::Info::LongrockChalk, MeshPackId::ObjectLongRockChalk},
    {MobileStatic::Info::LongrockLimestone, MeshPackId::ObjectLongRockLimeStone},
    {MobileStatic::Info::LongrockSandstone, MeshPackId::ObjectLongRockSandStone},
    {MobileStatic::Info::LongrockVolcanic, MeshPackId::ObjectLongRockVolcanic},
    {MobileStatic::Info::RockChalk, MeshPackId::ObjectRockChalk},
    {MobileStatic::Info::RockLimestone, MeshPackId::ObjectRockLimeStone},
    {MobileStatic::Info::RockSandstone, MeshPackId::ObjectRockSandStone},
    {MobileStatic::Info::RockVolcanic, MeshPackId::ObjectRockVolcanic},
    {MobileStatic::Info::SharprockChalk, MeshPackId::ObjectSharpRockChalk},
    {MobileStatic::Info::SharprockLimestone, MeshPackId::ObjectSharpRockLimeStone},
    {MobileStatic::Info::SharprockSandstone, MeshPackId::ObjectSharpRockSandStone},
    {MobileStatic::Info::SharprockVolcanic, MeshPackId::ObjectSharpRockVolcanic}};

MeshLookup<MobileObject::Info> openblack::mobileObjectMeshLookup {
    {MobileObject::Info::EgyptBarrel, MeshPackId::BuildingEgyptianBarrel},
    {MobileObject::Info::EgyptCart, MeshPackId::BuildingEgyptianCart},
    {MobileObject::Info::EgyptPotA, MeshPackId::BuildingEgyptianPotA},
    {MobileObject::Info::EgyptPotB, MeshPackId::BuildingEgyptianPotB},
    {MobileObject::Info::MagicFood, MeshPackId::Dummy},
    {MobileObject::Info::LumpOfPoo, MeshPackId::ObjectCreatureTurd},
    {MobileObject::Info::WaterJug, MeshPackId::Dummy},
    {MobileObject::Info::Arrow, MeshPackId::Dummy},
    {MobileObject::Info::Ball, MeshPackId::Dummy},
    {MobileObject::Info::Cart, MeshPackId::BuildingEgyptianCart},
    {MobileObject::Info::FoodPot, MeshPackId::BuildingEgyptianPotA},
    {MobileObject::Info::WoodPot, MeshPackId::BuildingEgyptianPotA},
    {MobileObject::Info::PileFood, MeshPackId::BuildingEgyptianPotA},
    {MobileObject::Info::PileWood, MeshPackId::BuildingEgyptianPotA},
    {MobileObject::Info::MagicWood, MeshPackId::BuildingWood00},
    {MobileObject::Info::Crop, MeshPackId::TreeBush},
    {MobileObject::Info::OldScaffold, MeshPackId::Dummy},
    {MobileObject::Info::Champi, MeshPackId::ObjectChampi},
    {MobileObject::Info::Toadstool, MeshPackId::ObjectToadstool},
    {MobileObject::Info::MagicMushroom, MeshPackId::ObjectMagicMushroom},
    {MobileObject::Info::CreatureSwapOrb, MeshPackId::Dummy},
    {MobileObject::Info::Creed, MeshPackId::Creed},
    {MobileObject::Info::Eyes, MeshPackId::Creed},
    {MobileObject::Info::Ark, MeshPackId::ObjectArk},
    {MobileObject::Info::Whale, MeshPackId::ObjectLargeFishDead},
    {MobileObject::Info::OneOffSpellSeed, MeshPackId::Dummy},
    {MobileObject::Info::HanoiPuzzleBase, MeshPackId::BuildingXandoBase},
    {MobileObject::Info::HanoiPuzzlePart1, MeshPackId::Puzzle1},
    {MobileObject::Info::HanoiPuzzlePart2, MeshPackId::Puzzle2},
    {MobileObject::Info::HanoiPuzzlePart3, MeshPackId::Puzzle3},
    {MobileObject::Info::HanoiPuzzlePart4, MeshPackId::Puzzle4},
};
