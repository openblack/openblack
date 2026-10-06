/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ParticleTypes.h"

#include <array>
#include <utility>

namespace
{
/// Each particle type's name and file, by its number. Several types share a file: food and poisoned food, the healing
/// chakra and its spot visual, and the landscape vortex's in and out effects before it lands.
constexpr std::array<std::pair<std::string_view, std::string_view>, openblack::particles::k_ParticleTypeCount> k_Types {{
    {"None", ""},                                                                  // 0
    {"Tornado", ""},                                                               // 1
    {"Firework", ""},                                                              // 2
    {"Firework Single", ""},                                                       // 3
    {"Leaves", "SF_Forest"},                                                       // 4
    {"Magicball", ""},                                                             // 5
    {"Lightning", "SF_LightningStormPush"},                                        // 6
    {"Lightning Bolt", "SF_LightningBolt"},                                        // 7
    {"Explosion 1", ""},                                                           // 8
    {"Food", "SF_Food"},                                                           // 9
    {"Food Poisoned", "SF_Food"},                                                  // 10
    {"Explosion One", "SF_BeamExplosionSingle"},                                   // 11
    {"Explosion One Pu One", "SF_BeamExplosionMany"},                              // 12
    {"Explosion One Pu Two", "SF_BeamExplosionLoads"},                             // 13
    {"Explosion Citadel", "SF_BeamExplosionCitadel"},                              // 14
    {"Wood", "SF_Wood"},                                                           // 15
    {"Water", "SF_Water"},                                                         // 16
    {"Water Pu One", "SF_WaterPU1"},                                               // 17
    {"Water On Holder", "SF_WaterOnHolder"},                                       // 18
    {"Water In Hand", "SF_WaterInHand"},                                           // 19
    {"Water In Hand Pu One", "SF_WaterInHandPU1"},                                 // 20
    {"Heal", "SF_HealChakra"},                                                     // 21
    {"Mana Path", "SF_ManaPathNew"},                                               // 22
    {"Explode Object", "SF_ExplodeObject"},                                        // 23
    {"Belief Sprite", "SF_BeliefSprite"},                                          // 24
    {"Town Belief", "SF_TownBelief"},                                              // 25
    {"Spell Fail", "SF_FailedApply"},                                              // 26
    {"Spell Succeed", ""},                                                         // 27
    {"Spell Selection", "SF_SpellSelection"},                                      // 28
    {"Grip Landscape", "SF_GripLandscape"},                                        // 29
    {"Magic Object Created", "SF_MagicObjectCreated"},                             // 30
    {"Creature Gesture", "SF_CreatureGestureChain"},                               // 31
    {"Villager Teleport", "SF_TeleportVillager"},                                  // 32
    {"Creature Target", "SF_CreatureTarget"},                                      // 33
    {"Creature Cast Visual", "SF_SimpleBeamCreatureCast"},                         // 34
    {"Gesture", "SF_Gesture"},                                                     // 35
    {"Kb Test", "SF_Fireworks2"},                                                  // 36
    {"On Fire", "SF_OnFire"},                                                      // 37
    {"Magic Fx", "SF_VolFX"},                                                      // 38
    {"Magic Fx On Object", "SF_VolFXArtifact"},                                    // 39
    {"Magic Fx On Citadel", "SF_VolFXCitadel"},                                    // 40
    {"Fire Fx", ""},                                                               // 41
    {"Fire Fx On Object", ""},                                                     // 42
    {"Fireball", "SF_FireBallThrow"},                                              // 43
    {"Fireball In Hand", "SF_FireBallInHand"},                                     // 44
    {"Fireball In Hand Pu One", ""},                                               // 45
    {"Fireball In Hand Pu Two", ""},                                               // 46
    {"Fireball On Holder", "SF_FireBallOnHolder"},                                 // 47
    {"Gesture Local", "SF_GestureChain"},                                          // 48
    {"Magic System", ""},                                                          // 49
    {"Heal In Hand", "SF_HealChakraInHand"},                                       // 50
    {"Heal On Holder", "SF_HealChakraOnHolder"},                                   // 51
    {"Fireball Pu One", "SF_FireBallThrowPU"},                                     // 52
    {"Fireball Pu Two", "SF_FireBallThrowPU2"},                                    // 53
    {"Heal Pu One", "SF_HealChakraPU"},                                            // 54
    {"Lightning Storm In Hand", "SF_LightningStormInHand"},                        // 55
    {"Lightning Storm In Hand Pu One", "SF_LightningStormInHandPU1"},              // 56
    {"Lightning Storm In Hand Pu Two", "SF_LightningStormInHandPU2"},              // 57
    {"Lightning Storm On Holder", "SF_LightningStormOnHolder"},                    // 58
    {"Lightning Bolt In Hand", "SF_LightningBoltInHand"},                          // 59
    {"Lightning Bolt On Holder", "SF_LightningBoltOnHolder"},                      // 60
    {"Lightning Strike", "SF_LightningStrike"},                                    // 61
    {"Lightning Single Strike", "SF_LightningSingleStrike"},                       // 62
    {"Food In Hand", ""},                                                          // 63
    {"Shield", "SF_DefenseSphere"},                                                // 64
    {"Shield In Hand", "SF_DefenseSphereInHand"},                                  // 65
    {"Shield On Holder", "SF_DefenseSphereOnHolder"},                              // 66
    {"Physical Shield Fx", "SF_PhysicalShieldFX"},                                 // 67
    {"Lightning Bolt Pu One", "SF_LightningBoltPUOne"},                            // 68
    {"Lightning Bolt Pu Two", "SF_LightningBoltPUTwo"},                            // 69
    {"Lightning Bolt In Hand Pu One", "SF_LightningBoltInHandPUOne"},              // 70
    {"Lightning Bolt In Hand Pu Two", "SF_LightningBoltInHandPUTwo"},              // 71
    {"Teleport", ""},                                                              // 72
    {"Teleport Vortex", "SF_TeleportVortex"},                                      // 73
    {"Teleport On Holder", "SF_TeleportOnHolder"},                                 // 74
    {"Teleport In Hand", "SF_TeleportInHand"},                                     // 75
    {"Landscape Vortex Object Mover", "SF_LandscapeVortexObjectMover"},            // 76
    {"Landscape Vortex Lightmap", "SF_LandscapeVortexLightMap"},                   // 77
    {"Landscape Vortex In Before", "SF_LandscapeVortexInBefore"},                  // 78
    {"Landscape Vortex In After", "SF_LandscapeVortexInAfter"},                    // 79
    {"Landscape Vortex Out Before", "SF_LandscapeVortexInBefore"},                 // 80
    {"Landscape Vortex Out After", "SF_LandscapeVortexOutAfter"},                  // 81
    {"Volcano Vortex Before", "SF_LandscapeVolcanoBefore"},                        // 82
    {"Volcano Vortex After", "SF_LandscapeVolcanoAfter"},                          // 83
    {"Volcano Vortex Lightmap", "SF_LandscapeVolcanoLightMap"},                    // 84
    {"Creature Spell Physical", "SF_CreatureSpellGeneric"},                        // 85
    {"Creature Spell Mental", ""},                                                 // 86
    {"Creature Spell Itchy", "SF_CreatureSpellItch"},                              // 87
    {"Creature Spell Itchy In Hand", "SF_CreatureSpellItchInHand"},                // 88
    {"Creature Spell Itchy On Holder", "SF_CreatureSpellItchOnHolder"},            // 89
    {"Creature Spell Freeze", "SF_CreatureSpellFreeze"},                           // 90
    {"Creature Spell Freeze In Hand", ""},                                         // 91
    {"Creature Spell Freeze On Holder", "SF_CreatureSpellFreezeOnHolder"},         // 92
    {"Creature Spell Compassion", "SF_CreatureSpellCompassion"},                   // 93
    {"Creature Spell Compassion In Hand", ""},                                     // 94
    {"Creature Spell Compassion On Holder", "SF_CreatureSpellCompassionOnHolder"}, // 95
    {"Creature Spell Weak", ""},                                                   // 96
    {"Creature Spell Weak In Hand", ""},                                           // 97
    {"Creature Spell Weak On Holder", ""},                                         // 98
    {"Script Highlight Gold Glints", "SF_ScriptHighlightGlintsGold"},              // 99
    {"Script Highlight Silver Glints", "SF_ScriptHighlightGlintsSilver"},          // 100
    {"Script Highlight Bronze Glints", "SF_ScriptHighlightGlintsBronze"},          // 101
    {"Script Highlight Gold Active", "SF_ScriptHighlightActiveGold"},              // 102
    {"Script Highlight Silver Active", "SF_ScriptHighlightActiveSilver"},          // 103
    {"Flock Flying", ""},                                                          // 104
    {"Flock Ground", ""},                                                          // 105
    {"Storm Cast", "SF_StormCast"},                                                // 106
    {"Food Pickup", "SF_MultiPickUpFood"},                                         // 107
    {"Food Pickup Poisoned", "SF_MultiPickUpFoodPoisoned"},                        // 108
    {"Food Pickup Fish", "SF_MultiPickUpFoodFish"},                                // 109
    {"Wood Pickup", "SF_MultiPickUpWood"},                                         // 110
    {"Food Putdown Poisoned", "SF_MultiPutDownFoodPoisoned"},                      // 111
    {"Wood Putdown", "SF_MultiPutDownWood"},                                       // 112
    {"Food Putdown", "SF_MultiPutDownFood"},                                       // 113
    {"Steam", "SF_Steam"},                                                         // 114
    {"Smoke", "SF_Smoke"},                                                         // 115
    {"Bonfire", "SF_Bonfire"},                                                     // 116
    {"Evil Smoke", "SF_EvilSmoke"},                                                // 117
    {"Dust", ""},                                                                  // 118
    {"Magic Beam", "SF_SimpleBeam"},                                               // 119
    {"Magic Beam On Citadel", "SF_SimpleBeamCitadel"},                             // 120
    {"Magic Beam Creature Swap", "SF_SimpleBeamCreatureSwap"},                     // 121
    {"Flock Flying Cast Good", "SF_FlockFlyingCastGood"},                          // 122
    {"Flock Flying Cast Evil", "SF_FlockFlyingCastEvil"},                          // 123
    {"Flock Flying Rain Good", "SF_FlockFlyingRainGood"},                          // 124
    {"Flock Flying Rain Evil", "SF_FlockFlyingRainEvil"},                          // 125
    {"Flock Ground Dust", "SF_FlockGroundDust"},                                   // 126
    {"Butterflies", "SF_Butterflies"},                                             // 127
    {"Butterflies On Object", "SF_ButterfliesOnObject"},                           // 128
    {"Flies", "SF_Flies"},                                                         // 129
    {"Flies On Object", "SF_FliesOnObject"},                                       // 130
    {"Object Appear", "SF_MagicObjectCreated2"},                                   // 131
    {"Object Disappear", ""},                                                      // 132
    {"Sing Stones Glow", ""},                                                      // 133
    {"Player Icon Fountain", "SF_PlayerIconFountain"},                             // 134
    {"Bang", "SF_SmokeExplode"},                                                   // 135
    {"Heal Fx", "SF_HealChakra"},                                                  // 136
    {"Highlight On Object", "SF_HighlightOnObject"},                               // 137
    {"Beam Explosion Fx", "SF_BeamExplosionFX"},                                   // 138
    {"Flash", "SF_Flash"},                                                         // 139
    {"Ticker Tape", "SF_TickerTape"},                                              // 140
    {"Forest Created", "SF_ForestCreated"},                                        // 141
    {"Singing Stones Heal", "SF_SingingStonesHeal"},                               // 142
    {"Pilefood Speedup", "SF_SparklesFromObject"},                                 // 143
    {"Spelldispenser Vortex", "SF_SpellDispenserVortex"},                          // 144
    {"See This Beam", "SF_SeeThisBeam"},                                           // 145
    {"See This Beam2", ""},                                                        // 146
    {"Test", ""},                                                                  // 147
    {"Test2", ""},                                                                 // 148
    {"Test3", ""},                                                                 // 149
}};
} // namespace

std::string_view openblack::particles::ParticleTypeFile(ParticleType type)
{
	const auto index = static_cast<size_t>(type);
	return index < k_Types.size() ? k_Types.at(index).second : std::string_view {};
}

std::string_view openblack::particles::ParticleTypeName(ParticleType type)
{
	const auto index = static_cast<size_t>(type);
	return index < k_Types.size() ? k_Types.at(index).first : std::string_view {};
}
