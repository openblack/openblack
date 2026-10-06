/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdint>

#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Graphics/ShaderSamplers.h"

using namespace openblack::graphics::shader_samplers;

namespace
{

constexpr uint8_t k_Vec4 = 2;
constexpr uint8_t k_SamplerType = 0x20;
constexpr uint8_t k_FragmentFlag = 0x10;
constexpr uint8_t k_StorageImage = 1; // the reserved kind bgfx gives storage images
constexpr uint8_t k_Dim2D = 2;
constexpr uint8_t k_Dim2DArray = 3;

/// A fake bgfx shader binary header, as its shader compiler writes one
class FakeShader
{
public:
	FakeShader(char kind, uint8_t version)
	    : _version(version)
	{
		_bytes = {static_cast<uint8_t>(kind), 'S', 'H', version};
		Write32(0x12345678); // input hash
		if (version >= 6)
		{
			Write32(0x9ABCDEF0); // output hash
		}
		_countAt = _bytes.size();
		Write16(0);
	}

	FakeShader& Uniform(std::string_view name, uint8_t type, uint16_t binding, uint8_t dimension = 0)
	{
		_bytes.push_back(static_cast<uint8_t>(name.size()));
		_bytes.insert(_bytes.end(), name.begin(), name.end());
		_bytes.push_back(type);
		_bytes.push_back(1); // array size
		Write16(binding);
		Write16(0); // register count
		if (_version >= 8)
		{
			_bytes.push_back(1); // component type
			_bytes.push_back(dimension);
		}
		if (_version >= 10)
		{
			Write16(0); // texture format
		}
		++_count;
		_bytes[_countAt] = static_cast<uint8_t>(_count & 0xFF);
		_bytes[_countAt + 1] = static_cast<uint8_t>(_count >> 8);
		return *this;
	}

	[[nodiscard]] const std::vector<uint8_t>& Bytes() const { return _bytes; }

private:
	void Write16(uint16_t value)
	{
		_bytes.push_back(static_cast<uint8_t>(value & 0xFF));
		_bytes.push_back(static_cast<uint8_t>(value >> 8));
	}
	void Write32(uint32_t value)
	{
		Write16(static_cast<uint16_t>(value & 0xFFFF));
		Write16(static_cast<uint16_t>(value >> 16));
	}

	uint8_t _version;
	std::vector<uint8_t> _bytes;
	size_t _countAt = 0;
	uint16_t _count = 0;
};

} // namespace

TEST(ShaderSamplers, ReadsTheSamplersWithTheirStagesAndKinds)
{
	// Version 11 bindings sit two past the stage
	const auto shader = FakeShader('F', 11)
	                        .Uniform("u_colour", k_Vec4, 0)
	                        .Uniform("s_diffuse", k_SamplerType | k_FragmentFlag, 2, k_Dim2D)
	                        .Uniform("s_blocks", k_SamplerType | k_FragmentFlag, 2 + 13, k_Dim2DArray);
	const auto samplers = ReadSpirvSamplers(shader.Bytes());
	ASSERT_TRUE(samplers.has_value());
	const std::vector<Sampler> expected {
	    {.name = "s_diffuse", .stage = 0, .dimension = Dimension::Texture2D},
	    {.name = "s_blocks", .stage = 13, .dimension = Dimension::Texture2DArray},
	};
	EXPECT_EQ(*samplers, expected);
}

TEST(ShaderSamplers, OlderBinariesOffsetTheStagesByShaderKind)
{
	const auto vertex = FakeShader('V', 10).Uniform("s_heightmap", k_SamplerType, 16 + 1, k_Dim2D);
	const auto fragment = FakeShader('F', 10).Uniform("s_alpha", k_SamplerType | k_FragmentFlag, 48 + 16 + 3, k_Dim2D);
	const auto vertexSamplers = ReadSpirvSamplers(vertex.Bytes());
	const auto fragmentSamplers = ReadSpirvSamplers(fragment.Bytes());
	ASSERT_TRUE(vertexSamplers.has_value());
	ASSERT_TRUE(fragmentSamplers.has_value());
	ASSERT_EQ(vertexSamplers->size(), 1);
	ASSERT_EQ(fragmentSamplers->size(), 1);
	EXPECT_EQ(vertexSamplers->front().stage, 1);
	EXPECT_EQ(fragmentSamplers->front().stage, 3);
}

TEST(ShaderSamplers, BinariesWithoutTextureInfoHaveUnknownKinds)
{
	const auto shader = FakeShader('V', 7).Uniform("s_texture", k_SamplerType, 16 + 4);
	const auto samplers = ReadSpirvSamplers(shader.Bytes());
	ASSERT_TRUE(samplers.has_value());
	ASSERT_EQ(samplers->size(), 1);
	EXPECT_EQ(samplers->front().stage, 4);
	EXPECT_EQ(samplers->front().dimension, Dimension::Unknown);
}

TEST(ShaderSamplers, SkipsStorageImagesAndStagesOutOfRange)
{
	const auto shader = FakeShader('F', 11)
	                        .Uniform("s_image", k_StorageImage, 2 + 1, k_Dim2D)
	                        .Uniform("s_far", k_SamplerType, 2 + 16, k_Dim2D)
	                        .Uniform("s_near", k_SamplerType, 2 + 15, k_Dim2D);
	const auto samplers = ReadSpirvSamplers(shader.Bytes());
	ASSERT_TRUE(samplers.has_value());
	ASSERT_EQ(samplers->size(), 1);
	EXPECT_EQ(samplers->front().name, "s_near");
}

TEST(ShaderSamplers, RejectsWhatIsNotAShader)
{
	const std::vector<uint8_t> notAShader {'P', 'N', 'G', 11, 0, 0, 0, 0};
	EXPECT_FALSE(ReadSpirvSamplers(notAShader).has_value());
	EXPECT_FALSE(ReadSpirvSamplers({}).has_value());

	// Cut off in the middle of a uniform
	auto truncated = FakeShader('F', 11).Uniform("s_diffuse", k_SamplerType, 2, k_Dim2D).Bytes();
	truncated.resize(truncated.size() - 3);
	EXPECT_FALSE(ReadSpirvSamplers(truncated).has_value());
}

TEST(ShaderSamplers, EachKindGetsAWhiteTextureOfItsKind)
{
	EXPECT_EQ(DefaultTextureFor(Dimension::Texture2D), DefaultTexture::White2D);
	EXPECT_EQ(DefaultTextureFor(Dimension::Texture2DArray), DefaultTexture::White2DArray);
	EXPECT_EQ(DefaultTextureFor(Dimension::Cube), DefaultTexture::WhiteCube);
	EXPECT_EQ(DefaultTextureFor(Dimension::CubeArray), DefaultTexture::WhiteCube);
	EXPECT_EQ(DefaultTextureFor(Dimension::Texture3D), DefaultTexture::White3D);
	// bgfx has no 1D textures, and a kind not recorded is most likely 2D
	EXPECT_EQ(DefaultTextureFor(Dimension::Texture1D), DefaultTexture::White2D);
	EXPECT_EQ(DefaultTextureFor(Dimension::Unknown), DefaultTexture::White2D);
}

TEST(ShaderSamplers, AProgramSharesTheSamplersOfBothShadersOnce)
{
	const std::vector<Sampler> vertex {
	    {.name = "s_heightmap", .stage = 1, .dimension = Dimension::Texture2D},
	    {.name = "s_landLight", .stage = 7, .dimension = Dimension::Texture2D},
	};
	const std::vector<Sampler> fragment {
	    {.name = "s_diffuse", .stage = 0, .dimension = Dimension::Texture2D},
	    {.name = "s_landLight", .stage = 7, .dimension = Dimension::Texture2D},
	};
	const auto merged = Merge(vertex, fragment);
	ASSERT_EQ(merged.size(), 3);
	EXPECT_EQ(merged[0].name, "s_heightmap");
	EXPECT_EQ(merged[1].name, "s_landLight");
	EXPECT_EQ(merged[2].name, "s_diffuse");
	EXPECT_TRUE(StageCollisions(merged).empty());
}

TEST(ShaderSamplers, FindsSamplersSharingAStage)
{
	const std::vector<Sampler> samplers {
	    {.name = "s_landColour", .stage = 8, .dimension = Dimension::Texture2D},
	    {.name = "s_landAlpha", .stage = 8, .dimension = Dimension::Texture2D},
	    {.name = "s_diffuse", .stage = 0, .dimension = Dimension::Texture2D},
	};
	const auto collisions = StageCollisions(samplers);
	ASSERT_EQ(collisions.size(), 1);
	EXPECT_EQ(collisions.front().first, "s_landColour");
	EXPECT_EQ(collisions.front().second, "s_landAlpha");
}
