/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

//  626 Meshes.
//  441 Anims.
//  0 with substitution.
//  0 with dummy object.

enum MESH_LIST
{
	MSH_INVALID = -1,                       //
	MSH_DUMMY = 0,                          //
	MSH_A_BAT_1 = 1,                        //
	MSH_A_BAT_2 = 2,                        //
	MSH_A_CROW_1 = 3,                       //
	MSH_A_CROW_2 = 4,                       //
	MSH_A_DOVE_1 = 5,                       //
	MSH_A_DOVE_2 = 6,                       //
	MSH_A_PIGEON_1 = 7,                     //
	MSH_A_PIGEON_2 = 8,                     //
	MSH_A_SEAGULL_1 = 9,                    //
	MSH_A_SEAGULL_2 = 10,                   //
	MSH_A_SPELL_DOVE = 11,                  //
	MSH_A_SWALLOW_1 = 12,                   //
	MSH_A_SWALLOW_2 = 13,                   //
	MSH_A_VULTURE_1 = 14,                   //
	MSH_A_VULTURE_2 = 15,                   //
	MSH_A_COW_1 = 16,                       //
	MSH_A_COW_2 = 17,                       //
	MSH_A_COW_3 = 18,                       //
	MSH_A_HORSE_1 = 19,                     //
	MSH_A_HORSE_2 = 20,                     //
	MSH_A_HORSE_3 = 21,                     //
	MSH_A_LEOPARD_1 = 22,                   //
	MSH_A_LEOPARD_2 = 23,                   //
	MSH_A_LEOPARD_3 = 24,                   //
	MSH_A_LION_1 = 25,                      //
	MSH_A_LION_2 = 26,                      //
	MSH_A_LION_3 = 27,                      //
	MSH_A_PIG_1 = 28,                       //
	MSH_A_PIG_2 = 29,                       //
	MSH_A_PIG_3 = 30,                       //
	MSH_SHARK_BONED = 31,                   //
	MSH_A_SHEEP_1 = 32,                     //
	MSH_A_SHEEP_2 = 33,                     //
	MSH_A_SHEEP_3 = 34,                     //
	MSH_A_TIGER_1 = 35,                     //
	MSH_A_TIGER_2 = 36,                     //
	MSH_A_TIGER_3 = 37,                     //
	MSH_A_TORTOISE_1 = 38,                  //
	MSH_A_TORTOISE_2 = 39,                  //
	MSH_A_TORTOISE_3 = 40,                  //
	MSH_A_WOLF_1 = 41,                      //
	MSH_A_WOLF_2 = 42,                      //
	MSH_A_WOLF_3 = 43,                      //
	MSH_A_ZEBRA_1 = 44,                     //
	MSH_A_ZEBRA_2 = 45,                     //
	MSH_A_ZEBRA_3 = 46,                     //
	MSH_B_AMCN_1 = 47,                      //
	MSH_B_AMCN_1_A = 48,                    //
	MSH_B_AMCN_2 = 49,                      //
	MSH_B_AMCN_2_A = 50,                    //
	MSH_B_AMCN_3 = 51,                      //
	MSH_B_AMCN_3_A = 52,                    //
	MSH_B_AMCN_4 = 53,                      //
	MSH_B_AMCN_5 = 54,                      //
	MSH_B_AMCN_CRECHE = 55,                 //
	MSH_B_AMCN_FENCE = 56,                  //
	MSH_B_AMCN_GRAVEYARD = 57,              //
	MSH_B_AMCN_TOTEM = 58,                  //
	MSH_B_AMCN_WONDER = 59,                 //
	MSH_B_AMCN_WORKSHOP = 60,               //
	MSH_B_AZTC_1 = 61,                      //
	MSH_B_AZTC_2 = 62,                      //
	MSH_B_AZTC_3 = 63,                      //
	MSH_B_AZTC_4 = 64,                      //
	MSH_B_AZTC_5 = 65,                      //
	MSH_B_AZTC_CRECHE = 66,                 //
	MSH_B_AZTC_GRAVEYARD = 67,              //
	MSH_B_AZTC_OLMECHEAD = 68,              //
	MSH_B_AZTC_STATUE = 69,                 //
	MSH_B_AZTC_TEMPLE = 70,                 //
	MSH_B_AZTC_TOTEM = 71,                  //
	MSH_B_AZTC_WONDER = 72,                 //
	MSH_B_AZTC_WORKSHOP = 73,               //
	MSH_B_CELT_1 = 74,                      //
	MSH_B_CELT_2 = 75,                      //
	MSH_B_CELT_3 = 76,                      //
	MSH_B_CELT_4 = 77,                      //
	MSH_B_CELT_5 = 78,                      //
	MSH_B_CELT_5A = 79,                     //
	MSH_B_CELT_CRECHE = 80,                 //
	MSH_B_CELT_FENCE_SHORT = 81,            //
	MSH_B_CELT_FENCE_TALL = 82,             //
	MSH_B_CELT_GRAVEYARD = 83,              //
	MSH_B_CELT_GRAVE_01 = 84,               //
	MSH_B_CELT_GRAVE_02 = 85,               //
	MSH_B_CELT_GRAVE_03 = 86,               //
	MSH_B_CELT_TOTEM = 87,                  //
	MSH_B_CELT_WALLGATE_SPELL = 88,         //
	MSH_B_CELT_WALLTOWER_SPELL = 89,        //
	MSH_B_CELT_WALL_SPELL = 90,             //
	MSH_B_CELT_WONDER = 91,                 //
	MSH_B_CELT_WORKSHOP = 92,               //
	MSH_B_CITADEL_AMCN_ALTAR = 93,          //
	MSH_B_CITADEL_AZTC_ALTAR = 94,          //
	MSH_B_CITADEL_CELT_ALTAR = 95,          //
	MSH_B_CITADEL_DESIRE_HUNGRY = 96,       //
	MSH_B_CITADEL_DESIRE_SLEEP = 97,        //
	MSH_B_CITADEL_EGPT_ALTAR = 98,          //
	MSH_B_CITADEL_GREK_ALTAR = 99,          //
	MSH_B_CITADEL_JAPN_ALTAR = 100,         //
	MSH_B_CITADEL_NORS_ALTAR = 101,         //
	MSH_B_CITADEL_POP_COUNTER_BEAD = 102,   //
	MSH_B_CITADEL_TIBT_ALTAR = 103,         //
	MSH_B_EGPT_1 = 104,                     //
	MSH_B_EGPT_1_A = 105,                   //
	MSH_B_EGPT_2 = 106,                     //
	MSH_B_EGPT_2_A = 107,                   //
	MSH_B_EGPT_3 = 108,                     //
	MSH_B_EGPT_4 = 109,                     //
	MSH_B_EGPT_5 = 110,                     //
	MSH_B_EGPT_5_A = 111,                   //
	MSH_B_EGPT_BARREL = 112,                //
	MSH_B_EGPT_CART = 113,                  //
	MSH_B_EGPT_CRECHE = 114,                //
	MSH_B_EGPT_GRAVEYARD = 115,             //
	MSH_B_EGPT_NEEDLE = 116,                //
	MSH_B_EGPT_POT_A = 117,                 //
	MSH_B_EGPT_POT_B = 118,                 //
	MSH_B_EGPT_TOTEM = 119,                 //
	MSH_B_EGPT_WELL = 120,                  //
	MSH_B_EGPT_WINDMILL = 121,              //
	MSH_B_EGPT_WONDER = 122,                //
	MSH_B_EGPT_WORKSHOP = 123,              //
	MSH_B_GREK_1 = 124,                     //
	MSH_B_GREK_2 = 125,                     //
	MSH_B_GREK_2_A = 126,                   //
	MSH_B_GREK_3 = 127,                     //
	MSH_B_GREK_4 = 128,                     //
	MSH_B_GREK_5 = 129,                     //
	MSH_B_GREK_5A = 130,                    //
	MSH_B_GREK_CRECHE = 131,                //
	MSH_B_GREK_GRAVEYARD = 132,             //
	MSH_B_GREK_SAIL = 133,                  //
	MSH_B_GREK_STATUE = 134,                //
	MSH_B_GREK_TOTEM = 135,                 //
	MSH_B_GREK_WONDER = 136,                //
	MSH_B_GREK_WORKSHOP = 137,              //
	MSH_B_JAPN_1 = 138,                     //
	MSH_B_JAPN_2 = 139,                     //
	MSH_B_JAPN_3 = 140,                     //
	MSH_B_JAPN_4 = 141,                     //
	MSH_B_JAPN_5 = 142,                     //
	MSH_B_JAPN_CRECHE = 143,                //
	MSH_B_JAPN_GRAVEYARD = 144,             //
	MSH_B_JAPN_TOTEM = 145,                 //
	MSH_B_JAPN_WONDER = 146,                //
	MSH_B_JAPN_WORKSHOP = 147,              //
	MSH_B_CAMPFIRE = 148,                   //
	MSH_B_CREATURE_PEN_LEASH_POLE = 149,    //
	MSH_B_FOOTBALL_CENTRECIRCLE = 150,      //
	MSH_B_FOOTBALL_CORNERFLAG = 151,        //
	MSH_B_FOOTBALL_GOAL = 152,              //
	MSH_B_FOOTBALL_PITCH = 153,             //
	MSH_B_FOOTBALL_PITCH_BIG = 154,         //
	MSH_B_MINE_ENTRANCE = 155,              //
	MSH_B_PIER = 156,                       //
	MSH_B_SCAFFOLD_01 = 157,                //
	MSH_B_SCAFFOLD_02 = 158,                //
	MSH_B_SCAFFOLD_03 = 159,                //
	MSH_B_XANDO_BASE = 160,                 //
	MSH_B_XANDO_O = 161,                    //
	MSH_B_XANDO_X = 162,                    //
	MSH_PIPER_ENTRANCE_PHYS_1 = 163,        //
	MSH_B_AMCN_VILLAGECENTRE = 164,         //
	MSH_B_AZTC_VILLAGECENTRE = 165,         //
	MSH_B_CELT_VILLAGECENTRE = 166,         //
	MSH_B_DESIRE_FLAG_ABODE = 167,          //
	MSH_B_DESIRE_FLAG_CHILDREN = 168,       //
	MSH_B_DESIRE_FLAG_CIVIC_BUILDING = 169, //
	MSH_B_DESIRE_FLAG_FOOD = 170,           //
	MSH_B_DESIRE_FLAG_PLAYTIME = 171,       //
	MSH_B_DESIRE_FLAG_PROTECTION = 172,     //
	MSH_B_DESIRE_FLAG_RAIN = 173,           //
	MSH_B_DESIRE_FLAG_SUN = 174,            //
	MSH_B_DESIRE_FLAG_WOOD = 175,           //
	MSH_B_EGPT_VILLAGECENTRE = 176,         //
	MSH_B_GREK_VILLAGECENTRE = 177,         //
	MSH_B_JAPN_VILLAGECENTRE = 178,         //
	MSH_B_NORS_VILLAGECENTRE = 179,         //
	MSH_B_PLAYER_ICON01 = 180,              //
	MSH_B_PLAYER_ICON01_PLINTH = 181,       //
	MSH_B_PLAYER_ICON_APE = 182,            //
	MSH_B_PLAYER_ICON_BEAR = 183,           //
	MSH_B_PLAYER_ICON_COW = 184,            //
	MSH_B_PLAYER_ICON_HORSE = 185,          //
	MSH_B_PLAYER_ICON_LEOPARD = 186,        //
	MSH_B_PLAYER_ICON_LION = 187,           //
	MSH_B_PLAYER_ICON_PLINTH_AMCN = 188,    //
	MSH_B_PLAYER_ICON_PLINTH_AZTC = 189,    //
	MSH_B_PLAYER_ICON_PLINTH_CELT = 190,    //
	MSH_B_PLAYER_ICON_PLINTH_EGPT = 191,    //
	MSH_B_PLAYER_ICON_PLINTH_GREK = 192,    //
	MSH_B_PLAYER_ICON_PLINTH_JAPN = 193,    //
	MSH_B_PLAYER_ICON_PLINTH_NORS = 194,    //
	MSH_B_PLAYER_ICON_PLINTH_TIBT = 195,    //
	MSH_B_PLAYER_ICON_POLARBEAR = 196,      //
	MSH_B_PLAYER_ICON_SHEEP = 197,          //
	MSH_B_PLAYER_ICON_TIGER = 198,          //
	MSH_B_PLAYER_ICON_TORTOISE = 199,       //
	MSH_B_PLAYER_ICON_WOLF = 200,           //
	MSH_B_PLAYER_ICON_ZEBRA = 201,          //
	MSH_B_TIBT_VILLAGECENTRE = 202,         //
	MSH_B_VILLAGECENTRE_SPELLHAND = 203,    //
	MSH_B_NORS_1 = 204,                     //
	MSH_B_NORS_2 = 205,                     //
	MSH_B_NORS_2_A = 206,                   //
	MSH_B_NORS_3 = 207,                     //
	MSH_B_NORS_3_A = 208,                   //
	MSH_B_NORS_4 = 209,                     //
	MSH_B_NORS_5 = 210,                     //
	MSH_B_NORS_CRECHE = 211,                //
	MSH_B_NORS_GATE = 212,                  //
	MSH_B_NORS_GRAVEYARD = 213,             //
	MSH_B_NORS_PIGPEN = 214,                //
	MSH_B_NORS_TEMPLE = 215,                //
	MSH_B_NORS_TOTEM = 216,                 //
	MSH_B_NORS_WONDER = 217,                //
	MSH_B_NORS_WORKSHOP = 218,              //
	MSH_NORS_GATE_PHYS_1 = 219,             //
	MSH_NORS_GATE_PHYS_2 = 220,             //
	MSH_B_AMCN_STORAGE_PIT = 221,           //
	MSH_B_AZTC_STORAGE_PIT = 222,           //
	MSH_B_CELT_STORAGE_PIT = 223,           //
	MSH_B_EGPT_STORAGE_PIT = 224,           //
	MSH_B_GRAIN = 225,                      //
	MSH_B_GREK_STORAGE_PIT = 226,           //
	MSH_B_JAPN_STORAGE_PIT = 227,           //
	MSH_B_NORS_STORAGE_PIT = 228,           //
	MSH_B_TIBT_STORAGE_PIT = 229,           //
	MSH_B_WOOD_00 = 230,                    //
	MSH_B_WOOD_01 = 231,                    //
	MSH_B_WOOD_02 = 232,                    //
	MSH_B_WOOD_03 = 233,                    //
	MSH_B_WOOD_04 = 234,                    //
	MSH_B_WOOD_OUTSIDE = 235,               //
	MSH_B_TIBT_1 = 236,                     //
	MSH_B_TIBT_2 = 237,                     //
	MSH_B_TIBT_3 = 238,                     //
	MSH_B_TIBT_3_A = 239,                   //
	MSH_B_TIBT_4 = 240,                     //
	MSH_B_TIBT_5 = 241,                     //
	MSH_B_TIBT_5_A = 242,                   //
	MSH_B_TIBT_CRECHE = 243,                //
	MSH_B_TIBT_GRAVEYARD = 244,             //
	MSH_B_TIBT_LARGE_PILLER = 245,          //
	MSH_B_TIBT_MINI_TEMPLE = 246,           //
	MSH_B_TIBT_TOTEM = 247,                 //
	MSH_B_TIBT_WONDER = 248,                //
	MSH_B_TIBT_WORKSHOP = 249,              //
	MSH_B_SPELLHAND = 250,                  //
	MSH_B_WORSHIPGRAIN = 251,               //
	MSH_B_WORSHIPSITE_01_NEUTRAL = 252,     //
	MSH_BOULDER1_CHALK = 253,               //
	MSH_BOULDER1_LIME = 254,                //
	MSH_BOULDER1_SAND = 255,                //
	MSH_BOULDER1_VOLCANIC = 256,            //
	MSH_BOULDER2_CHALK = 257,               //
	MSH_BOULDER2_LIME = 258,                //
	MSH_BOULDER2_SAND = 259,                //
	MSH_BOULDER2_VOLCANIC = 260,            //
	MSH_BOULDER3_CHALK = 261,               //
	MSH_BOULDER3_LIME = 262,                //
	MSH_BOULDER3_SAND = 263,                //
	MSH_BOULDER3_VOLCANIC = 264,            //
	MSH_FAT_PILAR_CHALK = 265,              //
	MSH_FAT_PILAR_LIME = 266,               //
	MSH_FAT_PILAR_SAND = 267,               //
	MSH_FAT_PILAR_VOLCANIC = 268,           //
	MSH_F_CREATURE_CAVE = 269,              //
	MSH_F_FOREST = 270,                     //
	MSH_F_FOREST01 = 271,                   //
	MSH_F_FOREST02 = 272,                   //
	MSH_F_FOREST03 = 273,                   //
	MSH_F_FOREST_BOOK01 = 274,              //
	MSH_F_GIANTSKELETON = 275,              //
	MSH_F_GIANTSKULL = 276,                 //
	MSH_F_IDOL = 277,                       //
	MSH_F_PIPER_CAVE = 278,                 //
	MSH_F_RIB1 = 279,                       //
	MSH_F_RIB2 = 280,                       //
	MSH_F_SPOUTHOLE = 281,                  //
	MSH_F_VERTABRAE = 282,                  //
	MSH_PILAR2_CHALK = 283,                 //
	MSH_PILAR2_LIME = 284,                  //
	MSH_PILAR2_SAND = 285,                  //
	MSH_PILAR2_VOLCANIC = 286,              //
	MSH_PILAR3_CHALK = 287,                 //
	MSH_PILAR3_LIME = 288,                  //
	MSH_PILAR3_SAND = 289,                  //
	MSH_PILAR3_VOLCANIC = 290,              //
	MSH_PILAR_CHALK = 291,                  //
	MSH_PILAR_LIME = 292,                   //
	MSH_PILAR_SAND = 293,                   //
	MSH_PILAR_VOLCANIC = 294,               //
	MSH_PUZZLE_1 = 295,                     //
	MSH_PUZZLE_2 = 296,                     //
	MSH_PUZZLE_3 = 297,                     //
	MSH_PUZZLE_4 = 298,                     //
	MSH_PUZZLE_5 = 299,                     //
	MSH_SPIKEY_PILAR_CHALK = 300,           //
	MSH_SPIKEY_PILAR_LIME = 301,            //
	MSH_SPIKEY_PILAR_SAND = 302,            //
	MSH_SPIKEY_PILAR_VOLCANIC = 303,        //
	MSH_Z_PHONEBOX = 304,                   //
	MSH_I_BLAST = 305,                      //
	MSH_I_FIREBALL = 306,                   //
	MSH_I_FOOD = 307,                       //
	MSH_I_FOOTBALL = 308,                   //
	MSH_I_FOREST = 309,                     //
	MSH_I_HEAL = 310,                       //
	MSH_I_LIGHTNING = 311,                  //
	MSH_I_LIGHTNING2 = 312,                 //
	MSH_I_MINISCROLL = 313,                 //
	MSH_I_MINISCROLL_ACTIVE = 314,          //
	MSH_I_MINISCROLL_BRONZE = 315,          //
	MSH_I_MINISCROLL_BRONZE_ACTIVE = 316,   //
	MSH_I_MINISCROLL_GOLD = 317,            //
	MSH_I_MINISCROLL_GOLD_ACTIVE = 318,     //
	MSH_I_MINISCROLL_SILVER = 319,          //
	MSH_I_MINISCROLL_SILVER_ACTIVE = 320,   //
	MSH_I_OFFERING_BEDROLL = 321,           //
	MSH_I_OFFERING_FERTILITY = 322,         //
	MSH_I_OFFERING_FOOD = 323,              //
	MSH_I_OFFERING_PLAY = 324,              //
	MSH_I_OFFERING_PROTECTION = 325,        //
	MSH_I_OFFERING_SKULL = 326,             //
	MSH_I_OFFERING_VILLAGE_IMPROVE = 327,   //
	MSH_I_OFFERING_WATER = 328,             //
	MSH_I_OFFERING_WOOD = 329,              //
	MSH_I_PRIEST = 330,                     //
	MSH_I_SHIELD = 331,                     //
	MSH_I_SKELETON = 332,                   //
	MSH_I_SPELLGEM = 333,                   //
	MSH_I_TREES = 334,                      //
	MSH_I_WALL = 335,                       //
	MSH_GATE_TOTEM_PLINTHE_PHYS_1 = 336,    //
	MSH_GATE_TOTEM_PLINTHE_PHYS_2 = 337,    //
	MSH_GATE_TOTEM_PLINTHE_PHYS_3 = 338,    //
	MSH_O_ARK = 339,                        //
	MSH_O_ARK_DRY_DOCK = 340,               //
	MSH_O_ARK_WRECK = 341,                  //
	MSH_O_AXE = 342,                        //
	MSH_O_BAG = 343,                        //
	MSH_O_BALL = 344,                       //
	MSH_O_BELLTOWER = 345,                  //
	MSH_O_BOX_FRAME = 346,                  //
	MSH_O_BRANCH_EVERGREEN_INHAND = 347,    //
	MSH_O_BRANCH_FRUIT_INHAND = 348,        //
	MSH_O_BRANCH_HARDWOOD_INHAND = 349,     //
	MSH_O_BURNT_TREE = 350,                 //
	MSH_O_CAULDRON = 351,                   //
	MSH_O_CHAMPI = 352,                     //
	MSH_O_CREATURE_TURD = 353,              //
	MSH_O_CROOK = 354,                      //
	MSH_O_FISHINGROD = 355,                 //
	MSH_O_FLATROCK_CHALK = 356,             //
	MSH_O_FLATROCK_LIMESTONE = 357,         //
	MSH_O_FLATROCK_SANDSTONE = 358,         //
	MSH_O_FLATROCK_VOLCANIC = 359,          //
	MSH_O_FLOWER_BED_1 = 360,               //
	MSH_O_FLOWER_BED_2 = 361,               //
	MSH_O_GATE_TOTEM_APE = 362,             //
	MSH_O_GATE_TOTEM_BLANK = 363,           //
	MSH_O_GATE_TOTEM_COW = 364,             //
	MSH_O_GATE_TOTEM_PLINTHE = 365,         //
	MSH_O_GATE_TOTEM_TIGER = 366,           //
	MSH_O_HAMMER = 367,                     //
	MSH_O_INFO_SIGN = 368,                  //
	MSH_O_JAPN_STONETHING = 369,            //
	MSH_O_LARGE_FISH_DEAD = 370,            //
	MSH_O_LIGHT_BULB = 371,                 //
	MSH_O_LOGS_INHAND = 372,                //
	MSH_O_LONGROCK_CHALK = 373,             //
	MSH_O_LONGROCK_LIMESTONE = 374,         //
	MSH_O_LONGROCK_SANDSTONE = 375,         //
	MSH_O_LONGROCK_VOLCANIC = 376,          //
	MSH_O_MAGIC_MUSHROOM = 377,             //
	MSH_O_MALLET_HEAVY = 378,               //
	MSH_O_ROCK_CHALK = 379,                 //
	MSH_O_ROCK_LIMESTONE = 380,             //
	MSH_O_ROCK_SANDSTONE = 381,             //
	MSH_O_ROCK_VOLCANIC = 382,              //
	MSH_O_SAW = 383,                        //
	MSH_O_SCYTHE = 384,                     //
	MSH_O_SHARPROCK_CHALK = 385,            //
	MSH_O_SHARPROCK_LIMESTONE = 386,        //
	MSH_O_SHARPROCK_SANDSTONE = 387,        //
	MSH_O_SHARPROCK_VOLCANIC = 388,         //
	MSH_O_SKULL_TOTEM = 389,                //
	MSH_O_SPADE = 390,                      //
	MSH_O_SQUAREROCK_CHALK = 391,           //
	MSH_O_SQUAREROCK_LIMESTONE = 392,       //
	MSH_O_SQUAREROCK_SANDSTONE = 393,       //
	MSH_O_SQUAREROCK_VOLCANIC = 394,        //
	MSH_O_TICKERTAPE = 395,                 //
	MSH_O_TOADSTOOL = 396,                  //
	MSH_O_TOMBSTONE = 397,                  //
	MSH_O_TOWNLIGHT = 398,                  //
	MSH_O_TOY_BALL = 399,                   //
	MSH_O_TOY_BOWLING_BALL = 400,           //
	MSH_O_TOY_CUDDLY = 401,                 //
	MSH_O_TOY_DIE = 402,                    //
	MSH_O_TOY_SKITTLE = 403,                //
	MSH_O_WHEAT_ENCLOSURE = 404,            //
	MSH_O_WHEAT_INHAND = 405,               //
	MSH_O_WOOD_INHAND = 406,                //
	MSH_REWARD_CHEST_EXPLODE = 407,         //
	MSH_SCULPTURE_IN_PROGRESS = 408,        //
	MSH_SINGING_STONE_CENTER = 409,         //
	MSH_SINGING_STONE = 410,                //
	MSH_IDOL_BUILDER = 411,                 //
	MSH_NEW_SHAOLIN = 412,                  //
	MSH_P_AMCN_F_A_1 = 413,                 //
	MSH_P_AMCN_F_A_2 = 414,                 //
	MSH_P_AMCN_F_A_3 = 415,                 //
	MSH_P_AMCN_M_A_1 = 416,                 //
	MSH_P_AMCN_M_A_2 = 417,                 //
	MSH_P_AMCN_M_A_3 = 418,                 //
	MSH_P_ANIMAL_BREEDER = 419,             //
	MSH_P_ANIMAL_TRAINER = 420,             //
	MSH_P_AZTC_F_A_1 = 421,                 //
	MSH_P_AZTC_F_A_2 = 422,                 //
	MSH_P_AZTC_F_A_3 = 423,                 //
	MSH_P_AZTC_LEADER = 424,                //
	MSH_P_AZTC_M_A_1 = 425,                 //
	MSH_P_AZTC_M_A_2 = 426,                 //
	MSH_P_AZTC_M_A_3 = 427,                 //
	MSH_P_BANDIT = 428,                     //
	MSH_P_BOY_BLACK_1 = 429,                //
	MSH_P_BOY_BLACK_2 = 430,                //
	MSH_P_BOY_BLACK_3 = 431,                //
	MSH_P_BOY_ORIENTAL_1 = 432,             //
	MSH_P_BOY_ORIENTAL_2 = 433,             //
	MSH_P_BOY_ORIENTAL_3 = 434,             //
	MSH_P_BOY_TAN_1 = 435,                  //
	MSH_P_BOY_TAN_2 = 436,                  //
	MSH_P_BOY_TAN_3 = 437,                  //
	MSH_P_BOY_WHITE = 438,                  //
	MSH_P_BOY_WHITE_1 = 439,                //
	MSH_P_BOY_WHITE_2 = 440,                //
	MSH_P_BOY_WHITE_3 = 441,                //
	MSH_P_CELT_FOOTBALLER = 442,            //
	MSH_P_CELT_F_A_1 = 443,                 //
	MSH_P_CELT_F_A_2 = 444,                 //
	MSH_P_CELT_F_A_3 = 445,                 //
	MSH_P_CELT_M_A_1 = 446,                 //
	MSH_P_CELT_M_A_2 = 447,                 //
	MSH_P_CELT_M_A_3 = 448,                 //
	MSH_P_CRUSADER = 449,                   //
	MSH_P_CULT_PRIEST = 450,                //
	MSH_P_CULT_PRIESTESS = 451,             //
	MSH_P_EGPT_F_A_1 = 452,                 //
	MSH_P_EGPT_F_A_2 = 453,                 //
	MSH_P_EGPT_F_A_3 = 454,                 //
	MSH_P_EGPT_M_A_1 = 455,                 //
	MSH_P_EGPT_M_A_2 = 456,                 //
	MSH_P_EGPT_M_A_3 = 457,                 //
	MSH_P_EGPT_M_B_1 = 458,                 //
	MSH_P_EGPT_M_B_2 = 459,                 //
	MSH_P_ENGINEER = 460,                   //
	MSH_P_GIRL_BLACK_1 = 461,               //
	MSH_P_GIRL_BLACK_2 = 462,               //
	MSH_P_GIRL_BLACK_3 = 463,               //
	MSH_P_GIRL_ORIENTAL_1 = 464,            //
	MSH_P_GIRL_ORIENTAL_2 = 465,            //
	MSH_P_GIRL_ORIENTAL_3 = 466,            //
	MSH_P_GIRL_TAN_1 = 467,                 //
	MSH_P_GIRL_TAN_2 = 468,                 //
	MSH_P_GIRL_TAN_3 = 469,                 //
	MSH_P_GIRL_WHITE = 470,                 //
	MSH_P_GIRL_WHITE_1 = 471,               //
	MSH_P_GIRL_WHITE_2 = 472,               //
	MSH_P_GIRL_WHITE_3 = 473,               //
	MSH_P_GREK_F_A_1 = 474,                 //
	MSH_P_GREK_F_A_2 = 475,                 //
	MSH_P_GREK_F_A_3 = 476,                 //
	MSH_P_GREK_M_A_1 = 477,                 //
	MSH_P_GREK_M_A_2 = 478,                 //
	MSH_P_GREK_M_A_3 = 479,                 //
	MSH_P_HERMIT = 480,                     //
	MSH_P_HIPPY = 481,                      //
	MSH_P_IDOL_BUILDER = 482,               //
	MSH_P_INTRO_F = 483,                    //
	MSH_P_INTRO_M = 484,                    //
	MSH_P_JAPANESE_F_A_2 = 485,             //
	MSH_P_JAPANESE_F_A_3 = 486,             //
	MSH_P_JAPANESE_M_A_1 = 487,             //
	MSH_P_JAPANESE_M_A_2 = 488,             //
	MSH_P_JAPANESE_M_A_3 = 489,             //
	MSH_P_JAPN_F_A_1 = 490,                 //
	MSH_P_JAPN_F_A_2 = 491,                 //
	MSH_P_JAPN_F_A_3 = 492,                 //
	MSH_P_JAPN_M_A_1 = 493,                 //
	MSH_P_JAPN_M_A_2 = 494,                 //
	MSH_P_JAPN_M_A_3 = 495,                 //
	MSH_P_NOMAD = 496,                      //
	MSH_P_NORS_FOOTBALLER = 497,            //
	MSH_P_NORS_F_A_1 = 498,                 //
	MSH_P_NORS_F_A_2 = 499,                 //
	MSH_P_NORS_F_A_3 = 500,                 //
	MSH_P_NORS_M_A_1 = 501,                 //
	MSH_P_NORS_M_A_2 = 502,                 //
	MSH_P_NORS_M_A_3 = 503,                 //
	MSH_P_NORS_SAILOR = 504,                //
	MSH_P_PIED_PIPER = 505,                 //
	MSH_P_SAILOR_ACCORDIAN = 506,           //
	MSH_P_SCULPTOR = 507,                   //
	MSH_P_SHAOLIN_MONK = 508,               //
	MSH_P_SHEPERD = 509,                    //
	MSH_P_SKELETON_F = 510,                 //
	MSH_P_SKELETON_M = 511,                 //
	MSH_P_TIBETAN_F_A_2 = 512,              //
	MSH_P_TIBETAN_F_A_3 = 513,              //
	MSH_P_TIBETAN_M_A_1 = 514,              //
	MSH_P_TIBETAN_M_A_2 = 515,              //
	MSH_P_TIBETAN_M_A_3 = 516,              //
	MSH_P_TIBT_F_A_1 = 517,                 //
	MSH_P_TIBT_F_A_2 = 518,                 //
	MSH_P_TIBT_F_A_3 = 519,                 //
	MSH_P_TIBT_M_A_1 = 520,                 //
	MSH_P_TIBT_M_A_2 = 521,                 //
	MSH_P_TIBT_M_A_3 = 522,                 //
	MSH_P_TIBT_M_B_1 = 523,                 //
	MSH_P_TIBT_M_B_2 = 524,                 //
	MSH_CREED = 525,                        //
	MSH_S_BLAST_CONE = 526,                 //
	MSH_S_BLAST_CENTRE = 527,               //
	MSH_S_BLAST_INNER = 528,                //
	MSH_S_BLAST_OUTER = 529,                //
	MSH_S_COLUMN = 530,                     //
	MSH_S_FIREBALL_TAIL = 531,              //
	MSH_S_HEAL_MESH = 532,                  //
	MSH_S_GRAIN_PILE = 533,                 //
	MSH_S_HORN_OF_PLENTY = 534,             //
	MSH_S_PHILE = 535,                      //
	MSH_S_PHILE_ANGRY = 536,                //
	MSH_S_PHILE_BIG = 537,                  //
	MSH_S_PHILE_FAT = 538,                  //
	MSH_S_PHILE_FREEZE = 539,               //
	MSH_S_PHILE_FRIGHTEN = 540,             //
	MSH_S_PHILE_HUNGRY = 541,               //
	MSH_S_PHILE_ILL = 542,                  //
	MSH_S_PHILE_INVISIBLE = 543,            //
	MSH_S_PHILE_ITCHY = 544,                //
	MSH_S_PHILE_LOVE = 545,                 //
	MSH_S_PHILE_SMALL = 546,                //
	MSH_S_PHILE_STRONG = 547,               //
	MSH_S_PHILE_THIN = 548,                 //
	MSH_S_PHILE_THIRSTY = 549,              //
	MSH_S_PHILE_TIRED = 550,                //
	MSH_S_PHILE_WEAK = 551,                 //
	MSH_S_RAIN_CONE = 552,                  //
	MSH_S_SHIELDMAGICLOWPOLY = 553,         //
	MSH_S_SOLID_SHIELD = 554,               //
	MSH_S_SPELLBALLSURFACE02 = 555,         //
	MSH_S_SKULL_ICON = 556,                 //
	MSH_S_SPELL_CREATOR = 557,              //
	MSH_S_SPELL_DISPENSER = 558,            //
	MSH_S_VORTEXCYLINDER = 559,             //
	MSH_S_ZCHEATBOX = 560,                  //
	MSH_S_PULSE_IN = 561,                   //
	MSH_S_PULSE_OUT = 562,                  //
	MSH_A_WORM = 563,                       //
	MSH_B_DUMMY_CITADEL = 564,              //
	MSH_Z_APPLE_BARREL = 565,               //
	MSH_Z_ARROW = 566,                      //
	MSH_Z_SPELLROCK01 = 567,                //
	MSH_Z_SPELLROCK02 = 568,                //
	MSH_T_COPSE = 569,                      //
	MSH_T_COPSE_A = 570,                    //
	MSH_T_BEECH = 571,                      //
	MSH_T_BIRCH = 572,                      //
	MSH_T_BUSH = 573,                       //
	MSH_T_BUSH_A = 574,                     //
	MSH_T_BUSH_B = 575,                     //
	MSH_T_CEDAR = 576,                      //
	MSH_T_CONIFER = 577,                    //
	MSH_T_CONIFER_A = 578,                  //
	MSH_T_CYPRESS = 579,                    //
	MSH_T_CYPRESS_A = 580,                  //
	MSH_T_HEDGE = 581,                      //
	MSH_T_HEDGE_A = 582,                    //
	MSH_T_OAK = 583,                        //
	MSH_T_OAK_A = 584,                      //
	MSH_T_OLIVE = 585,                      //
	MSH_T_PALM = 586,                       //
	MSH_T_PALM_A = 587,                     //
	MSH_T_PALM_B = 588,                     //
	MSH_T_PALM_C = 589,                     //
	MSH_T_PINE = 590,                       //
	MSH_T_RICE = 591,                       //
	MSH_T_ROOTS = 592,                      //
	MSH_T_ROOTS_PILE = 593,                 //
	MSH_T_WHEAT = 594,                      //
	MSH_T_WHEAT_INFIELD = 595,              //
	MSH_U_AXE = 596,                        //
	MSH_U_BAG = 597,                        //
	MSH_U_BALL = 598,                       //
	MSH_U_BENCH = 599,                      //
	MSH_U_BUCKET = 600,                     //
	MSH_U_BUCKET02 = 601,                   //
	MSH_U_CART = 602,                       //
	MSH_U_CHEST = 603,                      //
	MSH_U_CHEST_TOP = 604,                  //
	MSH_U_CROOK = 605,                      //
	MSH_U_FISHING_ROD = 606,                //
	MSH_U_FRAME = 607,                      //
	MSH_U_FRAME_SKIN = 608,                 //
	MSH_U_HAMMER = 609,                     //
	MSH_U_MALLET_HEAVY = 610,               //
	MSH_U_SAW = 611,                        //
	MSH_U_SCYTHE = 612,                     //
	MSH_U_SPADE = 613,                      //
	MSH_U_STOOL = 614,                      //
	MSH_U_STOOL01 = 615,                    //
	MSH_U_TABLE = 616,                      //
	MSH_U_TROUGH = 617,                     //
	MSH_U_WASHING_LINE_AMCN = 618,          //
	MSH_U_WASHING_LINE_AZTC = 619,          //
	MSH_U_WASHING_LINE_CELT = 620,          //
	MSH_U_WASHING_LINE_EGPT = 621,          //
	MSH_U_WASHING_LINE_GREK = 622,          //
	MSH_U_WASHING_LINE_JAPN = 623,          //
	MSH_U_WASHING_LINE_NORS = 624,          //
	MSH_U_WASHING_LINE_TIBT = 625,          //
	MAX_COUNT_3D_MESHES = 626,              //
};

enum ANIM_LIST
{
	ANM_DONT_DRAW = -4,                          //
	ANM_NO_MOVE_ANIM_SET = -3,                   //
	ANM_NO_ANIM_SET = -2,                        //
	ANM_INVALID = -1,                            //
	ANM_BAT_ERATICGLIDE = 0,                     //
	ANM_BAT_FLAP = 1,                            //
	ANM_BAT_GLIDE = 2,                           //
	ANM_CROW_FLAP = 3,                           //
	ANM_CROW_GLIDE = 4,                          //
	ANM_CROW_LANDING = 5,                        //
	ANM_CROW_SIT = 6,                            //
	ANM_CROW_TAKEOFF = 7,                        //
	ANM_DOVE_FLAP = 8,                           //
	ANM_DOVE_GLIDE = 9,                          //
	ANM_DOVE_LANDING = 10,                       //
	ANM_DOVE_SIT = 11,                           //
	ANM_DOVE_TAKEOFF = 12,                       //
	ANM_PIGEON_FLAP = 13,                        //
	ANM_PIGEON_GLIDE = 14,                       //
	ANM_PIGEON_LANDING = 15,                     //
	ANM_PIGEON_SIT = 16,                         //
	ANM_PIGEON_TAKEOFF = 17,                     //
	ANM_SEAGULL_FLAP = 18,                       //
	ANM_SEAGULL_GENTLEFLAP = 19,                 //
	ANM_SEAGULL_GLIDE = 20,                      //
	ANM_SEAGULL_LANDING = 21,                    //
	ANM_SEAGULL_TAKEOFF = 22,                    //
	ANM_SPELL_DOVE_FLAP = 23,                    //
	ANM_SPELL_DOVE_GLIDE = 24,                   //
	ANM_SWALLOW_CALMGLIDE = 25,                  //
	ANM_SWALLOW_ERATICGLIDE = 26,                //
	ANM_SWALLOW_FLAP = 27,                       //
	ANM_A_COW_DEAD = 28,                         //
	ANM_A_COW_DEAD_ON_LHS = 29,                  //
	ANM_A_COW_DEAD_ON_RHS = 30,                  //
	ANM_A_COW_DIE = 31,                          //
	ANM_A_COW_DOWNED_ON_LHS = 32,                //
	ANM_A_COW_DOWNED_ON_RHS = 33,                //
	ANM_A_COW_DROWNING = 34,                     //
	ANM_A_COW_EAT_1 = 35,                        //
	ANM_A_COW_EAT_2 = 36,                        //
	ANM_A_COW_GOTO_EAT = 37,                     //
	ANM_A_COW_IN_HAND = 38,                      //
	ANM_A_COW_LANDED_LEFT_SIDE = 39,             //
	ANM_A_COW_LANDED_RIGHT_SIDE = 40,            //
	ANM_A_COW_RUN = 41,                          //
	ANM_A_COW_STAND = 42,                        //
	ANM_A_COW_THROWN = 43,                       //
	ANM_A_COW_UPFROM_EAT = 44,                   //
	ANM_A_COW_WALK = 45,                         //
	ANM_A_HORSE_DEADLHS = 46,                    //
	ANM_A_HORSE_DEADRHS = 47,                    //
	ANM_A_HORSE_EAT1 = 48,                       //
	ANM_A_HORSE_EAT2 = 49,                       //
	ANM_A_HORSE_FALL_DEADLHS = 50,               //
	ANM_A_HORSE_FALL_DEADRHS = 51,               //
	ANM_A_HORSE_GOTO_EAT = 52,                   //
	ANM_A_HORSE_IN_HAND = 53,                    //
	ANM_A_HORSE_LAND_GET_UPLHS = 54,             //
	ANM_A_HORSE_LAND_GET_UPRHS = 55,             //
	ANM_A_HORSE_RUN = 56,                        //
	ANM_A_HORSE_STAND = 57,                      //
	ANM_A_HORSE_THROWN = 58,                     //
	ANM_A_HORSE_TROT = 59,                       //
	ANM_A_HORSE_UP_FROM_EAT = 60,                //
	ANM_A_HORSE_WALK = 61,                       //
	ANM_A_LEOPARD_DEAD = 62,                     //
	ANM_A_LEOPARD_DEAD_ON_LEFT = 63,             //
	ANM_A_LEOPARD_DEAD_ON_RIGHT = 64,            //
	ANM_A_LEOPARD_DIE = 65,                      //
	ANM_A_LEOPARD_EAT = 66,                      //
	ANM_A_LEOPARD_GET_UP_FROM_LEFT = 67,         //
	ANM_A_LEOPARD_GET_UP_FROM_RIGHT = 68,        //
	ANM_A_LEOPARD_GOTO_EAT = 69,                 //
	ANM_A_LEOPARD_GOTO_SIT = 70,                 //
	ANM_A_LEOPARD_GOTO_SLEEP = 71,               //
	ANM_A_LEOPARD_IN_HAND = 72,                  //
	ANM_A_LEOPARD_POUNCE = 73,                   //
	ANM_A_LEOPARD_POUNCE_HI = 74,                //
	ANM_A_LEOPARD_POUNCE_LO = 75,                //
	ANM_A_LEOPARD_PROWL = 76,                    //
	ANM_A_LEOPARD_RUN = 77,                      //
	ANM_A_LEOPARD_SIT = 78,                      //
	ANM_A_LEOPARD_SLEEP = 79,                    //
	ANM_A_LEOPARD_STAND = 80,                    //
	ANM_A_LEOPARD_THROWN = 81,                   //
	ANM_A_LEOPARD_UPFROM_EAT = 82,               //
	ANM_A_LEOPARD_UPFROM_SIT = 83,               //
	ANM_A_LEOPARD_UPFROM_SLEEP = 84,             //
	ANM_A_LEOPARD_WALK = 85,                     //
	ANM_A_LION_DEAD = 86,                        //
	ANM_A_LION_DEAD_ON_LEFT = 87,                //
	ANM_A_LION_DEAD_ON_RIGHT = 88,               //
	ANM_A_LION_DIE = 89,                         //
	ANM_A_LION_DOWNED_ON_LHS = 90,               //
	ANM_A_LION_DOWNED_ON_RHS = 91,               //
	ANM_A_LION_EAT = 92,                         //
	ANM_A_LION_GET_UP_FROM_LEFT = 93,            //
	ANM_A_LION_GET_UP_FROM_RIGHT = 94,           //
	ANM_A_LION_GOTO_EAT = 95,                    //
	ANM_A_LION_GOTO_SIT = 96,                    //
	ANM_A_LION_GOTO_SLEEP = 97,                  //
	ANM_A_LION_IN_HAND = 98,                     //
	ANM_A_LION_POUNCE = 99,                      //
	ANM_A_LION_POUNCE_HI = 100,                  //
	ANM_A_LION_POUNCE_LO = 101,                  //
	ANM_A_LION_PROWL = 102,                      //
	ANM_A_LION_RUN = 103,                        //
	ANM_A_LION_SIT = 104,                        //
	ANM_A_LION_SLEEP = 105,                      //
	ANM_A_LION_STAND = 106,                      //
	ANM_A_LION_THROWN = 107,                     //
	ANM_A_LION_TURN_LEFT_90_DEGREES = 108,       //
	ANM_A_LION_TURN_RIGHT_90_DEGREES = 109,      //
	ANM_A_LION_UPFROM_EAT = 110,                 //
	ANM_A_LION_UPFROM_SIT = 111,                 //
	ANM_A_LION_UPFROM_SLEEP = 112,               //
	ANM_A_LION_WALK = 113,                       //
	ANM_A_PIG_DEADLHS = 114,                     //
	ANM_A_PIG_DEADRHS = 115,                     //
	ANM_A_PIG_EAT1 = 116,                        //
	ANM_A_PIG_EAT2 = 117,                        //
	ANM_A_PIG_FALL_DEADLHS = 118,                //
	ANM_A_PIG_FALL_DEADRHS = 119,                //
	ANM_A_PIG_GOTO_EAT = 120,                    //
	ANM_A_PIG_IN_HAND = 121,                     //
	ANM_A_PIG_LAND_GET_UPLHS = 122,              //
	ANM_A_PIG_LAND_GET_UPRHS = 123,              //
	ANM_A_PIG_OUT_OF_EAT = 124,                  //
	ANM_A_PIG_RUN = 125,                         //
	ANM_A_PIG_STAND = 126,                       //
	ANM_A_PIG_THROWN = 127,                      //
	ANM_A_PIG_WALK = 128,                        //
	ANM_SHARK_BONED_SWIM = 129,                  //
	ANM_A_SHEEP_DEAD_LHS = 130,                  //
	ANM_A_SHEEP_DEAD_RHS = 131,                  //
	ANM_A_SHEEP_EAT1 = 132,                      //
	ANM_A_SHEEP_EAT2 = 133,                      //
	ANM_A_SHEEP_FALL_DEADLHS = 134,              //
	ANM_A_SHEEP_FALL_DEADRHS = 135,              //
	ANM_A_SHEEP_GOTO_EAT = 136,                  //
	ANM_A_SHEEP_IN_HAND = 137,                   //
	ANM_A_SHEEP_LANDED_GETUPLHS = 138,           //
	ANM_A_SHEEP_LANDED_GETUPRHS = 139,           //
	ANM_A_SHEEP_OUT_OF_EAT = 140,                //
	ANM_A_SHEEP_RUN = 141,                       //
	ANM_A_SHEEP_STAND = 142,                     //
	ANM_A_SHEEP_THROWN = 143,                    //
	ANM_A_SHEEP_TROT = 144,                      //
	ANM_A_SHEEP_WALK = 145,                      //
	ANM_A_TIGER_DEAD = 146,                      //
	ANM_A_TIGER_DEAD_ON_LEFT = 147,              //
	ANM_A_TIGER_DEAD_ON_RIGHT = 148,             //
	ANM_A_TIGER_DIE = 149,                       //
	ANM_A_TIGER_EAT = 150,                       //
	ANM_A_TIGER_GET_UP_FROM_LEFT = 151,          //
	ANM_A_TIGER_GET_UP_FROM_RIGHT = 152,         //
	ANM_A_TIGER_GOTO_EAT = 153,                  //
	ANM_A_TIGER_GOTO_SIT = 154,                  //
	ANM_A_TIGER_GOTO_SLEEP = 155,                //
	ANM_A_TIGER_IN_HAND = 156,                   //
	ANM_A_TIGER_POUNCE = 157,                    //
	ANM_A_TIGER_POUNCE_HI = 158,                 //
	ANM_A_TIGER_POUNCE_LO = 159,                 //
	ANM_A_TIGER_PROWL = 160,                     //
	ANM_A_TIGER_RUN = 161,                       //
	ANM_A_TIGER_SIT = 162,                       //
	ANM_A_TIGER_SLEEP = 163,                     //
	ANM_A_TIGER_STAND = 164,                     //
	ANM_A_TIGER_THROWN = 165,                    //
	ANM_A_TIGER_UPFROM_EAT = 166,                //
	ANM_A_TIGER_UPFROM_SIT = 167,                //
	ANM_A_TIGER_UPFROM_SLEEP = 168,              //
	ANM_A_TIGER_WALK = 169,                      //
	ANM_A_TORTOISE_DEAD = 170,                   //
	ANM_A_TORTOISE_STAND = 171,                  //
	ANM_A_TORTOISE_WALK = 172,                   //
	ANM_A_WOLF_DIE = 173,                        //
	ANM_A_WOLF_EAT = 174,                        //
	ANM_A_WOLF_GOTO_EAT = 175,                   //
	ANM_A_WOLF_GOTO_SIT = 176,                   //
	ANM_A_WOLF_GOTO_SLEEP = 177,                 //
	ANM_A_WOLF_HOWL = 178,                       //
	ANM_A_WOLF_POUNCE = 179,                     //
	ANM_A_WOLF_PROWL = 180,                      //
	ANM_A_WOLF_RUN = 181,                        //
	ANM_A_WOLF_SIT = 182,                        //
	ANM_A_WOLF_SLEEP = 183,                      //
	ANM_A_WOLF_STAND = 184,                      //
	ANM_A_WOLF_UPFROM_EAT = 185,                 //
	ANM_A_WOLF_UPFROM_SIT = 186,                 //
	ANM_A_WOLF_UPFROM_SLEEP = 187,               //
	ANM_A_WOLF_WALK = 188,                       //
	ANM_MINE_ENTRANCE = 189,                     //
	ANM_PLAYER_ICON01_PLINTH = 190,              //
	ANM_NORS_GATE = 191,                         //
	ANM_PHONEBOX = 192,                          //
	ANM_PIPER_CAVE_CLOSING = 193,                //
	ANM_PIPER_CAVE_OPENING = 194,                //
	ANM_GATE_TOTEM_PLINTHE = 195,                //
	ANM_PIED_PIPER_DANCE_STAND = 196,            //
	ANM_PIED_PIPER_DANCE_STAND_SPIN = 197,       //
	ANM_PIED_PIPER_DANCE_WALK = 198,             //
	ANM_PIED_PIPER_DANCE_WALK_SPIN = 199,        //
	ANM_PIED_PIPER_SUMMON = 200,                 //
	ANM_PIED_PIPER_WALK = 201,                   //
	ANM_P_AFRICAN_DRUMMER = 202,                 //
	ANM_P_AMBIENT1 = 203,                        //
	ANM_P_AMBIENT2 = 204,                        //
	ANM_P_ARSONIST = 205,                        //
	ANM_P_ATTACKED_BY_LION = 206,                //
	ANM_P_ATTRACT_YOUR_ATTENTION = 207,          //
	ANM_P_AVOID_OBJECT_DIVE = 208,               //
	ANM_P_AVOID_OBJECT_DODGE = 209,              //
	ANM_P_BECKON = 210,                          //
	ANM_P_BLOW_RASPBERRY = 211,                  //
	ANM_P_CARRYING_BAG = 212,                    //
	ANM_P_CARRYING_POT = 213,                    //
	ANM_P_CARRYING_STICKS = 214,                 //
	ANM_P_CARRY_AXE = 215,                       //
	ANM_P_CARRY_OBJECT_RUN = 216,                //
	ANM_P_CHOPPING_TREE = 217,                   //
	ANM_P_CLAP_1 = 218,                          //
	ANM_P_CLAP_2 = 219,                          //
	ANM_P_CLAP_3 = 220,                          //
	ANM_P_CLAP_4 = 221,                          //
	ANM_P_CONDUCT_MEETING = 222,                 //
	ANM_P_COUPLE_KISS_MAN = 223,                 //
	ANM_P_COUPLE_KISS_WOMAN = 224,               //
	ANM_P_CRAWL_INJURED = 225,                   //
	ANM_P_CRAWL_INJURED_INTO = 226,              //
	ANM_P_CROWD_IMPRESSED_1 = 227,               //
	ANM_P_CROWD_IMPRESSED_2 = 228,               //
	ANM_P_CROWD_LOST = 229,                      //
	ANM_P_CROWD_LOST_2 = 230,                    //
	ANM_P_CROWD_MEXICAN_WAVE = 231,              //
	ANM_P_CROWD_UNIMPRESSED_1 = 232,             //
	ANM_P_CROWD_UNIMPRESSED_2 = 233,             //
	ANM_P_CROWD_WON = 234,                       //
	ANM_P_CROWD_WON_2 = 235,                     //
	ANM_P_DANCE_PISS_TAKE_ = 236,                //
	ANM_P_DANCE_STAND = 237,                     //
	ANM_P_DANCE_TOGETHER_ARMLINK = 238,          //
	ANM_P_DANCE_TOGETHER_CIRCLING = 239,         //
	ANM_P_DANCE_TOGETHER_CIRCLING_PARTNER = 240, //
	ANM_P_DANCE_X = 241,                         //
	ANM_P_DANCE_X_PARTNER = 242,                 //
	ANM_P_DEAD1 = 243,                           //
	ANM_P_DEAD1_GOTO_HEAVEN = 244,               //
	ANM_P_DEAD1_GOTO_HELL = 245,                 //
	ANM_P_DEAD2 = 246,                           //
	ANM_P_DEAD2_GOTO_HEAVEN = 247,               //
	ANM_P_DEAD2_GOTO_HELL = 248,                 //
	ANM_P_DEAD_DROWNED = 249,                    //
	ANM_P_DEAD_IN_HAND = 250,                    //
	ANM_P_DRIBBLE_BALL = 251,                    //
	ANM_P_DROWNING = 252,                        //
	ANM_P_DYING = 253,                           //
	ANM_P_EAT = 254,                             //
	ANM_P_FAKE_FOUL_FALL = 255,                  //
	ANM_P_FARMER = 256,                          //
	ANM_P_FARMER_HARVESTING = 257,               //
	ANM_P_FARMER_SHOO_SHEEP = 258,               //
	ANM_P_FARMER_SOWING_SEEDS = 259,             //
	ANM_P_FARMER_STOKE_SHEEP = 260,              //
	ANM_P_FARMER_TURNING_SOIL = 261,             //
	ANM_P_FISHERMAN = 262,                       //
	ANM_P_FRIGHT_DUCK = 263,                     //
	ANM_P_FRIGHT_JUMP = 264,                     //
	ANM_P_F_DANCE_A = 265,                       //
	ANM_P_F_DANCE_B = 266,                       //
	ANM_P_F_DANCE_C = 267,                       //
	ANM_P_GIVE_BIRTH = 268,                      //
	ANM_P_GOALKEEPER = 269,                      //
	ANM_P_GOALKEEPER_2 = 270,                    //
	ANM_P_GOALKEEPER_3 = 271,                    //
	ANM_P_GOSSIP_MAN = 272,                      //
	ANM_P_GOSSIP_WOMAN_1 = 273,                  //
	ANM_P_GOSSIP_WOMAN_2 = 274,                  //
	ANM_P_GOSSIP_WOMAN_3 = 275,                  //
	ANM_P_HAMMERING = 276,                       //
	ANM_P_HELP_SOMEONE_HELPEE = 277,             //
	ANM_P_HELP_SOMEONE_HELPER = 278,             //
	ANM_P_HUG = 279,                             //
	ANM_P_INSPECT_OBJECT_1 = 280,                //
	ANM_P_INSPECT_OBJECT_2 = 281,                //
	ANM_P_INSPECT_OBJECT_3 = 282,                //
	ANM_P_INTO_DEAD_DROWNED = 283,               //
	ANM_P_INTO_HAMMERING = 284,                  //
	ANM_P_INTO_MOURNING = 285,                   //
	ANM_P_INTO_POINTING = 286,                   //
	ANM_P_INTO_PRAY = 287,                       //
	ANM_P_INTO_PULL_OBJECT = 288,                //
	ANM_P_INTO_PUSH_OBJECT = 289,                //
	ANM_P_INTO_RUN_MAN = 290,                    //
	ANM_P_INTO_RUN_WOMAN = 291,                  //
	ANM_P_INTO_SAW_WOOD = 292,                   //
	ANM_P_INTO_SLEDGEHAMMER = 293,               //
	ANM_P_INTO_SLEEP = 294,                      //
	ANM_P_INTO_SPRINT_MAN = 295,                 //
	ANM_P_INTO_SPRINT_WOMAN = 296,               //
	ANM_P_INTO_WALK_MAN = 297,                   //
	ANM_P_INTO_WALK_WOMAN = 298,                 //
	ANM_P_JUMP_A = 299,                          //
	ANM_P_JUMP_AT_HAND = 300,                    //
	ANM_P_JUMP_AT_HAND_2 = 301,                  //
	ANM_P_KICK_BALL = 302,                       //
	ANM_P_KICK_BALL_PASS = 303,                  //
	ANM_P_KICK_BALL_PASS_PT1 = 304,              //
	ANM_P_KICK_BALL_PASS_PT2 = 305,              //
	ANM_P_LANDED = 306,                          //
	ANM_P_LANDED_FROM_BACK = 307,                //
	ANM_P_LANDED_FROM_FEET = 308,                //
	ANM_P_LANDED_FROM_FEET_CARRY_OBJECT = 309,   //
	ANM_P_LOOKING_FOR_SOMETHING = 310,           //
	ANM_P_LOOK_AT_HAND = 311,                    //
	ANM_P_LOVING_TOUCH = 312,                    //
	ANM_P_MOURNING = 313,                        //
	ANM_P_M_DANCE_A = 314,                       //
	ANM_P_M_DANCE_B = 315,                       //
	ANM_P_M_DANCE_C = 316,                       //
	ANM_P_ON_FIRE_DIE = 317,                     //
	ANM_P_ON_FIRE_DOUSED = 318,                  //
	ANM_P_ON_FIRE_RUN = 319,                     //
	ANM_P_OUT_OF_HAMMERING = 320,                //
	ANM_P_OUT_OF_MOURNING = 321,                 //
	ANM_P_OUT_OF_POINTING = 322,                 //
	ANM_P_OUT_OF_PRAY = 323,                     //
	ANM_P_OUT_OF_PULL_OBJECT = 324,              //
	ANM_P_OUT_OF_PUSH_OBJECT = 325,              //
	ANM_P_OUT_OF_RUN_MAN = 326,                  //
	ANM_P_OUT_OF_RUN_WOMAN = 327,                //
	ANM_P_OUT_OF_SAW_WOOD = 328,                 //
	ANM_P_OUT_OF_SLEDGEHAMMER = 329,             //
	ANM_P_OUT_OF_SLEEP = 330,                    //
	ANM_P_OUT_OF_WALK_WOMAN = 331,               //
	ANM_P_OVERWORKED1 = 332,                     //
	ANM_P_OVERWORKED2 = 333,                     //
	ANM_P_PANIC_MAN = 334,                       //
	ANM_P_PANIC_WOMAN = 335,                     //
	ANM_P_PICKUP_BALL_PT1 = 336,                 //
	ANM_P_PICKUP_BALL_PT2 = 337,                 //
	ANM_P_PICK_UP_BAG = 338,                     //
	ANM_P_PICK_UP_POT_ON_HEAD = 339,             //
	ANM_P_PICK_UP_STICKS = 340,                  //
	ANM_P_PLAY_ACCORDIAN = 341,                  //
	ANM_P_POISONED = 342,                        //
	ANM_P_PRAY = 343,                            //
	ANM_P_PROD_CAMPFIRE = 344,                   //
	ANM_P_PULL_OBJECT = 345,                     //
	ANM_P_PUSH_OBJECT = 346,                     //
	ANM_P_PUT_DOWN_BAG = 347,                    //
	ANM_P_PUT_DOWN_STICKS = 348,                 //
	ANM_P_PUT_OUT_FIRE = 349,                    //
	ANM_P_PUZZLED = 350,                         //
	ANM_P_RUN_MAN = 351,                         //
	ANM_P_RUN_WITH_BALL = 352,                   //
	ANM_P_RUN_WOMAN = 353,                       //
	ANM_P_SAW_WOOD = 354,                        //
	ANM_P_SCARED_STIFF = 355,                    //
	ANM_P_SCARED_STIFF_2 = 356,                  //
	ANM_P_SCORE_GOAL = 357,                      //
	ANM_P_SCRIPT1 = 358,                         //
	ANM_P_SCRUBBS = 359,                         //
	ANM_P_SCULPTOR = 360,                        //
	ANM_P_SHAKE_FIST_FEMALE = 361,               //
	ANM_P_SHOOT_AT_GOAL = 362,                   //
	ANM_P_SHOOT_AT_GOAL_2 = 363,                 //
	ANM_P_SHOOT_AT_GOAL_PT_1 = 364,              //
	ANM_P_SHOOT_AT_GOAL_PT_2 = 365,              //
	ANM_P_SHRUG_SHOULDERS = 366,                 //
	ANM_P_SITTING_DOWN1_INTO = 367,              //
	ANM_P_SITTING_DOWN1_OUT_OF = 368,            //
	ANM_P_SITTING_DOWN1_SITTING = 369,           //
	ANM_P_SITTING_DOWN2_INTO = 370,              //
	ANM_P_SITTING_DOWN2_OUT_OF = 371,            //
	ANM_P_SITTING_DOWN2_SITTING = 372,           //
	ANM_P_SITTING_DOWN_DANCE_SAILING1 = 373,     //
	ANM_P_SITTING_DOWN_DANCE_SAILING2 = 374,     //
	ANM_P_SITTING_DOWN_DANCE_SAILING3 = 375,     //
	ANM_P_SITTING_DOWN_MALE1 = 376,              //
	ANM_P_SITTING_DOWN_MALE2 = 377,              //
	ANM_P_SITTING_SWINGING_LEGS = 378,           //
	ANM_P_SKIP02 = 379,                          //
	ANM_P_SLEDGEHAMMER = 380,                    //
	ANM_P_SLEEP = 381,                           //
	ANM_P_SLEEPING = 382,                        //
	ANM_P_SPRINT_RUN_MAN = 383,                  //
	ANM_P_SPRINT_RUN_WOMAN = 384,                //
	ANM_P_STAND = 385,                           //
	ANM_P_STAND_DESPAIR_1 = 386,                 //
	ANM_P_STAND_DESPAIR_2 = 387,                 //
	ANM_P_STAND_DESPAIR_3 = 388,                 //
	ANM_P_STIR_POT01 = 389,                      //
	ANM_P_SWIM2 = 390,                           //
	ANM_P_TALK1_FEMALE = 391,                    //
	ANM_P_TALK1_MALE = 392,                      //
	ANM_P_TALK2_FEMALE = 393,                    //
	ANM_P_TALK2_MALE = 394,                      //
	ANM_P_TALKING_AND_POINTING = 395,            //
	ANM_P_TALK_TO_NEIGHBOUR = 396,               //
	ANM_P_THANK_YOU = 397,                       //
	ANM_P_THANK_YOU_FEMALE = 398,                //
	ANM_P_THROWN = 399,                          //
	ANM_P_THROWN_DEAD = 400,                     //
	ANM_P_THROWN_VORTEX = 401,                   //
	ANM_P_THROW_BALL_PT_1 = 402,                 //
	ANM_P_THROW_BALL_PT_2 = 403,                 //
	ANM_P_THROW_ROCK = 404,                      //
	ANM_P_THROW_WATER = 405,                     //
	ANM_P_TITANIC = 406,                         //
	ANM_P_TURN_180 = 407,                        //
	ANM_P_TURN_90_LEFT = 408,                    //
	ANM_P_TURN_90_RIGHT = 409,                   //
	ANM_P_TURN_ON_THE_SPOT_LEFT = 410,           //
	ANM_P_TURN_ON_THE_SPOT_RIGHT = 411,          //
	ANM_P_WAITING_FOR_BALL = 412,                //
	ANM_P_WAITING_FOR_BALL_2 = 413,              //
	ANM_P_WAITING_FOR_KICK_OFF = 414,            //
	ANM_P_WAITING_FOR_KICK_OFF_2 = 415,          //
	ANM_P_WAITING_FOR_KICK_OFF_3 = 416,          //
	ANM_P_WAITING_FOR_KICK_OFF_4 = 417,          //
	ANM_P_WAITING_IMPATEINTLY = 418,             //
	ANM_P_WALK_BECKON02 = 419,                   //
	ANM_P_WALK_DESPAIR_AWAY_1 = 420,             //
	ANM_P_WALK_DESPAIR_AWAY_2 = 421,             //
	ANM_P_WALK_DESPAIR_AWAY_3 = 422,             //
	ANM_P_WALK_DESPAIR_TO_1 = 423,               //
	ANM_P_WALK_DESPAIR_TO_2 = 424,               //
	ANM_P_WALK_DESPAIR_TO_3 = 425,               //
	ANM_P_WALK_INJURED = 426,                    //
	ANM_P_WALK_MAN = 427,                        //
	ANM_P_WALK_POINT = 428,                      //
	ANM_P_WALK_TIRED_EYES = 429,                 //
	ANM_P_WALK_TO_STAND_WOMAN = 430,             //
	ANM_P_WALK_WOMAN = 431,                      //
	ANM_P_WELL_DONE__FEMALE = 432,               //
	ANM_P_WHITTLING_AT_STICK = 433,              //
	ANM_P_YANKED_UNDERGROUND = 434,              //
	ANM_P_YANKED_UNDERGROUND2 = 435,             //
	ANM_P_YANKED_UNDERGROUND3 = 436,             //
	ANM_P_YAWN = 437,                            //
	ANM_P_YAWN2 = 438,                           //
	ANM_S_PRIEST_SUCK_LIFE_FORCE = 439,          //
	ANM_A_WORM = 440,                            //
	MAX_COUNT_3D_ANIMS = 441,                    //
};

constexpr char* gG3DStringList[626] = {
	"MSH_DUMMY",
	"MSH_A_BAT_1",
	"MSH_A_BAT_2",
	"MSH_A_CROW_1",
	"MSH_A_CROW_2",
	"MSH_A_DOVE_1",
	"MSH_A_DOVE_2",
	"MSH_A_PIGEON_1",
	"MSH_A_PIGEON_2",
	"MSH_A_SEAGULL_1",
	"MSH_A_SEAGULL_2",
	"MSH_A_SPELL_DOVE",
	"MSH_A_SWALLOW_1",
	"MSH_A_SWALLOW_2",
	"MSH_A_VULTURE_1",
	"MSH_A_VULTURE_2",
	"MSH_A_COW_1",
	"MSH_A_COW_2",
	"MSH_A_COW_3",
	"MSH_A_HORSE_1",
	"MSH_A_HORSE_2",
	"MSH_A_HORSE_3",
	"MSH_A_LEOPARD_1",
	"MSH_A_LEOPARD_2",
	"MSH_A_LEOPARD_3",
	"MSH_A_LION_1",
	"MSH_A_LION_2",
	"MSH_A_LION_3",
	"MSH_A_PIG_1",
	"MSH_A_PIG_2",
	"MSH_A_PIG_3",
	"MSH_SHARK_BONED",
	"MSH_A_SHEEP_1",
	"MSH_A_SHEEP_2",
	"MSH_A_SHEEP_3",
	"MSH_A_TIGER_1",
	"MSH_A_TIGER_2",
	"MSH_A_TIGER_3",
	"MSH_A_TORTOISE_1",
	"MSH_A_TORTOISE_2",
	"MSH_A_TORTOISE_3",
	"MSH_A_WOLF_1",
	"MSH_A_WOLF_2",
	"MSH_A_WOLF_3",
	"MSH_A_ZEBRA_1",
	"MSH_A_ZEBRA_2",
	"MSH_A_ZEBRA_3",
	"MSH_B_AMCN_1",
	"MSH_B_AMCN_1_A",
	"MSH_B_AMCN_2",
	"MSH_B_AMCN_2_A",
	"MSH_B_AMCN_3",
	"MSH_B_AMCN_3_A",
	"MSH_B_AMCN_4",
	"MSH_B_AMCN_5",
	"MSH_B_AMCN_CRECHE",
	"MSH_B_AMCN_FENCE",
	"MSH_B_AMCN_GRAVEYARD",
	"MSH_B_AMCN_TOTEM",
	"MSH_B_AMCN_WONDER",
	"MSH_B_AMCN_WORKSHOP",
	"MSH_B_AZTC_1",
	"MSH_B_AZTC_2",
	"MSH_B_AZTC_3",
	"MSH_B_AZTC_4",
	"MSH_B_AZTC_5",
	"MSH_B_AZTC_CRECHE",
	"MSH_B_AZTC_GRAVEYARD",
	"MSH_B_AZTC_OLMECHEAD",
	"MSH_B_AZTC_STATUE",
	"MSH_B_AZTC_TEMPLE",
	"MSH_B_AZTC_TOTEM",
	"MSH_B_AZTC_WONDER",
	"MSH_B_AZTC_WORKSHOP",
	"MSH_B_CELT_1",
	"MSH_B_CELT_2",
	"MSH_B_CELT_3",
	"MSH_B_CELT_4",
	"MSH_B_CELT_5",
	"MSH_B_CELT_5A",
	"MSH_B_CELT_CRECHE",
	"MSH_B_CELT_FENCE_SHORT",
	"MSH_B_CELT_FENCE_TALL",
	"MSH_B_CELT_GRAVEYARD",
	"MSH_B_CELT_GRAVE_01",
	"MSH_B_CELT_GRAVE_02",
	"MSH_B_CELT_GRAVE_03",
	"MSH_B_CELT_TOTEM",
	"MSH_B_CELT_WALLGATE_SPELL",
	"MSH_B_CELT_WALLTOWER_SPELL",
	"MSH_B_CELT_WALL_SPELL",
	"MSH_B_CELT_WONDER",
	"MSH_B_CELT_WORKSHOP",
	"MSH_B_CITADEL_AMCN_ALTAR",
	"MSH_B_CITADEL_AZTC_ALTAR",
	"MSH_B_CITADEL_CELT_ALTAR",
	"MSH_B_CITADEL_DESIRE_HUNGRY",
	"MSH_B_CITADEL_DESIRE_SLEEP",
	"MSH_B_CITADEL_EGPT_ALTAR",
	"MSH_B_CITADEL_GREK_ALTAR",
	"MSH_B_CITADEL_JAPN_ALTAR",
	"MSH_B_CITADEL_NORS_ALTAR",
	"MSH_B_CITADEL_POP_COUNTER_BEAD",
	"MSH_B_CITADEL_TIBT_ALTAR",
	"MSH_B_EGPT_1",
	"MSH_B_EGPT_1_A",
	"MSH_B_EGPT_2",
	"MSH_B_EGPT_2_A",
	"MSH_B_EGPT_3",
	"MSH_B_EGPT_4",
	"MSH_B_EGPT_5",
	"MSH_B_EGPT_5_A",
	"MSH_B_EGPT_BARREL",
	"MSH_B_EGPT_CART",
	"MSH_B_EGPT_CRECHE",
	"MSH_B_EGPT_GRAVEYARD",
	"MSH_B_EGPT_NEEDLE",
	"MSH_B_EGPT_POT_A",
	"MSH_B_EGPT_POT_B",
	"MSH_B_EGPT_TOTEM",
	"MSH_B_EGPT_WELL",
	"MSH_B_EGPT_WINDMILL",
	"MSH_B_EGPT_WONDER",
	"MSH_B_EGPT_WORKSHOP",
	"MSH_B_GREK_1",
	"MSH_B_GREK_2",
	"MSH_B_GREK_2_A",
	"MSH_B_GREK_3",
	"MSH_B_GREK_4",
	"MSH_B_GREK_5",
	"MSH_B_GREK_5A",
	"MSH_B_GREK_CRECHE",
	"MSH_B_GREK_GRAVEYARD",
	"MSH_B_GREK_SAIL",
	"MSH_B_GREK_STATUE",
	"MSH_B_GREK_TOTEM",
	"MSH_B_GREK_WONDER",
	"MSH_B_GREK_WORKSHOP",

	"MSH_B_JAPN_1",
	"MSH_B_JAPN_2",
	"MSH_B_JAPN_3",
	"MSH_B_JAPN_4",
	"MSH_B_JAPN_5",
	"MSH_B_JAPN_CRECHE",
	"MSH_B_JAPN_GRAVEYARD",
	"MSH_B_JAPN_TOTEM",
	"MSH_B_JAPN_WONDER",
	"MSH_B_JAPN_WORKSHOP",

	"MSH_B_CAMPFIRE",
	"MSH_B_CREATURE_PEN_LEASH_POLE",

	"MSH_B_FOOTBALL_CENTRECIRCLE",
	"MSH_B_FOOTBALL_CORNERFLAG",
	"MSH_B_FOOTBALL_GOAL",
	"MSH_B_FOOTBALL_PITCH",
	"MSH_B_FOOTBALL_PITCH_BIG",

	"MSH_B_MINE_ENTRANCE",
	"MSH_B_PIER",
	"MSH_B_SCAFFOLD_01",
	"MSH_B_SCAFFOLD_02",
	"MSH_B_SCAFFOLD_03",
	"MSH_B_XANDO_BASE",
	"MSH_B_XANDO_O",
	"MSH_B_XANDO_X",
	"MSH_PIPER_ENTRANCE_PHYS_1",

	"MSH_B_AMCN_VILLAGECENTRE",
	"MSH_B_AZTC_VILLAGECENTRE",
	"MSH_B_CELT_VILLAGECENTRE",

	"MSH_B_DESIRE_FLAG_ABODE",
	"MSH_B_DESIRE_FLAG_CHILDREN",
	"MSH_B_DESIRE_FLAG_CIVIC_BUILDING",
	"MSH_B_DESIRE_FLAG_FOOD",
	"MSH_B_DESIRE_FLAG_PLAYTIME",
	"MSH_B_DESIRE_FLAG_PROTECTION",
	"MSH_B_DESIRE_FLAG_RAIN",
	"MSH_B_DESIRE_FLAG_SUN",
	"MSH_B_DESIRE_FLAG_WOOD",

	"MSH_B_EGPT_VILLAGECENTRE",
	"MSH_B_GREK_VILLAGECENTRE",
	"MSH_B_JAPN_VILLAGECENTRE",
	"MSH_B_NORS_VILLAGECENTRE",

	"MSH_B_PLAYER_ICON01",
	"MSH_B_PLAYER_ICON01_PLINTH",
	"MSH_B_PLAYER_ICON_APE",
	"MSH_B_PLAYER_ICON_BEAR",
	"MSH_B_PLAYER_ICON_COW",
	"MSH_B_PLAYER_ICON_HORSE",
	"MSH_B_PLAYER_ICON_LEOPARD",
	"MSH_B_PLAYER_ICON_LION",
	"MSH_B_PLAYER_ICON_PLINTH_AMCN",
	"MSH_B_PLAYER_ICON_PLINTH_AZTC",
	"MSH_B_PLAYER_ICON_PLINTH_CELT",
	"MSH_B_PLAYER_ICON_PLINTH_EGPT",
	"MSH_B_PLAYER_ICON_PLINTH_GREK",
	"MSH_B_PLAYER_ICON_PLINTH_JAPN",
	"MSH_B_PLAYER_ICON_PLINTH_NORS",
	"MSH_B_PLAYER_ICON_PLINTH_TIBT",
	"MSH_B_PLAYER_ICON_POLARBEAR",
	"MSH_B_PLAYER_ICON_SHEEP",
	"MSH_B_PLAYER_ICON_TIGER",
	"MSH_B_PLAYER_ICON_TORTOISE",
	"MSH_B_PLAYER_ICON_WOLF",
	"MSH_B_PLAYER_ICON_ZEBRA",

	"MSH_B_TIBT_VILLAGECENTRE",
	"MSH_B_VILLAGECENTRE_SPELLHAND",
	"MSH_B_NORS_1",
	"MSH_B_NORS_2",
	"MSH_B_NORS_2_A",
	"MSH_B_NORS_3",
	"MSH_B_NORS_3_A",
	"MSH_B_NORS_4",
	"MSH_B_NORS_5",
	"MSH_B_NORS_CRECHE",
	"MSH_B_NORS_GATE",
	"MSH_B_NORS_GRAVEYARD",
	"MSH_B_NORS_PIGPEN",
	"MSH_B_NORS_TEMPLE",
	"MSH_B_NORS_TOTEM",
	"MSH_B_NORS_WONDER",
	"MSH_B_NORS_WORKSHOP",
	"MSH_NORS_GATE_PHYS_1",
	"MSH_NORS_GATE_PHYS_2",
	"MSH_B_AMCN_STORAGE_PIT",
	"MSH_B_AZTC_STORAGE_PIT",
	"MSH_B_CELT_STORAGE_PIT",
	"MSH_B_EGPT_STORAGE_PIT",
	"MSH_B_GRAIN",
	"MSH_B_GREK_STORAGE_PIT",
	"MSH_B_JAPN_STORAGE_PIT",
	"MSH_B_NORS_STORAGE_PIT",
	"MSH_B_TIBT_STORAGE_PIT",
	"MSH_B_WOOD_00",
	"MSH_B_WOOD_01",
	"MSH_B_WOOD_02",
	"MSH_B_WOOD_03",
	"MSH_B_WOOD_04",
	"MSH_B_WOOD_OUTSIDE",
	"MSH_B_TIBT_1",
	"MSH_B_TIBT_2",
	"MSH_B_TIBT_3",
	"MSH_B_TIBT_3_A",
	"MSH_B_TIBT_4",
	"MSH_B_TIBT_5",
	"MSH_B_TIBT_5_A",
	"MSH_B_TIBT_CRECHE",
	"MSH_B_TIBT_GRAVEYARD",
	"MSH_B_TIBT_LARGE_PILLER",
	"MSH_B_TIBT_MINI_TEMPLE",
	"MSH_B_TIBT_TOTEM",
	"MSH_B_TIBT_WONDER",
	"MSH_B_TIBT_WORKSHOP",
	"MSH_B_SPELLHAND",
	"MSH_B_WORSHIPGRAIN",
	"MSH_B_WORSHIPSITE_01_NEUTRAL",
	"MSH_BOULDER1_CHALK",
	"MSH_BOULDER1_LIME",
	"MSH_BOULDER1_SAND",
	"MSH_BOULDER1_VOLCANIC",
	"MSH_BOULDER2_CHALK",
	"MSH_BOULDER2_LIME",
	"MSH_BOULDER2_SAND",
	"MSH_BOULDER2_VOLCANIC",
	"MSH_BOULDER3_CHALK",
	"MSH_BOULDER3_LIME",
	"MSH_BOULDER3_SAND",
	"MSH_BOULDER3_VOLCANIC",
	"MSH_FAT_PILAR_CHALK",
	"MSH_FAT_PILAR_LIME",
	"MSH_FAT_PILAR_SAND",
	"MSH_FAT_PILAR_VOLCANIC",
	"MSH_F_CREATURE_CAVE",
	"MSH_F_FOREST",
	"MSH_F_FOREST01",
	"MSH_F_FOREST02",
	"MSH_F_FOREST03",
	"MSH_F_FOREST_BOOK01",
	"MSH_F_GIANTSKELETON",
	"MSH_F_GIANTSKULL",
	"MSH_F_IDOL",
	"MSH_F_PIPER_CAVE",
	"MSH_F_RIB1",
	"MSH_F_RIB2",
	"MSH_F_SPOUTHOLE",
	"MSH_F_VERTABRAE",
	"MSH_PILAR2_CHALK",
	"MSH_PILAR2_LIME",
	"MSH_PILAR2_SAND",
	"MSH_PILAR2_VOLCANIC",
	"MSH_PILAR3_CHALK",
	"MSH_PILAR3_LIME",
	"MSH_PILAR3_SAND",
	"MSH_PILAR3_VOLCANIC",
	"MSH_PILAR_CHALK",
	"MSH_PILAR_LIME",
	"MSH_PILAR_SAND",
	"MSH_PILAR_VOLCANIC",
	"MSH_PUZZLE_1",
	"MSH_PUZZLE_2",
	"MSH_PUZZLE_3",
	"MSH_PUZZLE_4",
	"MSH_PUZZLE_5",
	"MSH_SPIKEY_PILAR_CHALK",
	"MSH_SPIKEY_PILAR_LIME",
	"MSH_SPIKEY_PILAR_SAND",
	"MSH_SPIKEY_PILAR_VOLCANIC",
	"MSH_Z_PHONEBOX",
	"MSH_I_BLAST",
	"MSH_I_FIREBALL",
	"MSH_I_FOOD",
	"MSH_I_FOOTBALL",
	"MSH_I_FOREST",
	"MSH_I_HEAL",
	"MSH_I_LIGHTNING",
	"MSH_I_LIGHTNING2",
	"MSH_I_MINISCROLL",
	"MSH_I_MINISCROLL_ACTIVE",
	"MSH_I_MINISCROLL_BRONZE",
	"MSH_I_MINISCROLL_BRONZE_ACTIVE",
	"MSH_I_MINISCROLL_GOLD",
	"MSH_I_MINISCROLL_GOLD_ACTIVE",
	"MSH_I_MINISCROLL_SILVER",
	"MSH_I_MINISCROLL_SILVER_ACTIVE",
	"MSH_I_OFFERING_BEDROLL",
	"MSH_I_OFFERING_FERTILITY",
	"MSH_I_OFFERING_FOOD",
	"MSH_I_OFFERING_PLAY",
	"MSH_I_OFFERING_PROTECTION",
	"MSH_I_OFFERING_SKULL",
	"MSH_I_OFFERING_VILLAGE_IMPROVE",
	"MSH_I_OFFERING_WATER",
	"MSH_I_OFFERING_WOOD",
	"MSH_I_PRIEST",
	"MSH_I_SHIELD",
	"MSH_I_SKELETON",
	"MSH_I_SPELLGEM",
	"MSH_I_TREES",
	"MSH_I_WALL",
	"MSH_GATE_TOTEM_PLINTHE_PHYS_1",
	"MSH_GATE_TOTEM_PLINTHE_PHYS_2",
	"MSH_GATE_TOTEM_PLINTHE_PHYS_3",
	"MSH_O_ARK",
	"MSH_O_ARK_DRY_DOCK",
	"MSH_O_ARK_WRECK",
	"MSH_O_AXE",
	"MSH_O_BAG",
	"MSH_O_BALL",
	"MSH_O_BELLTOWER",
	"MSH_O_BOX_FRAME",
	"MSH_O_BRANCH_EVERGREEN_INHAND",
	"MSH_O_BRANCH_FRUIT_INHAND",
	"MSH_O_BRANCH_HARDWOOD_INHAND",
	"MSH_O_BURNT_TREE",
	"MSH_O_CAULDRON",
	"MSH_O_CHAMPI",
	"MSH_O_CREATURE_TURD",
	"MSH_O_CROOK",
	"MSH_O_FISHINGROD",
	"MSH_O_FLATROCK_CHALK",
	"MSH_O_FLATROCK_LIMESTONE",
	"MSH_O_FLATROCK_SANDSTONE",
	"MSH_O_FLATROCK_VOLCANIC",
	"MSH_O_FLOWER_BED_1",
	"MSH_O_FLOWER_BED_2",
	"MSH_O_GATE_TOTEM_APE",
	"MSH_O_GATE_TOTEM_BLANK",
	"MSH_O_GATE_TOTEM_COW",
	"MSH_O_GATE_TOTEM_PLINTHE",
	"MSH_O_GATE_TOTEM_TIGER",
	"MSH_O_HAMMER",
	"MSH_O_INFO_SIGN",
	"MSH_O_JAPN_STONETHING",
	"MSH_O_LARGE_FISH_DEAD",
	"MSH_O_LIGHT_BULB",
	"MSH_O_LOGS_INHAND",
	"MSH_O_LONGROCK_CHALK",
	"MSH_O_LONGROCK_LIMESTONE",
	"MSH_O_LONGROCK_SANDSTONE",
	"MSH_O_LONGROCK_VOLCANIC",
	"MSH_O_MAGIC_MUSHROOM",
	"MSH_O_MALLET_HEAVY",
	"MSH_O_ROCK_CHALK",
	"MSH_O_ROCK_LIMESTONE",
	"MSH_O_ROCK_SANDSTONE",
	"MSH_O_ROCK_VOLCANIC",
	"MSH_O_SAW",
	"MSH_O_SCYTHE",
	"MSH_O_SHARPROCK_CHALK",
	"MSH_O_SHARPROCK_LIMESTONE",
	"MSH_O_SHARPROCK_SANDSTONE",
	"MSH_O_SHARPROCK_VOLCANIC",
	"MSH_O_SKULL_TOTEM",
	"MSH_O_SPADE",
	"MSH_O_SQUAREROCK_CHALK",
	"MSH_O_SQUAREROCK_LIMESTONE",
	"MSH_O_SQUAREROCK_SANDSTONE",
	"MSH_O_SQUAREROCK_VOLCANIC",
	"MSH_O_TICKERTAPE",
	"MSH_O_TOADSTOOL",
	"MSH_O_TOMBSTONE",
	"MSH_O_TOWNLIGHT",
	"MSH_O_TOY_BALL",
	"MSH_O_TOY_BOWLING_BALL",
	"MSH_O_TOY_CUDDLY",
	"MSH_O_TOY_DIE",
	"MSH_O_TOY_SKITTLE",
	"MSH_O_WHEAT_ENCLOSURE",
	"MSH_O_WHEAT_INHAND",
	"MSH_O_WOOD_INHAND",
	"MSH_REWARD_CHEST_EXPLODE",
	"MSH_SCULPTURE_IN_PROGRESS",
	"MSH_SINGING_STONE_CENTER",
	"MSH_SINGING_STONE",
	"MSH_IDOL_BUILDER",
	"MSH_NEW_SHAOLIN",
	"MSH_P_AMCN_F_A_1",
	"MSH_P_AMCN_F_A_2",
	"MSH_P_AMCN_F_A_3",
	"MSH_P_AMCN_M_A_1",
	"MSH_P_AMCN_M_A_2",
	"MSH_P_AMCN_M_A_3",
	"MSH_P_ANIMAL_BREEDER",
	"MSH_P_ANIMAL_TRAINER",
	"MSH_P_AZTC_F_A_1",
	"MSH_P_AZTC_F_A_2",
	"MSH_P_AZTC_F_A_3",
	"MSH_P_AZTC_LEADER",
	"MSH_P_AZTC_M_A_1",
	"MSH_P_AZTC_M_A_2",
	"MSH_P_AZTC_M_A_3",
	"MSH_P_BANDIT",
	"MSH_P_BOY_BLACK_1",
	"MSH_P_BOY_BLACK_2",
	"MSH_P_BOY_BLACK_3",
	"MSH_P_BOY_ORIENTAL_1",
	"MSH_P_BOY_ORIENTAL_2",
	"MSH_P_BOY_ORIENTAL_3",
	"MSH_P_BOY_TAN_1",
	"MSH_P_BOY_TAN_2",
	"MSH_P_BOY_TAN_3",
	"MSH_P_BOY_WHITE",
	"MSH_P_BOY_WHITE_1",
	"MSH_P_BOY_WHITE_2",
	"MSH_P_BOY_WHITE_3",
	"MSH_P_CELT_FOOTBALLER",
	"MSH_P_CELT_F_A_1",
	"MSH_P_CELT_F_A_2",
	"MSH_P_CELT_F_A_3",
	"MSH_P_CELT_M_A_1",
	"MSH_P_CELT_M_A_2",
	"MSH_P_CELT_M_A_3",
	"MSH_P_CRUSADER",
	"MSH_P_CULT_PRIEST",
	"MSH_P_CULT_PRIESTESS",
	"MSH_P_EGPT_F_A_1",
	"MSH_P_EGPT_F_A_2",
	"MSH_P_EGPT_F_A_3",
	"MSH_P_EGPT_M_A_1",
	"MSH_P_EGPT_M_A_2",
	"MSH_P_EGPT_M_A_3",
	"MSH_P_EGPT_M_B_1",
	"MSH_P_EGPT_M_B_2",
	"MSH_P_ENGINEER",
	"MSH_P_GIRL_BLACK_1",
	"MSH_P_GIRL_BLACK_2",
	"MSH_P_GIRL_BLACK_3",
	"MSH_P_GIRL_ORIENTAL_1",
	"MSH_P_GIRL_ORIENTAL_2",
	"MSH_P_GIRL_ORIENTAL_3",
	"MSH_P_GIRL_TAN_1",
	"MSH_P_GIRL_TAN_2",
	"MSH_P_GIRL_TAN_3",
	"MSH_P_GIRL_WHITE",
	"MSH_P_GIRL_WHITE_1",
	"MSH_P_GIRL_WHITE_2",
	"MSH_P_GIRL_WHITE_3",
	"MSH_P_GREK_F_A_1",
	"MSH_P_GREK_F_A_2",
	"MSH_P_GREK_F_A_3",
	"MSH_P_GREK_M_A_1",
	"MSH_P_GREK_M_A_2",
	"MSH_P_GREK_M_A_3",
	"MSH_P_HERMIT",
	"MSH_P_HIPPY",
	"MSH_P_IDOL_BUILDER",
	"MSH_P_INTRO_F",
	"MSH_P_INTRO_M",
	"MSH_P_JAPANESE_F_A_2",
	"MSH_P_JAPANESE_F_A_3",
	"MSH_P_JAPANESE_M_A_1",
	"MSH_P_JAPANESE_M_A_2",
	"MSH_P_JAPANESE_M_A_3",
	"MSH_P_JAPN_F_A_1",
	"MSH_P_JAPN_F_A_2",
	"MSH_P_JAPN_F_A_3",
	"MSH_P_JAPN_M_A_1",
	"MSH_P_JAPN_M_A_2",
	"MSH_P_JAPN_M_A_3",
	"MSH_P_NOMAD",
	"MSH_P_NORS_FOOTBALLER",
	"MSH_P_NORS_F_A_1",
	"MSH_P_NORS_F_A_2",
	"MSH_P_NORS_F_A_3",
	"MSH_P_NORS_M_A_1",
	"MSH_P_NORS_M_A_2",
	"MSH_P_NORS_M_A_3",
	"MSH_P_NORS_SAILOR",
	"MSH_P_PIED_PIPER",
	"MSH_P_SAILOR_ACCORDIAN",
	"MSH_P_SCULPTOR",
	"MSH_P_SHAOLIN_MONK",
	"MSH_P_SHEPERD",
	"MSH_P_SKELETON_F",
	"MSH_P_SKELETON_M",
	"MSH_P_TIBETAN_F_A_2",
	"MSH_P_TIBETAN_F_A_3",
	"MSH_P_TIBETAN_M_A_1",
	"MSH_P_TIBETAN_M_A_2",
	"MSH_P_TIBETAN_M_A_3",
	"MSH_P_TIBT_F_A_1",
	"MSH_P_TIBT_F_A_2",
	"MSH_P_TIBT_F_A_3",
	"MSH_P_TIBT_M_A_1",
	"MSH_P_TIBT_M_A_2",
	"MSH_P_TIBT_M_A_3",
	"MSH_P_TIBT_M_B_1",
	"MSH_P_TIBT_M_B_2",
	"MSH_CREED",
	"MSH_S_BLAST_CONE",
	"MSH_S_BLAST_CENTRE",
	"MSH_S_BLAST_INNER",
	"MSH_S_BLAST_OUTER",
	"MSH_S_COLUMN",
	"MSH_S_FIREBALL_TAIL",
	"MSH_S_HEAL_MESH",
	"MSH_S_GRAIN_PILE",
	"MSH_S_HORN_OF_PLENTY",
	"MSH_S_PHILE",
	"MSH_S_PHILE_ANGRY",
	"MSH_S_PHILE_BIG",
	"MSH_S_PHILE_FAT",
	"MSH_S_PHILE_FREEZE",
	"MSH_S_PHILE_FRIGHTEN",
	"MSH_S_PHILE_HUNGRY",
	"MSH_S_PHILE_ILL",
	"MSH_S_PHILE_INVISIBLE",
	"MSH_S_PHILE_ITCHY",
	"MSH_S_PHILE_LOVE",
	"MSH_S_PHILE_SMALL",
	"MSH_S_PHILE_STRONG",
	"MSH_S_PHILE_THIN",
	"MSH_S_PHILE_THIRSTY",
	"MSH_S_PHILE_TIRED",
	"MSH_S_PHILE_WEAK",
	"MSH_S_RAIN_CONE",
	"MSH_S_SHIELDMAGICLOWPOLY",
	"MSH_S_SOLID_SHIELD",
	"MSH_S_SPELLBALLSURFACE02",
	"MSH_S_SKULL_ICON",
	"MSH_S_SPELL_CREATOR",
	"MSH_S_SPELL_DISPENSER",
	"MSH_S_VORTEXCYLINDER",
	"MSH_S_ZCHEATBOX",
	"MSH_S_PULSE_IN",
	"MSH_S_PULSE_OUT",
	"MSH_A_WORM",
	"MSH_B_DUMMY_CITADEL",
	"MSH_Z_APPLE_BARREL",
	"MSH_Z_ARROW",
	"MSH_Z_SPELLROCK01",
	"MSH_Z_SPELLROCK02",
	"MSH_T_COPSE",
	"MSH_T_COPSE_A",
	"MSH_T_BEECH",
	"MSH_T_BIRCH",
	"MSH_T_BUSH",
	"MSH_T_BUSH_A",
	"MSH_T_BUSH_B",
	"MSH_T_CEDAR",
	"MSH_T_CONIFER",
	"MSH_T_CONIFER_A",
	"MSH_T_CYPRESS",
	"MSH_T_CYPRESS_A",
	"MSH_T_HEDGE",
	"MSH_T_HEDGE_A",
	"MSH_T_OAK",
	"MSH_T_OAK_A",
	"MSH_T_OLIVE",
	"MSH_T_PALM",
	"MSH_T_PALM_A",
	"MSH_T_PALM_B",
	"MSH_T_PALM_C",
	"MSH_T_PINE",
	"MSH_T_RICE",
	"MSH_T_ROOTS",
	"MSH_T_ROOTS_PILE",
	"MSH_T_WHEAT",
	"MSH_T_WHEAT_INFIELD",
	"MSH_U_AXE",
	"MSH_U_BAG",
	"MSH_U_BALL",
	"MSH_U_BENCH",
	"MSH_U_BUCKET",
	"MSH_U_BUCKET02",
	"MSH_U_CART",
	"MSH_U_CHEST",
	"MSH_U_CHEST_TOP",
	"MSH_U_CROOK",
	"MSH_U_FISHING_ROD",
	"MSH_U_FRAME",
	"MSH_U_FRAME_SKIN",
	"MSH_U_HAMMER",
	"MSH_U_MALLET_HEAVY",
	"MSH_U_SAW",
	"MSH_U_SCYTHE",
	"MSH_U_SPADE",
	"MSH_U_STOOL",
	"MSH_U_STOOL01",
	"MSH_U_TABLE",
	"MSH_U_TROUGH",
	"MSH_U_WASHING_LINE_AMCN",
	"MSH_U_WASHING_LINE_AZTC",
	"MSH_U_WASHING_LINE_CELT",
	"MSH_U_WASHING_LINE_EGPT",
	"MSH_U_WASHING_LINE_GREK",
	"MSH_U_WASHING_LINE_JAPN",
	"MSH_U_WASHING_LINE_NORS",
	"MSH_U_WASHING_LINE_TIBT"
};

// Last Saved,#2001-02-01 13:28:36#,"opurkiss"
