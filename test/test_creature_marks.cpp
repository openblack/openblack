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
#include <vector>

#include <gtest/gtest.h>

#include "Creature/CreatureMarks.h"
#include "Creature/CreatureSkin.h"

using namespace openblack;
using namespace openblack::creature_marks;

namespace
{
constexpr size_t k_SkinTexels = 256 * 256;

/// Damage atlases of one colour and alpha each
DamageArt FakeArt(std::array<uint8_t, 3> fresh, uint8_t freshAlpha, std::array<uint8_t, 3> old, uint8_t oldAlpha)
{
	const auto texels = static_cast<size_t>(k_AtlasSize) * k_AtlasSize;
	return {
	    .fresh = {.colours = std::vector(texels, fresh), .alpha = std::vector(texels, freshAlpha)},
	    .old = {.colours = std::vector(texels, old), .alpha = std::vector(texels, oldAlpha)},
	};
}
} // namespace

TEST(CreatureMarks, AFullListLosesItsOldestMark)
{
	std::vector<Mark> marks;
	for (size_t i = 0; i < k_MaxMarks; ++i)
	{
		Add(marks, {.u = 0, .v = 0, .skin = 0, .age = static_cast<uint8_t>(i == 10 ? 50 : 3), .type = 0, .column = 0});
	}
	ASSERT_EQ(marks.size(), k_MaxMarks);
	const Mark fresh {.u = 9, .v = 9, .skin = 1, .age = 0, .type = 3, .column = 2};
	Add(marks, fresh);
	EXPECT_EQ(marks.size(), k_MaxMarks);
	EXPECT_EQ(marks.at(10), fresh);
}

TEST(CreatureMarks, MarksAgeEvery600Counts)
{
	Marks marks;
	Add(marks.wounds, {.u = 0, .v = 0, .skin = 0, .age = 0, .type = 3, .column = 0});
	Add(marks.blood, {.u = 0, .v = 0, .skin = 0, .age = 0, .type = 0, .column = 0});
	EXPECT_FALSE(Heal(marks, 599));
	EXPECT_EQ(marks.wounds[0].age, 0);
	EXPECT_TRUE(Heal(marks, 1));
	EXPECT_EQ(marks.wounds[0].age, 1);
	EXPECT_EQ(marks.blood[0].age, 1);
	EXPECT_EQ(marks.counts, 0u);
	// A heal effect's counts age them many steps at once
	EXPECT_TRUE(Heal(marks, k_CountsPerStep * 10 + 5));
	EXPECT_EQ(marks.wounds[0].age, 11);
	EXPECT_EQ(marks.counts, 5u);
}

TEST(CreatureMarks, MarksGoOnceTheirKindHasLasted)
{
	Marks marks;
	// A wound of the first kind lasts 32 steps, the last kind one, the others 64; blood 63
	Add(marks.wounds, {.u = 0, .v = 0, .skin = 0, .age = 31, .type = 0, .column = 0});
	Add(marks.wounds, {.u = 0, .v = 0, .skin = 0, .age = 31, .type = 3, .column = 0});
	Add(marks.wounds, {.u = 0, .v = 0, .skin = 0, .age = 0, .type = 7, .column = 0});
	Add(marks.blood, {.u = 0, .v = 0, .skin = 0, .age = 62, .type = 0, .column = 0});
	Add(marks.blood, {.u = 0, .v = 0, .skin = 0, .age = 61, .type = 0, .column = 0});
	Heal(marks, k_CountsPerStep);
	ASSERT_EQ(marks.wounds.size(), 1u);
	EXPECT_EQ(marks.wounds[0].type, 3);
	ASSERT_EQ(marks.blood.size(), 1u);
	EXPECT_EQ(marks.blood[0].age, 62);
}

TEST(CreatureMarks, WoundsFadeFromFreshToOld)
{
	const auto art = FakeArt({200, 0, 0}, 255, {0, 100, 0}, 55);
	const Mark fresh {.u = 0, .v = 0, .skin = 0, .age = 0, .type = 3, .column = 0};
	EXPECT_EQ(WoundTexel(art, fresh, 0, 0), glm::u8vec4(200, 0, 0, 255));
	// Half way through its 64 steps: half way between, in 256ths
	auto half = fresh;
	half.age = 32;
	EXPECT_EQ(WoundTexel(art, half, 0, 0), glm::u8vec4(100, 50, 0, 155));
	auto old = fresh;
	old.age = 64;
	EXPECT_EQ(WoundTexel(art, old, 0, 0), glm::u8vec4(0, 100, 0, 55));
}

TEST(CreatureMarks, ColoursBlendAt8BitsAndCutTo4)
{
	EXPECT_EQ(BlendTexel(0x0123, {0xFF, 0xFF, 0xFF}, 255), 0xFFFF);
	EXPECT_EQ(BlendTexel(0x0123, {0xFF, 0xFF, 0xFF}, 0), 0xF123);
	// Blue: (0x30 * 128 + 0xC0 * 127) / 255 = 119.8, so 119 >> 4 = 7
	EXPECT_EQ(BlendTexel(0x0003, {0, 0, 0xC0}, 127) & 0xF, 7);
}

TEST(CreatureMarks, WoundsAreCentredAndCutAtTheEdges)
{
	std::vector<uint16_t> skin(k_SkinTexels, 0x0000);
	const auto art = FakeArt({0xFF, 0xFF, 0xFF}, 255, {0xFF, 0xFF, 0xFF}, 255);
	PaintWound(skin, art, {.u = 4, .v = 100, .skin = 0, .age = 0, .type = 3, .column = 1});
	const auto at = [&skin](int x, int y) { return skin.at(static_cast<size_t>((y * 256) + x)); };
	// 32 texels square about its place, the part off the left edge left out
	EXPECT_EQ(at(0, 84), 0xFFFF);
	EXPECT_EQ(at(19, 115), 0xFFFF);
	EXPECT_EQ(at(20, 100), 0x0000);
	EXPECT_EQ(at(5, 83), 0x0000);
	EXPECT_EQ(at(255, 100), 0x0000);
}

TEST(CreatureMarks, BloodDarkensAsItDries)
{
	EXPECT_EQ(BloodColour(0), glm::u8vec3(150, 0, 20));
	EXPECT_EQ(BloodColour(21), glm::u8vec3(150, 0, 20));
	EXPECT_EQ(BloodColour(22), glm::u8vec3(120, 20, 20));
	EXPECT_EQ(BloodColour(43), glm::u8vec3(80, 40, 0));
	std::vector<uint16_t> skin(k_SkinTexels, 0x0FFF);
	PaintBlood(skin, {.u = 3, .v = 2, .skin = 0, .age = 0, .type = 0, .column = 0});
	// Three quarters red over white
	EXPECT_EQ(skin.at((2 * 256) + 3), BlendTexel(0x0FFF, {150, 0, 20}, k_BloodAlpha));
	EXPECT_EQ(skin.at((2 * 256) + 4), 0x0FFF);
}

TEST(CreatureMarks, SkinsArePaintedTattoosThenWoundsThenBlood)
{
	using namespace openblack::creature_skin;
	auto art = std::make_unique<Art>();
	art->damage = FakeArt({0, 0, 0}, 255, {0, 0, 0}, 255);
	for (size_t d = 0; d < art->designs.size(); ++d)
	{
		for (size_t level = 0; level < creature_tattoo::k_MaskLevels; ++level)
		{
			const auto size = creature_tattoo::k_DesignSize >> level;
			art->designs.at(d).at(level) = {.size = size, .levels = std::vector<uint8_t>(static_cast<size_t>(size) * size, 15)};
		}
	}
	creature_tattoo::Slots tattoos {};
	tattoos[0] = {.design = 1, .site = 0, .colour = {0xFF, 0xFF, 0xFF}};
	std::optional<creature_tattoo::Sites> sites = creature_tattoo::Sites {};
	sites->at(0) = {.enabled = true, .u = 128, .v = 128, .skin = 0, .size = 1.0f, .mirror = false, .rotation = 0};
	Marks marks;
	Add(marks.wounds, {.u = 128, .v = 128, .skin = 0, .age = 0, .type = 3, .column = 0});
	Add(marks.blood, {.u = 128, .v = 128, .skin = 0, .age = 0, .type = 0, .column = 0});
	// A wound on another skin isn't painted on this one
	Add(marks.wounds, {.u = 10, .v = 10, .skin = 1, .age = 0, .type = 3, .column = 0});

	const std::vector<uint16_t> base(k_SkinTexels, 0x0000);
	const std::vector<uint16_t> variant(k_SkinTexels, 0x0FFF);
	std::vector<uint16_t> out(k_SkinTexels);
	Compose(out, base, variant, 255, {.skinIndex = 0, .tattoos = tattoos, .sites = sites, .marks = marks, .art = art.get()});
	const auto at = [&out](int x, int y) { return out.at(static_cast<size_t>((y * 256) + x)); };
	// The variant at full weight, white; a white tattoo over the middle; a black wound over that; blood on top
	EXPECT_EQ(at(5, 5), 0x0FFF);
	EXPECT_EQ(at(100, 100), 0xFFFF);
	EXPECT_EQ(at(120, 120), 0xF000);
	EXPECT_EQ(at(128, 128), BlendTexel(0xF000, BloodColour(0), k_BloodAlpha));
	EXPECT_EQ(at(10, 10), 0x0FFF);

	// Without the art, the blend alone
	Compose(out, base, variant, 0, {.skinIndex = 0, .tattoos = tattoos, .sites = sites, .marks = marks, .art = nullptr});
	EXPECT_EQ(at(128, 128), 0x0000);
}
