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

/// The game's 19 render modes: each sets a draw's blending, alpha test, depth write and how the texture's alpha is
/// combined with the vertex colour. An L3D material's type is the number of the mode it is drawn in.
namespace openblack::graphics::render_modes
{

enum class Mode : uint8_t
{
	Smooth = 0,
	SmoothAlpha = 1,
	Textured = 2,
	TexturedAlpha = 3,
	AlphaTextured = 4,
	AlphaTexturedAlpha = 5,
	AlphaTexturedAlphaNz = 6,
	SmoothAlphaNz = 7,
	TexturedAlphaNz = 8,
	TexturedChroma = 9,
	AlphaTexturedAlphaAdditiveChroma = 10,
	AlphaTexturedAlphaAdditiveChromaNz = 11,
	AlphaTexturedAlphaAdditive = 12,
	AlphaTexturedAlphaAdditiveNz = 13,
	Landscape = 14, ///< the land blocks, drawn as AlphaTexturedAlpha
	TexturedChromaAlpha = 15,
	TexturedChromaAlphaNz = 16,
	Mode17 = 17, ///< drawn as Textured; no material uses it
	ChromaJustZ = 18,
};
inline constexpr uint32_t k_ModeCount = 19;

enum class Blend : uint8_t
{
	Disabled,
	Standard, ///< source alpha, 1 - source alpha
	Additive, ///< source alpha, 1
	JustZ,    ///< nothing of the colour is written, only the depth where the alpha test passes
};

struct ModeDesc
{
	Blend blend;
	/// Fragments whose alpha is below the material's reference are dropped
	bool alphaTest;
	bool zWrite;
	/// The alpha is the texture's times the vertex colour's, else the texture's alone (or none without a texture)
	bool alphaModulate;
	/// Whether the mode draws the material's texture
	bool textured;
};

// clang-format off
inline constexpr std::array<ModeDesc, k_ModeCount> k_Modes = {{
	{Blend::Disabled, false, true,  false, false}, // 0
	{Blend::Standard, false, true,  false, false}, // 1
	{Blend::Disabled, false, true,  false, true},  // 2
	{Blend::Standard, false, true,  true,  true},  // 3
	{Blend::Standard, false, true,  false, true},  // 4
	{Blend::Standard, false, true,  true,  true},  // 5
	{Blend::Standard, false, false, true,  true},  // 6
	{Blend::Standard, false, false, false, false}, // 7
	{Blend::Standard, false, false, true,  true},  // 8
	{Blend::Standard, true,  true,  false, true},  // 9
	{Blend::Additive, true,  true,  true,  true},  // 10
	{Blend::Additive, true,  false, true,  true},  // 11
	{Blend::Additive, false, true,  true,  true},  // 12
	{Blend::Additive, false, false, true,  true},  // 13
	{Blend::Standard, false, true,  true,  true},  // 14, as 5
	{Blend::Standard, true,  true,  true,  true},  // 15
	{Blend::Standard, true,  false, true,  true},  // 16
	{Blend::Disabled, false, true,  false, true},  // 17, as 2
	{Blend::JustZ,    true,  true,  false, true},  // 18
}};
// clang-format on

[[nodiscard]] constexpr const ModeDesc& Desc(Mode mode)
{
	return k_Modes.at(static_cast<uint8_t>(mode));
}

} // namespace openblack::graphics::render_modes
