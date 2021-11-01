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

MeshLookup<Tree::Info> openblack::treeMeshLookup {
    {Tree::Info::Beech, MeshPackId::TreeBeech},       //
    {Tree::Info::Birch, MeshPackId::TreeBirch},       //
    {Tree::Info::Cedar, MeshPackId::TreeCedar},       //
    {Tree::Info::Conifer, MeshPackId::TreeConifer},   //
    {Tree::Info::ConiferA, MeshPackId::TreeConiferA}, //
    {Tree::Info::Oak, MeshPackId::TreeOak},           //
    {Tree::Info::OakA, MeshPackId::TreeOakA},         //
    {Tree::Info::Olive, MeshPackId::TreeOlive},       //
    {Tree::Info::Palm, MeshPackId::TreePalm},         //
    {Tree::Info::PalmA, MeshPackId::TreePalmA},       //
    {Tree::Info::PalmB, MeshPackId::TreePalmB},       //
    {Tree::Info::PalmC, MeshPackId::TreePalmC},       //
    {Tree::Info::Pine, MeshPackId::TreePine},         //
    {Tree::Info::Bush, MeshPackId::TreeBush},         //
    {Tree::Info::BushA, MeshPackId::TreeBushA},       //
    {Tree::Info::BushB, MeshPackId::TreeBushB},       //
    {Tree::Info::Cypress, MeshPackId::TreeCypress},   //
    {Tree::Info::CypressA, MeshPackId::TreeCypressA}, //
    {Tree::Info::Copse, MeshPackId::TreeCopse},       //
    {Tree::Info::CopseA, MeshPackId::TreeCopseA},     //
    {Tree::Info::Hedge, MeshPackId::TreeHedge},       //
    {Tree::Info::HedgeA, MeshPackId::TreeHedgeA},     //
    {Tree::Info::Burnt, MeshPackId::ObjectBurntTree}, //
};

MeshLookup<AbodeInfo> openblack::abodeMeshLookup {
    // Norse
    {AbodeInfo::NorseTownCentre, MeshPackId::BuildingNorseVillageCentre},
    {AbodeInfo::NorseStoragePit, MeshPackId::BuildingNorseStoragePit},
    {AbodeInfo::NorseGraveyard, MeshPackId::BuildingNorseGraveyard},
    {AbodeInfo::NorseWorkshop, MeshPackId::BuildingNorseWorkshop},
    {AbodeInfo::NorseCreche, MeshPackId::BuildingNorseCreche},
    {AbodeInfo::NorseHut, MeshPackId::BuildingNorse1},
    {AbodeInfo::NorseShackX, MeshPackId::BuildingNorse2},
    {AbodeInfo::NorseShackY, MeshPackId::BuildingNorse3},
    {AbodeInfo::NorseTent, MeshPackId::BuildingNorse4},
    {AbodeInfo::NorseTempleX, MeshPackId::BuildingNorse5},
    {AbodeInfo::NorseTempleY, MeshPackId::BuildingNorse2A},
    {AbodeInfo::NorseWonder, MeshPackId::BuildingNorseWonder},
    // Celtic
    {AbodeInfo::CelticTownCentre, MeshPackId::BuildingCelticVillageCentre},
    {AbodeInfo::CelticStoragePit, MeshPackId::BuildingCelticStoragePit},
    {AbodeInfo::CelticGraveyard, MeshPackId::BuildingCelticGraveyard},
    {AbodeInfo::CelticWorkshop, MeshPackId::BuildingCelticWorkshop},
    {AbodeInfo::CelticCreche, MeshPackId::BuildingCelticCreche},
    {AbodeInfo::CelticHut, MeshPackId::BuildingCeltic1},
    {AbodeInfo::CelticShackX, MeshPackId::BuildingCeltic2},
    {AbodeInfo::CelticShackY, MeshPackId::BuildingCeltic3},
    {AbodeInfo::CelticTent, MeshPackId::BuildingCeltic4},
    {AbodeInfo::CelticTempleX, MeshPackId::BuildingCeltic5},
    {AbodeInfo::CelticWonder, MeshPackId::BuildingCelticWonder},
    // Japanese
    {AbodeInfo::JapaneseTownCentre, MeshPackId::BuildingJapaneseVillageCentre},
    {AbodeInfo::JapaneseStoragePit, MeshPackId::BuildingJapaneseStoragePit},
    {AbodeInfo::JapaneseGraveyard, MeshPackId::BuildingJapaneseGraveyard},
    {AbodeInfo::JapaneseWorkshop, MeshPackId::BuildingJapaneseWorkshop},
    {AbodeInfo::JapaneseCreche, MeshPackId::BuildingJapaneseCreche},
    {AbodeInfo::JapaneseHut, MeshPackId::BuildingJapanese1},
    {AbodeInfo::JapaneseShackX, MeshPackId::BuildingJapanese2},
    {AbodeInfo::JapaneseShackY, MeshPackId::BuildingJapanese3},
    {AbodeInfo::JapaneseTent, MeshPackId::BuildingJapanese4},
    {AbodeInfo::JapaneseTempleX, MeshPackId::BuildingJapanese5},
    {AbodeInfo::JapaneseWonder, MeshPackId::BuildingJapaneseWonder},
    // Aztec
    {AbodeInfo::AztecTownCentre, MeshPackId::BuildingAztecVillageCentre},
    {AbodeInfo::AztecStoragePit, MeshPackId::BuildingAztecStoragePit},
    {AbodeInfo::AztecGraveyard, MeshPackId::BuildingAztecGraveyard},
    {AbodeInfo::AztecWorkshop, MeshPackId::BuildingAztecWorkshop},
    {AbodeInfo::AztecCreche, MeshPackId::BuildingAztecCreche},
    {AbodeInfo::AztecHut, MeshPackId::BuildingAztec1},
    {AbodeInfo::AztecShackX, MeshPackId::BuildingAztec2},
    {AbodeInfo::AztecShackY, MeshPackId::BuildingAztec3},
    {AbodeInfo::AztecTent, MeshPackId::BuildingAztec4},
    {AbodeInfo::AztecTempleX, MeshPackId::BuildingAztec5},
    {AbodeInfo::AztecWonder, MeshPackId::BuildingAztecWonder},
    // Tibetan
    {AbodeInfo::TibetanTownCentre, MeshPackId::BuildingTibetanVillageCentre},
    {AbodeInfo::TibetanStoragePit, MeshPackId::BuildingTibetanStoragePit},
    {AbodeInfo::TibetanGraveyard, MeshPackId::BuildingTibetanGraveyard},
    {AbodeInfo::TibetanWonder, MeshPackId::BuildingTibetanWorkshop},
    {AbodeInfo::TibetanCreche, MeshPackId::BuildingTibetanCreche},
    {AbodeInfo::TibetanHut, MeshPackId::BuildingTibetan1},
    {AbodeInfo::TibetanShackX, MeshPackId::BuildingTibetan2},
    {AbodeInfo::TibetanShackY, MeshPackId::BuildingTibetan3},
    {AbodeInfo::TibetanTent, MeshPackId::BuildingTibetan4},
    {AbodeInfo::TibetanTempleX, MeshPackId::BuildingTibetan5},
    {AbodeInfo::TibetanWonder, MeshPackId::BuildingTibetanWonder},
    // Indian
    {AbodeInfo::IndianTownCentre, MeshPackId::BuildingAmericanVillageCentre},
    {AbodeInfo::IndianStoragePit, MeshPackId::BuildingAmericanStoragePit},
    {AbodeInfo::IndianGraveyard, MeshPackId::BuildingAmericanGraveyard},
    {AbodeInfo::IndianWorkshop, MeshPackId::BuildingAmericanWorkshop},
    {AbodeInfo::IndianCreche, MeshPackId::BuildingAmericanCreche},
    {AbodeInfo::IndianHut, MeshPackId::BuildingAmerican1},
    {AbodeInfo::IndianShackX, MeshPackId::BuildingAmerican2},
    {AbodeInfo::IndianShackY, MeshPackId::BuildingAmerican3},
    {AbodeInfo::IndianTent, MeshPackId::BuildingAmerican4},
    {AbodeInfo::IndianTempleX, MeshPackId::BuildingAmerican5},
    {AbodeInfo::IndianWonder, MeshPackId::BuildingAmericanWonder},
    // Greek
    {AbodeInfo::GreekTownCentre, MeshPackId::BuildingGreekVillageCentre},
    {AbodeInfo::GreekStoragePit, MeshPackId::BuildingGreekStoragePit},
    {AbodeInfo::GreekGraveyard, MeshPackId::BuildingGreekGraveyard},
    {AbodeInfo::GreekWorkshop, MeshPackId::BuildingGreekWorkshop},
    {AbodeInfo::GreekCreche, MeshPackId::BuildingGreekCreche},
    {AbodeInfo::GreekHut, MeshPackId::BuildingGreek1},
    {AbodeInfo::GreekShackX, MeshPackId::BuildingGreek2},
    {AbodeInfo::GreekShackY, MeshPackId::BuildingGreek3},
    {AbodeInfo::GreekTent, MeshPackId::BuildingGreek4},
    {AbodeInfo::GreekTempleX, MeshPackId::BuildingGreek5},
    {AbodeInfo::GreekTempleY, MeshPackId::BuildingGreek5A},
    {AbodeInfo::GreekWonder, MeshPackId::BuildingGreekWonder},
};

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

MeshLookup<Feature::Info> openblack::featureMeshLookup {
    {Feature::Info::FatPilarSand, MeshPackId::FatPilarSand},
    {Feature::Info::Pilar2Sand, MeshPackId::Pilar2Sand},
    {Feature::Info::Pilar3Sand, MeshPackId::Pilar3Sand},
    {Feature::Info::FatPilarChalk, MeshPackId::FatPilarChalk},
    {Feature::Info::Pilar2Chalk, MeshPackId::Pilar2Chalk},
    {Feature::Info::Pilar3Chalk, MeshPackId::Pilar3Chalk},
    {Feature::Info::FatPilarLime, MeshPackId::FatPilarLime},
    {Feature::Info::Pilar2Lime, MeshPackId::Pilar2Lime},
    {Feature::Info::Pilar3Lime, MeshPackId::Pilar3Lime},
    {Feature::Info::PilarVolcanic, MeshPackId::PilarVolcanic},
    {Feature::Info::Pilar2Volcanic, MeshPackId::Pilar2Volcanic},
    {Feature::Info::Pilar3Volcanic, MeshPackId::Pilar3Volcanic},
    {Feature::Info::Ark, MeshPackId::ObjectArk},
    {Feature::Info::ArkWreck, MeshPackId::ObjectArkWreck},
    {Feature::Info::ArkDryDock, MeshPackId::ObjectArkDryDock},
    {Feature::Info::MagicMushroom, MeshPackId::ObjectMagicMushroom},
    {Feature::Info::Toadstool, MeshPackId::ObjectToadstool},
    {Feature::Info::AztcStatue, MeshPackId::BuildingAztecStatue},
    {Feature::Info::SpikeyPilarLime, MeshPackId::SpikeyPilarLime},
    {Feature::Info::SpikeyPilarChalk, MeshPackId::SpikeyPilarChalk},
    {Feature::Info::SpikeyPilarSand, MeshPackId::SpikeyPilarSand},
    {Feature::Info::SpikeyPilarVolcanic, MeshPackId::SpikeyPilarVolcanic},
};
