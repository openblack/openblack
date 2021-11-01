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
