/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <bit>

#include <GLWFile.h>
#include <L3DFile.h>
#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "3D/Light.h"
#include "Graphics/LightBeams.h"

using namespace openblack;
using namespace openblack::graphics;

namespace
{
/// A light of a .glw file, at y 33 with its axes those of the world, pointing its cone down the y axis
glw::Glow MakeLight(uint32_t type, uint32_t flags, glm::vec3 colour, float size)
{
	glw::Glow light {};
	light.size = sizeof(glw::Glow);
	light.unk1 = type;
	light.red = colour.r;
	light.green = colour.g;
	light.blue = colour.b;
	light.posX = 1.0f;
	light.posY = 33.0f;
	light.posZ = 2.0f;
	// The light's x, z and y axes (its cone runs down the third), then its position
	light.unk14 = 1.0f;
	light.unk19 = 1.0f;
	light.unk21 = 1.0f;
	light.unk23 = 1.0f;
	light.unk24 = 33.0f;
	light.unk25 = 2.0f;
	light.unk26 = 60.0f;
	light.dirX = std::bit_cast<float>(flags);
	light.dirY = 90.0f;
	light.emitterSize = size;
	return light;
}
} // namespace

TEST(LightEmitter, SizesAndColoursTheGlowByTheLight)
{
	// A candle of the temple: an orange light of size 8
	const auto emitter = MakeLightEmitter(MakeLight(0, 0, {1.0f, 0.65f, 0.0f}, 8.0f));
	const float size = glm::length(glm::vec3(1.0f, 0.65f, 0.0f)) * 8.0f * 0.05f;
	EXPECT_FLOAT_EQ(emitter.glow.haloSize, size * 2.0f);
	EXPECT_FLOAT_EQ(emitter.glow.centreSize, size * 1.2f);
	// A colour of 1 is a byte of 128, and the centre is half of it over 128
	EXPECT_FLOAT_EQ(emitter.glow.haloColour.r, 128.0f / 255.0f);
	EXPECT_FLOAT_EQ(emitter.glow.haloColour.g, 83.0f / 255.0f);
	EXPECT_FLOAT_EQ(emitter.glow.centreColour.r, 192.0f / 255.0f);
	EXPECT_FLOAT_EQ(emitter.glow.centreColour.b, 128.0f / 255.0f);
	EXPECT_FALSE(emitter.glow.orientation.has_value());
	EXPECT_FALSE(emitter.cone.has_value());
}

TEST(LightEmitter, SaturatesBrightColoursAndKeepsLightsOfNoSizeUnseen)
{
	const auto emitter = MakeLightEmitter(MakeLight(0, 0, {0.5f, 1.37f, 2.5f}, 0.0f));
	EXPECT_FLOAT_EQ(emitter.glow.haloColour.r, 64.0f / 255.0f);
	EXPECT_FLOAT_EQ(emitter.glow.haloColour.b, 1.0f);
	EXPECT_LT(emitter.glow.haloSize, 0.001f);
}

TEST(LightEmitter, AlignsGlowsAndGivesSpotLightsCones)
{
	const auto aligned = MakeLightEmitter(MakeLight(0, 8, {0.5f, 0.84f, 1.52f}, 100.0f));
	ASSERT_TRUE(aligned.glow.orientation.has_value());
	// The sprite's y runs along the light's third axis, the world's y here
	EXPECT_FLOAT_EQ((*aligned.glow.orientation)[1].y, 1.0f);

	const auto spot = MakeLightEmitter(MakeLight(1, 1, {0.5f, 0.83f, 0.49f}, 100.0f));
	ASSERT_TRUE(spot.cone.has_value());
	EXPECT_FLOAT_EQ(spot.cone->length, 60.0f);
	EXPECT_FLOAT_EQ(spot.cone->colour.r, 32.0f / 255.0f);
	// Only spot lights, and only those flagged, have cones
	EXPECT_FALSE(MakeLightEmitter(MakeLight(0, 1, {1.0f, 1.0f, 1.0f}, 100.0f)).cone.has_value());
	EXPECT_FALSE(MakeLightEmitter(MakeLight(1, 0, {1.0f, 1.0f, 1.0f}, 100.0f)).cone.has_value());
}

TEST(LightBeams, ConeFadesFromTheLightToItsFarEnd)
{
	const auto emitter = MakeLightEmitter(MakeLight(1, 1, {1.0f, 1.0f, 1.0f}, 100.0f));
	BeamMesh mesh;
	AppendCone(*emitter.cone, 0.0f, mesh);
	ASSERT_EQ(mesh.vertices.size(), 16);
	ASSERT_EQ(mesh.indices.size(), 48);
	for (size_t i = 0; i < mesh.vertices.size(); i += 2)
	{
		// A full angle of 90 degrees spreads as far as the cone is long
		const auto& far = mesh.vertices[i];
		EXPECT_NEAR(far.position.y, 33.0f - 60.0f, 1e-4f);
		EXPECT_NEAR(glm::length(glm::vec2(far.position.x - 1.0f, far.position.z - 2.0f)), 60.0f, 1e-3f);
		EXPECT_EQ(far.colour, glm::u8vec4(0, 0, 0, 255));
		const auto& near = mesh.vertices[i + 1];
		EXPECT_NEAR(near.position.y, 33.0f, 1e-4f);
		EXPECT_EQ(near.colour, glm::u8vec4(64, 64, 64, 255));
	}
	for (const auto index : mesh.indices)
	{
		EXPECT_LT(index, mesh.vertices.size());
	}
}

TEST(LightBeams, VolumeLightDrawsEachEdgeOutFromTheSource)
{
	// A square window of two triangles, facing the source 10 units behind it
	std::vector<l3d::L3DVertex> vertices(4);
	vertices[0].position = {0.0f, 0.0f, 0.0f};
	vertices[1].position = {1.0f, 0.0f, 0.0f};
	vertices[2].position = {1.0f, 1.0f, 0.0f};
	vertices[3].position = {0.0f, 1.0f, 0.0f};
	vertices[2].texCoord = {1.0f, 1.0f};
	const std::vector<uint16_t> indices {0, 1, 2, 0, 2, 3};
	const glm::vec3 source {0.5f, 0.5f, -10.0f};

	const auto mesh = MakeVolumeLight(vertices, indices, source, 20.0f);
	// Its four sides and the diagonal they share
	ASSERT_EQ(mesh.vertices.size(), 5 * 4);
	ASSERT_EQ(mesh.indices.size(), 5 * 6);
	for (size_t i = 0; i < mesh.vertices.size(); i += 2)
	{
		const auto& near = mesh.vertices[i];
		const auto& far = mesh.vertices[i + 1];
		EXPECT_EQ(near.colour, glm::u8vec4(0x40, 0x40, 0x40, 0x7f));
		EXPECT_EQ(far.colour, glm::u8vec4(0));
		EXPECT_EQ(near.uv, far.uv);
		EXPECT_NEAR(glm::distance(near.position, far.position), 20.0f * k_VolumeLightLengthScale, 1e-4f);
		EXPECT_NEAR(glm::dot(glm::normalize(far.position - near.position), glm::normalize(near.position - source)), 1.0f,
		            1e-5f);
	}
}
