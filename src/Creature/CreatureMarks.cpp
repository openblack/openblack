/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureMarks.h"

#include <cassert>

#include <algorithm>

using namespace openblack;
using namespace openblack::creature_marks;

namespace
{
constexpr uint32_t k_SkinSize = 256;
constexpr uint32_t k_Opaque = 0xF000;
/// The blood's colours by how old it is: up to 21 steps, up to 42, then older
constexpr std::array<glm::u8vec3, 3> k_BloodColours {glm::u8vec3(150, 0, 20), glm::u8vec3(120, 20, 20), glm::u8vec3(80, 40, 0)};
constexpr std::array<uint8_t, 2> k_BloodAges {22, 43};

uint8_t LifetimeOf(const Mark& wound)
{
	return k_WoundLifetimes.at(wound.type % k_WoundLifetimes.size());
}

bool Expired(const Mark& wound)
{
	return wound.age >= LifetimeOf(wound);
}
} // namespace

void creature_marks::Add(std::vector<Mark>& marks, const Mark& mark)
{
	if (marks.size() < k_MaxMarks)
	{
		marks.push_back(mark);
		return;
	}
	*std::ranges::max_element(marks, {}, &Mark::age) = mark;
}

bool creature_marks::Heal(Marks& marks, uint32_t counts)
{
	marks.counts += counts;
	bool aged = false;
	while (marks.counts >= k_CountsPerStep)
	{
		marks.counts -= k_CountsPerStep;
		aged = true;
		for (auto& blood : marks.blood)
		{
			++blood.age;
		}
		std::erase_if(marks.blood, [](const Mark& blood) { return blood.age >= k_BloodLifetime; });
		for (auto& wound : marks.wounds)
		{
			++wound.age;
		}
		std::erase_if(marks.wounds, Expired);
	}
	return aged;
}

glm::u8vec4 creature_marks::WoundTexel(const DamageArt& art, const Mark& wound, uint32_t x, uint32_t y)
{
	const auto column = wound.column % k_CellsPerRow;
	const auto row = wound.type % k_CellsPerRow;
	const auto index = (static_cast<size_t>((row * k_CellSize) + y) * k_AtlasSize) + (column * k_CellSize) + x;
	if (index >= art.fresh.colours.size() || index >= art.old.colours.size() || index >= art.fresh.alpha.size() ||
	    index >= art.old.alpha.size())
	{
		return glm::u8vec4(0);
	}
	// How far towards old, in 256ths
	const auto t = std::min<int32_t>((static_cast<int32_t>(wound.age) * 256) / LifetimeOf(wound), 256);
	const auto mix = [t](int32_t fresh, int32_t old) { return static_cast<uint8_t>(fresh + (((old - fresh) * t) / 256)); };
	const auto& fresh = art.fresh.colours[index];
	const auto& old = art.old.colours[index];
	return {mix(fresh[0], old[0]), mix(fresh[1], old[1]), mix(fresh[2], old[2]),
	        mix(art.fresh.alpha[index], art.old.alpha[index])};
}

uint16_t creature_marks::BlendTexel(uint16_t skin, const glm::u8vec3& colour, uint8_t alpha)
{
	const uint32_t a = alpha;
	// The skin's 4 bits widened to 8 and blended with the colour's 8, then cut back to the top 4
	const auto channel = [&](uint32_t shift, uint8_t value) {
		const uint32_t from = ((skin >> shift) & 0xFu) << 4u;
		return ((((from * (255u - a)) + (static_cast<uint32_t>(value) * a)) / 255u) >> 4u) << shift;
	};
	return static_cast<uint16_t>(channel(8, colour.r) | channel(4, colour.g) | channel(0, colour.b) | k_Opaque);
}

void creature_marks::PaintWound(std::span<uint16_t> skin, const DamageArt& art, const Mark& wound)
{
	assert(skin.size() == static_cast<size_t>(k_SkinSize) * k_SkinSize);
	const auto left = static_cast<int32_t>(wound.u) - static_cast<int32_t>(k_CellSize / 2);
	const auto top = static_cast<int32_t>(wound.v) - static_cast<int32_t>(k_CellSize / 2);
	for (uint32_t y = 0; y < k_CellSize; ++y)
	{
		const auto skinY = top + static_cast<int32_t>(y);
		if (skinY < 0 || skinY >= static_cast<int32_t>(k_SkinSize))
		{
			continue;
		}
		for (uint32_t x = 0; x < k_CellSize; ++x)
		{
			const auto skinX = left + static_cast<int32_t>(x);
			if (skinX < 0 || skinX >= static_cast<int32_t>(k_SkinSize))
			{
				continue;
			}
			const auto texel = WoundTexel(art, wound, x, y);
			if (texel.a == 0)
			{
				continue;
			}
			auto& target = skin[(static_cast<size_t>(skinY) * k_SkinSize) + static_cast<size_t>(skinX)];
			target = BlendTexel(target, glm::u8vec3(texel), texel.a);
		}
	}
}

glm::u8vec3 creature_marks::BloodColour(uint8_t age)
{
	if (age < k_BloodAges[0])
	{
		return k_BloodColours[0];
	}
	return age < k_BloodAges[1] ? k_BloodColours[1] : k_BloodColours[2];
}

void creature_marks::PaintBlood(std::span<uint16_t> skin, const Mark& blood)
{
	assert(skin.size() == static_cast<size_t>(k_SkinSize) * k_SkinSize);
	auto& target = skin[(static_cast<size_t>(blood.v) * k_SkinSize) + blood.u];
	target = BlendTexel(target, BloodColour(blood.age), k_BloodAlpha);
}
