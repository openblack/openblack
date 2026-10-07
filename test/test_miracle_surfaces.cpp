/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <memory>
#include <string>

#include <ParticleFile.h>
#include <gtest/gtest.h>

#include "Common/GameRandom.h"
#include "Particles/ParticleClassRegistry.h"
#include "Particles/ParticleDrawFrame.h"
#include "Particles/ParticleEffect.h"
#include "Particles/ParticleSurfaces.h"
#include "Particles/SurfaceOfRevolution.h"

using namespace openblack;
using namespace openblack::particles;

namespace
{
constexpr float k_Epsilon = 1e-4f;

class FlatWorld final: public ParticleWorldInterface
{
public:
	[[nodiscard]] float LandHeight(glm::vec2 xz) const override { return slope * xz.x; }
	[[nodiscard]] uint32_t PlayerColour(int /*player*/) const override { return 0x00FF00u; }
	[[nodiscard]] glm::vec3 CameraRight() const override { return {1.0f, 0.0f, 0.0f}; }
	[[nodiscard]] glm::vec3 CameraUp() const override { return {0.0f, 1.0f, 0.0f}; }
	float slope {0.0f};
};

class FixedRandom final: public GameRandomInterface
{
public:
	uint32_t GameRand(uint32_t /*n*/) override { return 0; }
	float GameFloatRand(float /*x*/) override { return 0.0f; }
	uint32_t LocalRand(int32_t /*n*/) override { return 0; }
	float LocalFloatRand(float /*x*/) override { return 0.0f; }
	int32_t CrtRand() override { return 0; }
	void CrtSrand(uint32_t /*seed*/) override {}
	[[nodiscard]] GameRandomSeeds GetSeeds() const override { return {}; }
	void SetSeeds(GameRandomSeeds /*seeds*/) override {}
	[[nodiscard]] ParticleRandomStream GetParticleStream() const override { return ParticleRandomStream::None; }
	void SetParticleStream(ParticleRandomStream /*stream*/) override {}
};

/// The dispenser's swirl as its file has it: a point at the origin carrying a second point a little above, drawn as a
/// starry disk six across
const std::string k_Swirl = R"(BEGINPROPERTIES
PROPERTY DeleteOnCloseDown BOOL 1
PROPERTY Hierarchies ARRAY SIZE 4 0 0 1 0
PROPERTY InitiallyCreated ARRAY SIZE 4 1 0 0 0
PROPERTY MaxSpellAge FLOAT -1
ENDPROPERTIES
BEGINCLASS ZR_SurfRevol ZR_SurfRevol0
BEGINPROPERTIES
PROPERTY AlphaFadeIn FLOAT 0.4
PROPERTY AlphaFadeOut FLOAT 0.4
PROPERTY ChangeSpecColor BOOL 0
PROPERTY ClampToLandscape BOOL 0
PROPERTY ColorA INTEGER 110
PROPERTY ColorB INTEGER 255
PROPERTY ColorG INTEGER 255
PROPERTY ColorR INTEGER 255
PROPERTY DoRaiseAboveLandscape BOOL 1
PROPERTY FadeAlphas BOOL 1
PROPERTY FunctionIndex INTEGER 0
PROPERTY Group INTEGER 3
PROPERTY MaterialSetDoubleSided BOOL 0
PROPERTY MaxUVChange FLOAT 0
PROPERTY MaxVertexChange FLOAT 0
PROPERTY MaterialUpdateZBuffer BOOL 0
PROPERTY NumU INTEGER 12
PROPERTY NumV INTEGER 4
PROPERTY Scale FLOAT 1
PROPERTY SpeedU FLOAT 0.05
PROPERTY SpeedV FLOAT 0.9
PROPERTY TextureFileName STRING .\Data\Textures\S_LightSheetStars.raw
PROPERTY TextureHeight INTEGER 256
PROPERTY TextureWidth INTEGER 256
PROPERTY UseAdditiveAlpha BOOL 1
PROPERTY UseLighting BOOL 1
ENDPROPERTIES
ENDCLASS
BEGINCLASS CreateRuleAnAtom CreateRuleAnAtom0
BEGINPROPERTIES
PROPERTY Group INTEGER 0
PROPERTY NextGroups ARRAY SIZE 1 2
PROPERTY PCreator PERSIS_PNTR ParticlePointCreator0
ENDPROPERTIES
ENDCLASS
BEGINCLASS CreateRuleAnAtom CreateRuleAnAtom2
BEGINPROPERTIES
PROPERTY Group INTEGER 2
PROPERTY NextGroups ARRAY SIZE 1 3
PROPERTY OffsetY FLOAT 0.1
PROPERTY PCreator PERSIS_PNTR ParticlePointCreator0
ENDPROPERTIES
ENDCLASS
BEGINCLASS ParticlePointCreator ParticlePointCreator0
BEGINPROPERTIES
PROPERTY InitialScale FLOAT 6
ENDPROPERTIES
ENDCLASS
)";
} // namespace

TEST(SurfaceOfRevolution, ProfilesFollowTheirCurves)
{
	using namespace surface;
	EXPECT_NEAR(Evaluate(Profile::Disk, 0.5f).radius, 0.5f, k_Epsilon);
	EXPECT_NEAR(Evaluate(Profile::Disk, 0.5f).height, 0.0f, k_Epsilon);
	EXPECT_NEAR(Evaluate(Profile::Funnel, 0.25f).height, -1.5f, k_Epsilon);
	EXPECT_NEAR(Evaluate(Profile::FunnelSpout, 0.5f).radius, 0.75f, k_Epsilon);
	EXPECT_NEAR(Evaluate(Profile::FunnelSpout, 0.5f).height, 0.0f, k_Epsilon);
	EXPECT_NEAR(Evaluate(Profile::FunnelParabola, 0.5f).height, -2.25f, k_Epsilon);
	EXPECT_EQ(ProfileOf(2), Profile::FunnelSpout);
	EXPECT_EQ(ProfileOf(7), Profile::Disk);
}

TEST(SurfaceOfRevolution, RingsBrightenInsideAndFadeAtTheRim)
{
	using namespace surface;
	EXPECT_NEAR(ShadeAt(0.0f, 0.4f, 0.4f).brightness, 0.0f, k_Epsilon);
	EXPECT_NEAR(ShadeAt(0.2f, 0.4f, 0.4f).brightness, 0.5f, k_Epsilon);
	EXPECT_NEAR(ShadeAt(0.2f, 0.4f, 0.4f).alpha, 1.0f, k_Epsilon);
	EXPECT_NEAR(ShadeAt(0.5f, 0.4f, 0.4f).brightness, 1.0f, k_Epsilon);
	EXPECT_NEAR(ShadeAt(0.8f, 0.4f, 0.4f).alpha, 0.5f, k_Epsilon);
	EXPECT_NEAR(ShadeAt(1.0f, 0.4f, 0.4f).alpha, 0.0f, k_Epsilon);
}

TEST(SurfaceOfRevolution, TheGridHasTwoTrianglesASquare)
{
	using namespace surface;
	const auto mesh =
	    Build({.profile = Profile::Disk, .numU = 12, .numV = 4, .fadeAlphas = true, .specular = true}, 0xFFFF0000u);
	ASSERT_EQ(mesh.vertices.size(), 48u);
	EXPECT_EQ(mesh.indices.size(), 11u * 3u * 6u);
	// The middle ring is a point, black and shining red; the rim is a circle of radius 1, see-through
	EXPECT_NEAR(mesh.vertices[0].position.x, 0.0f, k_Epsilon);
	EXPECT_EQ(mesh.vertices[0].colour, 0xFF000000u);
	EXPECT_EQ(mesh.vertices[0].specular, 0xFFFE0000u);
	EXPECT_EQ(mesh.vertices[24].specular, 0xFF000000u);
	EXPECT_NEAR(mesh.vertices[36].position.x, 1.0f, k_Epsilon);
	EXPECT_EQ(mesh.vertices[36].colour, 0x00FFFFFFu);
	EXPECT_NEAR(mesh.vertices[47].uv.x, 1.0f, k_Epsilon);
	EXPECT_NEAR(mesh.vertices[47].uv.y, 1.0f, k_Epsilon);
	// The first triangle runs out, in, and out a step round
	EXPECT_EQ(mesh.indices[0], 12);
	EXPECT_EQ(mesh.indices[1], 0);
	EXPECT_EQ(mesh.indices[2], 13);
}

TEST(SurfaceOfRevolution, SwayingTwistsTheRingsAndSlidesTheTexture)
{
	using namespace surface;
	const Shape shape {.profile = Profile::Disk, .numU = 4, .numV = 3};
	const auto still = Build(shape, 0);
	Mesh swayed;
	Sway(still, shape, 1.0f, 0.5f, 1.0f, swayed);
	// The rim turns by a whole radian, the middle not at all; the middle's texture slides by half
	const auto& rim = swayed.vertices[8];
	EXPECT_NEAR(rim.position.x, std::cos(1.0f), k_Epsilon);
	EXPECT_NEAR(rim.position.z, std::sin(1.0f), k_Epsilon);
	EXPECT_NEAR(swayed.vertices[0].uv.x, still.vertices[0].uv.x + 0.5f, k_Epsilon);
	EXPECT_NEAR(rim.uv.x, still.vertices[8].uv.x, k_Epsilon);
}

TEST(SurfaceOfRevolution, TheDispenserSwirlIsDrawnAsALitDiskOverTheLand)
{
	const auto classes = ParticleClassRegistry::WithAllClasses();
	FlatWorld world;
	world.slope = 0.5f;
	FixedRandom random;
	maths::ValueNoise noise;
	auto file = psys::ParticleFile::Parse(k_Swirl);
	ASSERT_TRUE(file.has_value());
	Effect effect(std::make_shared<const psys::ParticleFile>(std::move(*file)), EffectServices {classes, world, random, noise},
	              glm::vec3(0.0f), 1.0f, false);
	EXPECT_TRUE(effect.UnportedClasses().empty());
	for (int i = 0; i < 10; ++i)
	{
		effect.Step(0.1f);
	}
	Effect::DrawWalk walk;
	effect.Walk(1.0f, walk);
	draw::Frame frame;
	const draw::Sources sources {
	    .textures = [](std::string_view name) -> std::optional<std::pair<entt::id_type, entt::id_type>> {
		    return name == "S_LightSheetStars" ? std::optional(std::pair<entt::id_type, entt::id_type>(1, 2)) : std::nullopt;
	    },
	    .playerColour = {},
	    .random = {},
	    .landHeight = [&world](glm::vec2 xz) { return world.LandHeight(xz); },
	};
	draw::AddEffect(frame, walk, draw::DrawPath::Sorted, glm::vec3(0.0f), 0, sources);
	ASSERT_EQ(frame.surfaces.size(), 1u);
	const auto& surface = frame.surfaces[0];
	// Cut along the land's cells and diagonals where it crosses them
	EXPECT_GT(surface.vertexCount, 48u);
	EXPECT_TRUE(surface.lit);
	EXPECT_NEAR(frame.surfaceNormals[surface.firstVertex + 13].y, 1.0f, k_Epsilon);
	EXPECT_FALSE(surface.doubleSided);
	EXPECT_EQ(graphics::render_modes::Desc(frame.materials[surface.material].mode).blend,
	          graphics::render_modes::Blend::Additive);
	// The rim six out lies over the sloping land, a tenth above the land under the middle
	const auto& rim = frame.surfaceVertices[surface.firstVertex + 36];
	EXPECT_NEAR(rim.position.x, 6.0f, 1e-3f);
	EXPECT_NEAR(rim.position.y, 0.1f + 3.0f, 1e-3f);
	// Its texture has crept a second's worth round and out
	EXPECT_NEAR(rim.uv.x, 0.05f, 1e-3f);
	EXPECT_NEAR(rim.uv.y, 1.0f + std::fmod(0.9f, 1.0f), 1e-3f);
	// Tinted by the swirl's alpha of 110: the bright rings show at 110 of 256
	const auto& bright = frame.surfaceVertices[surface.firstVertex + 13];
	EXPECT_EQ(bright.abgr >> 24u, 109u);
}

TEST(SurfaceOfRevolution, AScaledParentSizesItsSurface)
{
	// The teleport's pool: a point of scale 2, grown five times over by a rule, carries the disk in its frame
	std::string text = k_Swirl;
	const auto replace = [&text](std::string_view from, std::string_view to) {
		text.replace(text.find(from), from.size(), to);
	};
	replace("PROPERTY InitialScale FLOAT 6", "PROPERTY InitialScale FLOAT 2");
	text += R"(BEGINCLASS UR_ChangeScale UR_ChangeScale0
BEGINPROPERTIES
PROPERTY Group INTEGER 2
PROPERTY StartScale FLOAT 5
PROPERTY StartTime FLOAT 0
PROPERTY StopScale FLOAT 5
PROPERTY StopTime FLOAT 5
ENDPROPERTIES
ENDCLASS
)";
	const auto classes = ParticleClassRegistry::WithAllClasses();
	FlatWorld world;
	FixedRandom random;
	maths::ValueNoise noise;
	auto file = psys::ParticleFile::Parse(text);
	ASSERT_TRUE(file.has_value());
	Effect effect(std::make_shared<const psys::ParticleFile>(std::move(*file)), EffectServices {classes, world, random, noise},
	              glm::vec3(0.0f), 1.0f, false);
	for (int i = 0; i < 10; ++i)
	{
		effect.Step(0.1f);
	}
	Effect::DrawWalk walk;
	effect.Walk(1.0f, walk);
	draw::Frame frame;
	const draw::Sources sources {
	    .textures = [](std::string_view) -> std::optional<std::pair<entt::id_type, entt::id_type>> {
		    return std::pair<entt::id_type, entt::id_type>(1, 2);
	    },
	    .playerColour = {},
	    .random = {},
	    .landHeight = {},
	};
	draw::AddEffect(frame, walk, draw::DrawPath::Sorted, glm::vec3(0.0f), 0, sources);
	ASSERT_EQ(frame.surfaces.size(), 1u);
	// Its scale of 2 times the rule's 5: the disk reaches 10 out
	EXPECT_NEAR(frame.surfaceVertices[frame.surfaces[0].firstVertex + 36].position.x, 10.0f, 1e-3f);
}

TEST(SurfaceOfRevolution, SlicingCutsATriangleIntoThreeAlongThePlane)
{
	using namespace surface;
	Mesh mesh;
	mesh.vertices = {{.position = {0.0f, 0.0f, 0.0f}, .uv = {0.0f, 0.0f}, .colour = 0xFF000000u, .specular = 0},
	                 {.position = {4.0f, 0.0f, 0.0f}, .uv = {1.0f, 0.0f}, .colour = 0xFFFFFFFFu, .specular = 0},
	                 {.position = {0.0f, 0.0f, 4.0f}, .uv = {0.0f, 1.0f}, .colour = 0xFF000000u, .specular = 0}};
	mesh.indices = {0, 1, 2};
	// x = 1 cuts the edges from the corner alone on its side, the second
	Slice(mesh, {.normal = {1.0f, 0.0f, 0.0f}, .offset = -1.0f});
	ASSERT_EQ(mesh.vertices.size(), 5u);
	ASSERT_EQ(mesh.indices.size(), 9u);
	EXPECT_EQ(mesh.indices[0], 1u);
	EXPECT_NEAR(mesh.vertices[3].position.x, 1.0f, k_Epsilon);
	EXPECT_NEAR(mesh.vertices[3].position.z, 3.0f, k_Epsilon);
	EXPECT_NEAR(mesh.vertices[4].position.x, 1.0f, k_Epsilon);
	EXPECT_NEAR(mesh.vertices[4].uv.x, 0.25f, k_Epsilon);
	// Three quarters of the way from white to black, by a byte of 191: 255 + (-255 * 191 >> 8), shifted down
	EXPECT_EQ(mesh.vertices[4].colour & 0xFFu, 64u);
}

TEST(SurfaceOfRevolution, TheLightIsTheAmbientFacingAwayAndMoreFacingIt)
{
	using namespace surface;
	EXPECT_EQ(LightLevel({0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, 90), 90u);
	EXPECT_EQ(LightLevel({0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 90), 90u + (165u * 255u >> 8u));
	EXPECT_EQ(Modulate(0xFFFFFFFFu, 0x6EFFFFFFu), 0x6DFEFEFEu);
	EXPECT_EQ(ScaleRgb(0xFF808080u, 128), 0xFF404040u);
}
