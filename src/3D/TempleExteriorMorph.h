/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <span>
#include <string>

namespace openblack::TempleExteriorMorph
{

/// A temple's outside is three sizes, by its player's share of influence, of five stages each, from evil to good: the
/// meshes B_TEMPLE<size><stage>, all of the same shape. Its mesh is a blend of the four about where it is.
constexpr uint32_t k_Sizes = 3;
constexpr uint32_t k_Stages = 5;

/// Citadel::Process moves each a step a turn toward where it is to be, and the rest of the way when that is near
constexpr float k_Step = 0.016f;
constexpr float k_Near = 0.001f;
[[nodiscard]] float Step(float current, float target);

/// One of the meshes a temple's is blended from, and how much of it
struct Corner
{
	uint32_t size;
	uint32_t stage;
	float weight;
};
/// fn_00882B10's meshes for a size and an alignment, each from 0 to 1, and their weights
[[nodiscard]] std::array<Corner, 4> Corners(float size, float alignment);
/// The name of a size's and stage's mesh
[[nodiscard]] std::string MeshName(uint32_t size, uint32_t stage);

/// The two images of a temple's texture blended from evil to neutral, or neutral to good, and how far, from 0 to 255
enum class Look : uint8_t
{
	Evil,
	Neutral,
	Good,
};
struct TextureBlend
{
	Look from;
	Look to;
	uint8_t weight;
};
[[nodiscard]] TextureBlend TextureOf(float alignment);
/// The name of a look's image of one of the four sets of textures
[[nodiscard]] std::string ImageName(Look look, uint32_t set);
/// Each 16 bit texel, four bits a channel, blended from one image to another by weight from 0 to 255, keeping the first
/// image's alpha
void BlendTexels(std::span<const uint16_t> from, std::span<const uint16_t> to, uint8_t weight, std::span<uint16_t> blended);

} // namespace openblack::TempleExteriorMorph
