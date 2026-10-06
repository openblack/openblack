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

#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

/// What a shader program samples, read from its compiled shaders, so that every sampler it declares can be bound on
/// every draw: Vulkan leaves a sampler nothing was bound to pointing wherever its descriptor's memory last pointed,
/// which can be a texture since freed, and reading it can hang the GPU even when the shader skips the read.
namespace openblack::graphics::shader_samplers
{

/// bgfx binds textures at stages 0 to 15
constexpr uint8_t k_MaxStages = 16;

/// The kind of texture a sampler reads
enum class Dimension : uint8_t
{
	Unknown, ///< Shaders compiled before the compiler recorded it
	Texture1D,
	Texture2D,
	Texture2DArray,
	Cube,
	CubeArray,
	Texture3D,
};

/// The textures a draw's unset samplers get, one of each kind a sampler can read
enum class DefaultTexture : uint8_t
{
	White2D,
	White2DArray,
	WhiteCube,
	White3D,
};

struct Sampler
{
	std::string name;
	uint8_t stage;
	Dimension dimension;

	bool operator==(const Sampler&) const = default;
};

/// The samplers a shader uses, from its bgfx binary compiled for Vulkan (SPIR-V), each with the stage it is bound at.
/// A sampler is bound at the stage its shader source declares on every backend, so the SPIR-V build serves them all,
/// and its compiler keeps only the samplers the shader reads. Nothing when the binary isn't a shader this can read.
[[nodiscard]] std::optional<std::vector<Sampler>> ReadSpirvSamplers(std::span<const uint8_t> binary);

/// The default texture for a sampler of the given kind: white, so that an unset texture modulates nothing. A 1D
/// sampler reads a 2D texture (bgfx has no 1D textures), a cube array reads the cube and an unknown kind the 2D one.
[[nodiscard]] DefaultTexture DefaultTextureFor(Dimension dimension) noexcept;

/// A program's samplers: its vertex shader's, then the fragment shader's that the vertex shader doesn't share by name
[[nodiscard]] std::vector<Sampler> Merge(std::span<const Sampler> vertex, std::span<const Sampler> fragment);

/// The pairs of differently named samplers a program reads at the same stage, where one texture would replace the other
[[nodiscard]] std::vector<std::pair<std::string, std::string>> StageCollisions(std::span<const Sampler> samplers);

} // namespace openblack::graphics::shader_samplers
