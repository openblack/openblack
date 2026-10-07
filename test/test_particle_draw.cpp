/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdint>

#include <array>
#include <string>
#include <vector>

#include <EnumHeader.h>
#include <StackedBitmap.h>
#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Graphics/ZSort.h"
#include "Particles/ParticleCreators.h"
#include "Particles/ParticleDrawFrame.h"
#include "Particles/ParticleMaths.h"

using namespace openblack;
using namespace openblack::particles;
using namespace openblack::particles::draw;

namespace
{
constexpr float k_Epsilon = 1e-4f;

void ExpectNear(const glm::vec3& actual, const glm::vec3& expected, float epsilon = k_Epsilon)
{
	EXPECT_NEAR(actual.x, expected.x, epsilon);
	EXPECT_NEAR(actual.y, expected.y, epsilon);
	EXPECT_NEAR(actual.z, expected.z, epsilon);
}

/// A frame with two materials and nothing yet drawn
Frame MakeFrame()
{
	Frame frame;
	frame.materials = {{.texture = 1, .alphaTexture = 2, .mode = graphics::render_modes::Mode::AlphaTexturedAlphaNz},
	                   {.texture = 3, .alphaTexture = 4, .mode = graphics::render_modes::Mode::AlphaTexturedAlphaAdditiveNz}};
	return frame;
}

void AddSprite(Frame& frame, glm::vec3 point, uint32_t material)
{
	frame.items.push_back({.kind = ItemKind::Sprite, .index = static_cast<uint32_t>(frame.sprites.size()), .sortPoint = point});
	frame.sprites.push_back({.positionHalfWidth = glm::vec4(point, 1.0f)});
	frame.spriteMaterials.push_back(material);
}

void AddGroup(Frame& frame, DrawPath path, glm::vec3 origin, uint32_t first)
{
	frame.groups.push_back(
	    {.path = path, .origin = origin, .firstItem = first, .itemCount = static_cast<uint32_t>(frame.items.size()) - first});
}

/// The game's ribbon: each segment's corners on either side of its two joints, as the camera sees the segment, and
/// where two segments meet their corners moved half way to each other
std::vector<glm::vec3> GameRibbon(const std::vector<glm::vec3>& joints, const std::vector<float>& scales, const glm::vec3& eye)
{
	const auto segments = joints.size() - 1;
	std::vector<glm::vec3> corners(segments * 4);
	for (size_t i = 0; i < segments; ++i)
	{
		const auto along = joints[i + 1] - joints[i];
		const auto side = [&](size_t joint) {
			const auto s = glm::cross(eye - joints[joint], along);
			const float length = glm::length(s);
			return length > 0.0f ? s * (scales[joint] / length) : glm::vec3(0.0f);
		};
		corners[i * 4 + 0] = joints[i] + side(i);
		corners[i * 4 + 1] = joints[i] - side(i);
		corners[i * 4 + 2] = joints[i + 1] + side(i + 1);
		corners[i * 4 + 3] = joints[i + 1] - side(i + 1);
	}
	for (size_t i = 1; i < segments; ++i)
	{
		auto* previous = &corners[(i - 1) * 4];
		auto* current = &corners[i * 4];
		current[0] = previous[2] = (previous[2] + current[0]) * 0.5f;
		current[1] = previous[3] = (previous[3] + current[1]) * 0.5f;
	}
	return corners;
}
} // namespace

TEST(ParticleDraw, LightMapsAreTakenOutOfTheirFilesGrid)
{
	// Four frames of 2 by 2 grey texels, two to a row of the file: frame f's texels are all f
	std::vector<uint8_t> bytes(16);
	for (size_t row = 0; row < 4; ++row)
	{
		for (size_t column = 0; column < 4; ++column)
		{
			bytes[row * 4 + column] = static_cast<uint8_t>((row / 2) * 2 + (column / 2));
		}
	}
	const auto bitmap = psys::LoadStackedBitmap(bytes, 2, 1, 4, 3);
	ASSERT_TRUE(bitmap.has_value());
	EXPECT_EQ(bitmap->frames, 3);
	for (int frame = 0; frame < 3; ++frame)
	{
		for (const auto texel : bitmap->Frame(frame))
		{
			EXPECT_EQ(texel, frame);
		}
	}
	// Frames past the last wrap round
	EXPECT_EQ(bitmap->Frame(4)[0], 1);
	// A file of another size is nothing
	EXPECT_FALSE(psys::LoadStackedBitmap(std::span(bytes).first(15), 2, 1, 4, 3).has_value());
}

TEST(ParticleDraw, EnumHeadersGiveTheirNames)
{
	const auto values =
	    psys::ParseEnumHeader("enum MESH_LIST\n{\n    MSH_A_BAT_1    =    1,\t//\t\n"
	                          "    MSH_S_BLAST_CENTRE = 527, // the blast\r\n  bad line\n  = 3,\n  X = y,\n};\n");
	ASSERT_EQ(values.size(), 2u);
	EXPECT_EQ(values.at("MSH_A_BAT_1"), 1);
	EXPECT_EQ(values.at("MSH_S_BLAST_CENTRE"), 527);
}

TEST(ParticleDraw, EnumHeadersCountNamesWithoutValuesAsC)
{
	// The shape of Data/SoundAction.h: explicit values, then a run of bare names counting on from the last one
	std::string text = "#ifndef INCL_SOUNDACTION_H\n#define INCL_SOUNDACTION_H\n\nenum\tLHSoundAction\n{\n"
	                   "\tSOUND_SPELL_HAND_LIGHTNING_LEVEL_3\t= 113,\n";
	for (int i = 114; i < 141; ++i)
	{
		text += "\tSOUND_FILLER_" + std::to_string(i) + ",\t\t\n";
	}
	text += "\tSOUND_SPELL_WATER,\n\tSOUND_SPELL_DOVES,\n    SOUND_SPELL_BATS,   \n\tSOUND_SPELL_WOLVES, // calls\n";
	for (int i = 145; i < 157; ++i)
	{
		text += "\tSOUND_FILLER_" + std::to_string(i) + ",\n";
	}
	text += "\tSOUND_ACTION_SWIM\n};\n\n#endif //SOUNDACTION included\n"
	        "enum SECOND { FIRST_OF_SECOND, SECOND_OF_SECOND = 7, THIRD_OF_SECOND };\n"
	        "enum THIRD\n{\n  KNOWN = 4,\n  UNKNOWN = KNOWN + 1,\n  AFTER_UNKNOWN,\n  KNOWN_AGAIN = 9,\n  AFTER_KNOWN,\n};\n";
	const auto values = psys::ParseEnumHeader(text);
	EXPECT_EQ(values.at("SOUND_SPELL_HAND_LIGHTNING_LEVEL_3"), 113);
	EXPECT_EQ(values.at("SOUND_FILLER_114"), 114);
	EXPECT_EQ(values.at("SOUND_SPELL_WATER"), 141);
	EXPECT_EQ(values.at("SOUND_SPELL_DOVES"), 142);
	EXPECT_EQ(values.at("SOUND_SPELL_BATS"), 143);
	EXPECT_EQ(values.at("SOUND_SPELL_WOLVES"), 144);
	EXPECT_EQ(values.at("SOUND_ACTION_SWIM"), 157);
	// Each enum counts from 0 again
	EXPECT_EQ(values.at("FIRST_OF_SECOND"), 0);
	EXPECT_EQ(values.at("SECOND_OF_SECOND"), 7);
	EXPECT_EQ(values.at("THIRD_OF_SECOND"), 8);
	// A value that isn't a whole number leaves it and the bare names after it out, until the next whole number
	EXPECT_EQ(values.at("KNOWN"), 4);
	EXPECT_FALSE(values.contains("UNKNOWN"));
	EXPECT_FALSE(values.contains("AFTER_UNKNOWN"));
	EXPECT_EQ(values.at("AFTER_KNOWN"), 10);
	// Nothing outside an enum's braces is a name
	EXPECT_FALSE(values.contains("INCL_SOUNDACTION_H"));
}

TEST(ParticleDraw, SortedThingsTakeTheirOwnPlacesAndSharedSheetsJoin)
{
	auto frame = MakeFrame();
	const glm::vec3 camera(0.0f);
	// One sorted effect: two sprites far away on one sheet, one near on another; a queued effect between them
	AddSprite(frame, {0.0f, 0.0f, 100.0f}, 0);
	AddSprite(frame, {0.0f, 0.0f, 10.0f}, 1);
	AddSprite(frame, {0.0f, 0.0f, 90.0f}, 0);
	AddGroup(frame, DrawPath::Sorted, glm::vec3(0.0f), 0);
	AddSprite(frame, {0.0f, 0.0f, 50.0f}, 1);
	AddSprite(frame, {0.0f, 0.0f, 55.0f}, 1);
	AddSprite(frame, {0.0f, 0.0f, 40.0f}, 0);
	AddGroup(frame, DrawPath::Queued, {0.0f, 0.0f, 50.0f}, 3);

	std::vector<Command> commands;
	std::vector<uint32_t> order;
	Order(frame, camera, std::nullopt, commands, order);
	ASSERT_EQ(commands.size(), 4u);
	// The two far ones in one draw, the farthest first, at the farthest's place
	EXPECT_EQ(commands[0].count, 2u);
	EXPECT_EQ(order[commands[0].first], 0u);
	EXPECT_EQ(order[commands[0].first + 1], 2u);
	EXPECT_EQ(commands[0].depth, graphics::zsort::Depth({0.0f, 0.0f, 100.0f}, camera));
	EXPECT_EQ(commands[1].count, 1u);
	EXPECT_EQ(order[commands[1].first], 1u);
	// The queued effect in its own order, all at its origin, each draw a little nearer than the one before
	EXPECT_EQ(commands[2].count, 2u);
	EXPECT_EQ(order[commands[2].first], 3u);
	EXPECT_EQ(order[commands[2].first + 1], 4u);
	EXPECT_EQ(commands[2].depth, graphics::zsort::Depth({0.0f, 0.0f, 50.0f}, camera));
	EXPECT_EQ(commands[3].depth, commands[2].depth - 1);
	// Sorted by depth, the greatest first, they come in this order: far sorted, queued, near sorted
	EXPECT_GT(commands[0].depth, commands[2].depth);
	EXPECT_GT(commands[3].depth, commands[1].depth);
	EXPECT_EQ(order.size(), frame.sprites.size());
}

TEST(ParticleDraw, AddedSpritesOnDifferentSheetsAreDrawnASheetAtATime)
{
	auto frame = MakeFrame();
	frame.materials.push_back(
	    {.texture = 5, .alphaTexture = 6, .mode = graphics::render_modes::Mode::AlphaTexturedAlphaAdditiveNz});
	// Far to near: two added sheets taking turns, then one blended over, then the two again
	AddSprite(frame, {0.0f, 0.0f, 100.0f}, 1);
	AddSprite(frame, {0.0f, 0.0f, 90.0f}, 2);
	AddSprite(frame, {0.0f, 0.0f, 80.0f}, 1);
	AddSprite(frame, {0.0f, 0.0f, 70.0f}, 2);
	AddSprite(frame, {0.0f, 0.0f, 60.0f}, 0);
	AddSprite(frame, {0.0f, 0.0f, 50.0f}, 2);
	AddSprite(frame, {0.0f, 0.0f, 40.0f}, 1);
	AddGroup(frame, DrawPath::Sorted, glm::vec3(0.0f), 0);
	std::vector<Command> commands;
	std::vector<uint32_t> order;
	Order(frame, glm::vec3(0.0f), std::nullopt, commands, order);
	// Adding is the same in any order, so each run takes a draw a sheet; the blended one keeps its place between them
	ASSERT_EQ(commands.size(), 5u);
	EXPECT_EQ(commands[0].material, 1u);
	EXPECT_EQ(commands[0].count, 2u);
	EXPECT_EQ(commands[1].material, 2u);
	EXPECT_EQ(commands[1].count, 2u);
	EXPECT_EQ(commands[2].material, 0u);
	EXPECT_EQ(commands[3].material, 2u);
	EXPECT_EQ(commands[4].material, 1u);
	EXPECT_EQ(order, (std::vector<uint32_t> {0, 2, 1, 3, 4, 5, 6}));
	for (size_t i = 1; i < commands.size(); ++i)
	{
		EXPECT_LT(commands[i].depth, commands[i - 1].depth);
	}
}

TEST(ParticleDraw, TheMiracleInTheHandIsDrawnJustAfterTheHand)
{
	auto frame = MakeFrame();
	AddSprite(frame, {0.0f, 0.0f, 5.0f}, 0);
	AddSprite(frame, {0.0f, 0.0f, 6.0f}, 1);
	AddGroup(frame, DrawPath::Immediate, {0.0f, 0.0f, 300.0f}, 0);
	std::vector<Command> commands;
	std::vector<uint32_t> order;
	const glm::vec3 hand(0.0f, 0.0f, 20.0f);
	Order(frame, glm::vec3(0.0f), hand, commands, order);
	ASSERT_EQ(commands.size(), 2u);
	const auto handDepth = graphics::zsort::Depth(hand, glm::vec3(0.0f));
	EXPECT_EQ(commands[0].depth, handDepth - 1);
	EXPECT_EQ(commands[1].depth, handDepth - 2);
	// Without a hand, at its origin
	Order(frame, glm::vec3(0.0f), std::nullopt, commands, order);
	EXPECT_EQ(commands[0].depth, graphics::zsort::Depth({0.0f, 0.0f, 300.0f}, glm::vec3(0.0f)));
}

TEST(ParticleDraw, RibbonsMatchTheGamesStrip)
{
	ChainCreator creator;
	creator.kind = Creator::Kind::Chain;
	const std::vector<glm::vec3> points {{0.0f, 0.0f, 0.0f}, {10.0f, 2.0f, 0.0f}, {20.0f, 0.0f, 5.0f}, {30.0f, 5.0f, 5.0f}};
	const std::vector<float> scales {1.0f, 2.0f, 1.5f, 0.5f};
	std::vector<Effect::DrawAtom> joints;
	for (size_t i = 0; i < points.size(); ++i)
	{
		joints.push_back({.creator = &creator,
		                  .position = points[i],
		                  .rotation = glm::mat3(1.0f),
		                  .scale = scales[i],
		                  .stretch = 1.0f,
		                  .alpha = 200.0f,
		                  .frame = 0.0f,
		                  .rgb = {255, 128, 0}});
	}
	std::vector<ChainVertex> vertices;
	AppendChain(vertices, creator, joints);
	ASSERT_EQ(vertices.size(), 12u);
	for (const glm::vec3 eye : {glm::vec3(5.0f, 50.0f, -40.0f), glm::vec3(-30.0f, 2.0f, 60.0f)})
	{
		const auto expected = GameRibbon(points, scales, eye);
		for (size_t i = 0; i < vertices.size(); ++i)
		{
			ExpectNear(ChainCorner(vertices[i], eye), expected[i], 1e-3f);
		}
	}
	EXPECT_EQ(vertices[0].abgr, (200u << 24u) | (0u << 16u) | (128u << 8u) | 255u);
}

TEST(ParticleDraw, RibbonFramesRepeatAlongTheChain)
{
	ChainCreator creator;
	creator.frameWidth = 32;
	creator.frameHeight = 64;
	creator.fileOffset = 1;
	creator.frameOfHead = 2;
	creator.frameOfTail = 4;
	creator.texturesForWholeChain = 2;
	// Four segments in two repeats: the first two show the tail's frame, the last two the head's
	const auto first = creator.SegmentUv(0, 4);
	EXPECT_FLOAT_EQ(first[0].x, 5.0f * 32.0f / 256.0f);
	EXPECT_FLOAT_EQ(first[1].x, 6.0f * 32.0f / 256.0f);
	EXPECT_FLOAT_EQ(first[0].y, 0.0f);
	EXPECT_FLOAT_EQ(first[2].y, 32.0f / 256.0f);
	const auto second = creator.SegmentUv(1, 4);
	EXPECT_FLOAT_EQ(second[0].y, 32.0f / 256.0f);
	EXPECT_FLOAT_EQ(second[3].y, 64.0f / 256.0f);
	const auto last = creator.SegmentUv(3, 4);
	EXPECT_FLOAT_EQ(last[0].x, 3.0f * 32.0f / 256.0f);
	// One repeat a segment by default
	creator.texturesForWholeChain = -1;
	EXPECT_FLOAT_EQ(creator.SegmentUv(2, 4)[2].y, 64.0f / 256.0f);
}

TEST(ParticleDraw, PlayersSymbolsGlowInTheirColour)
{
	Creator creator;
	Effect::DrawAtom atom {.creator = &creator,
	                       .position = {1.0f, 2.0f, 3.0f},
	                       .rotation = glm::mat3(1.0f),
	                       .scale = 2.0f,
	                       .stretch = 1.0f,
	                       .alpha = 255.0f,
	                       .frame = 0.0f,
	                       .rgb = {255, 255, 255},
	                       .age = 0.0f};
	auto symbol = SymbolOf(atom, 0xFF4646u, 2);
	// The glows half as big again, faint, the first in the player's colour and the second white
	EXPECT_FLOAT_EQ(symbol.instances[0].positionHalfWidth.w, 3.0f);
	EXPECT_FLOAT_EQ(symbol.instances[2].positionHalfWidth.w, 2.0f);
	EXPECT_NEAR(symbol.instances[0].colour.a, 99.0f / 255.0f, k_Epsilon);
	EXPECT_NEAR(symbol.instances[0].colour.r, 254.0f / 255.0f, k_Epsilon);
	EXPECT_NEAR(symbol.instances[0].colour.g, 69.0f / 255.0f, k_Epsilon);
	EXPECT_FLOAT_EQ(symbol.instances[1].colour.g, 1.0f);
	EXPECT_NEAR(symbol.instances[2].colour.a, 254.0f / 255.0f, k_Epsilon);
	// The player's cell of four to a row
	EXPECT_FLOAT_EQ(symbol.instances[2].uv.x, 0.5f);
	EXPECT_FLOAT_EQ(symbol.instances[2].uv.z, 0.25f);
	// After a second the glows have run back through their frames and the second has turned two radians
	atom.age = 1.0f;
	symbol = SymbolOf(atom, 0xFF4646u, 2);
	EXPECT_NEAR(symbol.instances[1].shape.y, 2.0f, k_Epsilon);
	EXPECT_FLOAT_EQ(symbol.instances[0].shape.y, 0.0f);
	// 32 - 20 = cell 12, and 32 - 23 = cell 9, of eight to a row
	EXPECT_FLOAT_EQ(symbol.instances[0].uv.x, 4.0f / 8.0f);
	EXPECT_FLOAT_EQ(symbol.instances[0].uv.y, 1.0f / 8.0f);
	EXPECT_FLOAT_EQ(symbol.instances[1].uv.x, 1.0f / 8.0f);
}

TEST(ParticleDraw, AnimatedTexturesMoveOnByCellsOrSlide)
{
	MeshCreator creator;
	creator.textureWidth = 64;
	creator.textureHeight = 32;
	// Four cells to a row
	EXPECT_FLOAT_EQ(creator.UvOffset(5).x, 0.25f);
	EXPECT_FLOAT_EQ(creator.UvOffset(5).y, 0.125f);
	creator.slideV = true;
	EXPECT_FLOAT_EQ(creator.UvOffset(500).x, 0.0f);
	EXPECT_FLOAT_EQ(creator.UvOffset(500).y, 32.0f * 500.0f / (1000.0f * 256.0f));
}

TEST(ParticleDraw, AChakraRisesThenFades)
{
	EXPECT_FLOAT_EQ(maths::ChakraFade(0.25f, 0.5f, 1.5f), 0.5f);
	EXPECT_FLOAT_EQ(maths::ChakraFade(0.5f, 0.5f, 1.5f), 1.0f);
	EXPECT_FLOAT_EQ(maths::ChakraFade(1.0f, 0.5f, 1.5f), 0.5f);
	EXPECT_FLOAT_EQ(maths::ChakraFade(2.0f, 0.5f, 1.5f), 0.0f);
	// Not a number counts as nothing
	EXPECT_FLOAT_EQ(maths::ChakraFade(0.0f, 0.0f, 0.0f), 0.0f);
}
