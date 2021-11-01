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

MeshLookup<Abode::Info> openblack::abodeMeshLookup {
    // Norse
    {Abode::Info::NorseTownCentre, MeshPackId::BuildingNorseVillageCentre},
    {Abode::Info::NorseStoragePit, MeshPackId::BuildingNorseStoragePit},
    {Abode::Info::NorseGraveyard, MeshPackId::BuildingNorseGraveyard},
    {Abode::Info::NorseWorkshop, MeshPackId::BuildingNorseWorkshop},
    {Abode::Info::NorseCreche, MeshPackId::BuildingNorseCreche},
    {Abode::Info::NorseHut, MeshPackId::BuildingNorse1},
    {Abode::Info::NorseShackX, MeshPackId::BuildingNorse2},
    {Abode::Info::NorseShackY, MeshPackId::BuildingNorse3},
    {Abode::Info::NorseTent, MeshPackId::BuildingNorse4},
    {Abode::Info::NorseTempleX, MeshPackId::BuildingNorse5},
    {Abode::Info::NorseTempleY, MeshPackId::BuildingNorse2A},
    {Abode::Info::NorseWonder, MeshPackId::BuildingNorseWonder},
    // Celtic
    {Abode::Info::CelticTownCentre, MeshPackId::BuildingCelticVillageCentre},
    {Abode::Info::CelticStoragePit, MeshPackId::BuildingCelticStoragePit},
    {Abode::Info::CelticGraveyard, MeshPackId::BuildingCelticGraveyard},
    {Abode::Info::CelticWorkshop, MeshPackId::BuildingCelticWorkshop},
    {Abode::Info::CelticCreche, MeshPackId::BuildingCelticCreche},
    {Abode::Info::CelticHut, MeshPackId::BuildingCeltic1},
    {Abode::Info::CelticShackX, MeshPackId::BuildingCeltic2},
    {Abode::Info::CelticShackY, MeshPackId::BuildingCeltic3},
    {Abode::Info::CelticTent, MeshPackId::BuildingCeltic4},
    {Abode::Info::CelticTempleX, MeshPackId::BuildingCeltic5},
    {Abode::Info::CelticWonder, MeshPackId::BuildingCelticWonder},
    // Japanese
    {Abode::Info::JapaneseTownCentre, MeshPackId::BuildingJapaneseVillageCentre},
    {Abode::Info::JapaneseStoragePit, MeshPackId::BuildingJapaneseStoragePit},
    {Abode::Info::JapaneseGraveyard, MeshPackId::BuildingJapaneseGraveyard},
    {Abode::Info::JapaneseWorkshop, MeshPackId::BuildingJapaneseWorkshop},
    {Abode::Info::JapaneseCreche, MeshPackId::BuildingJapaneseCreche},
    {Abode::Info::JapaneseHut, MeshPackId::BuildingJapanese1},
    {Abode::Info::JapaneseShackX, MeshPackId::BuildingJapanese2},
    {Abode::Info::JapaneseShackY, MeshPackId::BuildingJapanese3},
    {Abode::Info::JapaneseTent, MeshPackId::BuildingJapanese4},
    {Abode::Info::JapaneseTempleX, MeshPackId::BuildingJapanese5},
    {Abode::Info::JapaneseWonder, MeshPackId::BuildingJapaneseWonder},
    // Aztec
    {Abode::Info::AztecTownCentre, MeshPackId::BuildingAztecVillageCentre},
    {Abode::Info::AztecStoragePit, MeshPackId::BuildingAztecStoragePit},
    {Abode::Info::AztecGraveyard, MeshPackId::BuildingAztecGraveyard},
    {Abode::Info::AztecWorkshop, MeshPackId::BuildingAztecWorkshop},
    {Abode::Info::AztecCreche, MeshPackId::BuildingAztecCreche},
    {Abode::Info::AztecHut, MeshPackId::BuildingAztec1},
    {Abode::Info::AztecShackX, MeshPackId::BuildingAztec2},
    {Abode::Info::AztecShackY, MeshPackId::BuildingAztec3},
    {Abode::Info::AztecTent, MeshPackId::BuildingAztec4},
    {Abode::Info::AztecTempleX, MeshPackId::BuildingAztec5},
    {Abode::Info::AztecWonder, MeshPackId::BuildingAztecWonder},
    // Tibetan
    {Abode::Info::TibetanTownCentre, MeshPackId::BuildingTibetanVillageCentre},
    {Abode::Info::TibetanStoragePit, MeshPackId::BuildingTibetanStoragePit},
    {Abode::Info::TibetanGraveyard, MeshPackId::BuildingTibetanGraveyard},
    {Abode::Info::TibetanWonder, MeshPackId::BuildingTibetanWorkshop},
    {Abode::Info::TibetanCreche, MeshPackId::BuildingTibetanCreche},
    {Abode::Info::TibetanHut, MeshPackId::BuildingTibetan1},
    {Abode::Info::TibetanShackX, MeshPackId::BuildingTibetan2},
    {Abode::Info::TibetanShackY, MeshPackId::BuildingTibetan3},
    {Abode::Info::TibetanTent, MeshPackId::BuildingTibetan4},
    {Abode::Info::TibetanTempleX, MeshPackId::BuildingTibetan5},
    {Abode::Info::TibetanWonder, MeshPackId::BuildingTibetanWonder},
    // Indian
    {Abode::Info::IndianTownCentre, MeshPackId::BuildingAmericanVillageCentre},
    {Abode::Info::IndianStoragePit, MeshPackId::BuildingAmericanStoragePit},
    {Abode::Info::IndianGraveyard, MeshPackId::BuildingAmericanGraveyard},
    {Abode::Info::IndianWorkshop, MeshPackId::BuildingAmericanWorkshop},
    {Abode::Info::IndianCreche, MeshPackId::BuildingAmericanCreche},
    {Abode::Info::IndianHut, MeshPackId::BuildingAmerican1},
    {Abode::Info::IndianShackX, MeshPackId::BuildingAmerican2},
    {Abode::Info::IndianShackY, MeshPackId::BuildingAmerican3},
    {Abode::Info::IndianTent, MeshPackId::BuildingAmerican4},
    {Abode::Info::IndianTempleX, MeshPackId::BuildingAmerican5},
    {Abode::Info::IndianWonder, MeshPackId::BuildingAmericanWonder},
    // Greek
    {Abode::Info::GreekTownCentre, MeshPackId::BuildingGreekVillageCentre},
    {Abode::Info::GreekStoragePit, MeshPackId::BuildingGreekStoragePit},
    {Abode::Info::GreekGraveyard, MeshPackId::BuildingGreekGraveyard},
    {Abode::Info::GreekWorkshop, MeshPackId::BuildingGreekWorkshop},
    {Abode::Info::GreekCreche, MeshPackId::BuildingGreekCreche},
    {Abode::Info::GreekHut, MeshPackId::BuildingGreek1},
    {Abode::Info::GreekShackX, MeshPackId::BuildingGreek2},
    {Abode::Info::GreekShackY, MeshPackId::BuildingGreek3},
    {Abode::Info::GreekTent, MeshPackId::BuildingGreek4},
    {Abode::Info::GreekTempleX, MeshPackId::BuildingGreek5},
    {Abode::Info::GreekTempleY, MeshPackId::BuildingGreek5A},
    {Abode::Info::GreekWonder, MeshPackId::BuildingGreekWonder},
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
