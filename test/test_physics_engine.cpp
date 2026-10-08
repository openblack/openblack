/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <numbers>
#include <sstream>
#include <vector>

#include <PhysicsConstantsFile.h>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <gtest/gtest.h>

#include "Physics/Body.h"
#include "Physics/BodyShapes.h"
#include "Physics/Ground.h"
#include "Physics/Materials.h"
#include "Physics/PairRules.h"

using namespace openblack;
using namespace openblack::physics;

namespace
{
/// Flat or sloped land, never sea
class FakeLand final: public Ground
{
public:
	explicit FakeLand(float height, float slopeX = 0.0f)
	    : _height(height)
	    , _slope(slopeX)
	{
	}
	[[nodiscard]] float HeightAt(glm::vec2 xz) const override { return _height + _slope * xz.x; }
	[[nodiscard]] glm::vec3 NormalAt(glm::vec2) const override { return glm::normalize(glm::vec3(-_slope, 1.0f, 0.0f)); }
	[[nodiscard]] bool IsSeaCell(glm::vec2) const override { return false; }

private:
	float _height;
	float _slope;
};

/// Open sea everywhere, its floor far below
class FakeSea final: public Ground
{
public:
	[[nodiscard]] float HeightAt(glm::vec2) const override { return -100.0f; }
	[[nodiscard]] glm::vec3 NormalAt(glm::vec2) const override { return {0.0f, 1.0f, 0.0f}; }
	[[nodiscard]] bool IsSeaCell(glm::vec2) const override { return true; }
};

constexpr Material k_Stone {
    .density = 2.0f, .springK = 20.0f, .dampK = 1.0f, .friction = 1.0f, .spinKeptPerSecond = 0.7f, .drag = 0.0f};

/// A cube of half side `half` about its centre, faces turned outwards
Shape Cube(float half)
{
	Shape shape;
	for (int i = 0; i < 8; ++i)
	{
		const float x = ((i & 1) ^ ((i >> 1) & 1)) != 0 ? half : -half;
		const float y = (i & 2) != 0 ? half : -half;
		const float z = (i & 4) != 0 ? half : -half;
		shape.points.emplace_back(x, y, z);
	}
	shape.faces = {{0, 3, 2}, {0, 2, 1}, {4, 5, 6}, {4, 6, 7}, {0, 1, 5}, {0, 5, 4},
	               {3, 7, 6}, {3, 6, 2}, {0, 4, 7}, {0, 7, 3}, {1, 2, 6}, {1, 6, 5}};
	shape.radius = glm::length(glm::vec3(half));
	return shape;
}

Body MakeCube(float half, glm::vec3 centre, const Material& material = k_Stone, float mass = 10.0f, bool dynamic = true)
{
	Body body({.scale = 1.0f, .halfHeight = half, .mass = mass, .material = material, .dynamic = dynamic}, Cube(half));
	body.SetUpPose({.axes = glm::mat3(1.0f), .origin = centre});
	return body;
}

StepResult Step(Body& body, const Ground& ground)
{
	body.ClearForces();
	body.TouchGround(ground);
	body.ApplyContacts();
	return body.Integrate();
}

/// Steps until the body stops or is deleted; the number of steps taken
int StepUntil(Body& body, const Ground& ground, StepResult until, int maxSteps)
{
	for (int i = 1; i <= maxSteps; ++i)
	{
		if (Step(body, ground) == until)
		{
			return i;
		}
	}
	return -1;
}
} // namespace

TEST(PhysicsConstantsFile, ReadsTheHeaderAndRows)
{
	std::istringstream text("3\n2\n0.8 78.75 0.4 2.5 0.2 0\n2.0 21.5 1.25 1.25 0.7 1\n");
	const auto file = physconst::Parse(text, 24);
	ASSERT_TRUE(file.has_value());
	EXPECT_EQ(file->version, 3);
	EXPECT_EQ(file->declaredRows, 2);
	ASSERT_EQ(file->rows.size(), 2u);
	EXPECT_FLOAT_EQ(file->rows[1].density, 2.0f);
	EXPECT_FLOAT_EQ(file->rows[1].drag, 1.0f);
}

TEST(PhysicsConstantsFile, ReadsNoMoreRowsThanAsked)
{
	std::istringstream text("3 3\n1 1 1 1 1 1\n2 2 2 2 2 2\n3 3 3 3 3 3\n");
	const auto file = physconst::Parse(text, 2);
	ASSERT_TRUE(file.has_value());
	EXPECT_EQ(file->rows.size(), 2u);
}

TEST(PhysicsConstantsFile, RefusesTextWithoutAHeader)
{
	std::istringstream text("not numbers");
	EXPECT_FALSE(physconst::Parse(text, 24).has_value());
}

TEST(PhysicsMaterials, ClampsEachColumn)
{
	physconst::PhysicsConstantsFile file {.version = 3, .declaredRows = 1, .rows = {{5.0f, 300.0f, -1.0f, 9.0f, 0.1f, 7.0f}}};
	const MaterialTable table(file);
	const auto& row = table[MaterialRow::DefaultUnmovable];
	EXPECT_FLOAT_EQ(row.density, 3.0f);
	EXPECT_FLOAT_EQ(row.springK, 240.0f);
	EXPECT_FLOAT_EQ(row.dampK, 0.0f);
	EXPECT_FLOAT_EQ(row.friction, 3.0f);
	EXPECT_FLOAT_EQ(row.spinKeptPerSecond, 0.2f);
	EXPECT_FLOAT_EQ(row.drag, 4.0f);
	EXPECT_FLOAT_EQ(MaterialTable::Clamp({.density = 0.0f}).density, 0.05f);
}

TEST(PhysicsMaterials, RowsPastTheFileCopyTheFirst)
{
	physconst::PhysicsConstantsFile file {
	    .version = 3,
	    .declaredRows = 2,
	    .rows = {{0.8f, 10.0f, 1.0f, 1.0f, 0.5f, 1.0f}, {2.0f, 20.0f, 2.0f, 2.0f, 0.6f, 2.0f}}};
	const MaterialTable table(file);
	EXPECT_FLOAT_EQ(table[MaterialRow::DefaultMovable].density, 2.0f);
	EXPECT_EQ(table[MaterialRow::Toadstool], table[MaterialRow::DefaultUnmovable]);
	EXPECT_EQ(table[MaterialRow::Rock], table[MaterialRow::DefaultUnmovable]);
}

TEST(PhysicsMaterials, WithoutAFileEveryRowIsZero)
{
	const MaterialTable table(std::nullopt);
	EXPECT_EQ(table[MaterialRow::Rock], Material {});
	EXPECT_EQ(table[MaterialRow::Villager], Material {});
}

TEST(PhysicsBody, InertiaKeepsTheGamesCrossTerm)
{
	Shape shape;
	shape.points = {{1.0f, 2.0f, 3.0f}, {-1.0f, -2.0f, -3.0f}};
	shape.radius = glm::length(glm::vec3(1.0f, 2.0f, 3.0f));
	const Body body({.mass = 1.0f, .material = k_Stone}, shape);
	const auto& inertia = body.Inertia();
	EXPECT_FLOAT_EQ(inertia[0][0], 13.0f);
	EXPECT_FLOAT_EQ(inertia[0][1], -2.0f);
	EXPECT_FLOAT_EQ(inertia[2][1], -6.0f);
	// Where y z belongs the game sums x z
	EXPECT_FLOAT_EQ(inertia[1][2], -3.0f);
}

TEST(PhysicsBody, DragGrowsWithTheRadiusAndTheKind)
{
	auto material = k_Stone;
	material.drag = 2.0f;
	auto shape = Cube(1.0f);
	shape.dragFactor = 2.0f;
	const Body body({.mass = 1.0f, .material = material}, shape);
	EXPECT_NEAR(body.Drag(), 2.0f * shape.radius * shape.radius * 0.3f * 2.0f, 1e-5f);
	const Body still({.mass = 1.0f, .material = material, .dynamic = false}, Cube(1.0f));
	EXPECT_FLOAT_EQ(still.Drag(), 2.0f);
}

TEST(PhysicsBody, ADroppedCubeComesToRestOnTheLand)
{
	const FakeLand land(0.0f);
	auto body = MakeCube(0.5f, {0.0f, 3.0f, 0.0f});
	EXPECT_EQ(body.RestCounter(), -500);
	const int steps = StepUntil(body, land, StepResult::Stopped, 4000);
	ASSERT_GT(steps, 0);
	// It can't come to rest before it has been in the physics for its half height in seconds
	EXPECT_GT(steps, 100);
	EXPECT_NEAR(body.Centre().y, 0.5f, 0.2f);
	EXPECT_EQ(body.velocity, glm::vec3(0.0f));
}

TEST(PhysicsBody, ATallerBodyTakesLongerBeforeItCanRest)
{
	const FakeLand land(0.0f);
	Body body({.scale = 1.0f, .halfHeight = 2.0f, .mass = 10.0f, .material = k_Stone}, Cube(0.5f));
	body.SetUpPose({.axes = glm::mat3(1.0f), .origin = {0.0f, 0.55f, 0.0f}});
	const int steps = StepUntil(body, land, StepResult::Stopped, 4000);
	ASSERT_GT(steps, 0);
	EXPECT_GT(steps, 400);
}

TEST(PhysicsBody, FrictionHoldsOnGentleSlopesAndNotWithout)
{
	const FakeLand slope(0.0f, 0.3f);
	auto grippy = MakeCube(0.5f, {0.0f, 0.55f, 0.0f});
	auto slippery_material = k_Stone;
	slippery_material.friction = 0.02f;
	auto slippery = MakeCube(0.5f, {0.0f, 0.55f, 0.0f}, slippery_material);
	for (int i = 0; i < 600; ++i)
	{
		Step(grippy, slope);
		Step(slippery, slope);
	}
	EXPECT_LT(std::abs(grippy.Centre().x), 0.5f);
	// It slides downhill
	EXPECT_LT(slippery.Centre().x, -2.0f);
}

TEST(PhysicsBody, HowDeepABodySettlesComesFromItsMaterial)
{
	const auto settled = [](float springK, float dampK) {
		const FakeLand land(0.0f);
		auto material = k_Stone;
		material.springK = springK;
		material.dampK = dampK;
		auto body = MakeCube(0.5f, {0.0f, 3.0f, 0.0f}, material);
		EXPECT_GT(StepUntil(body, land, StepResult::Stopped, 4000), 0);
		return body.Centre().y;
	};
	// There is no bounce factor: stiffer contacts hold the body higher
	EXPECT_GT(settled(200.0f, 1.0f), settled(5.0f, 1.0f));
	EXPECT_GT(settled(20.0f, 8.0f), settled(20.0f, 0.5f));
}

TEST(PhysicsBody, SpeedIsCapped)
{
	const FakeLand land(-1000.0f);
	auto body = MakeCube(0.5f, {0.0f, 0.0f, 0.0f});
	body.velocity = {500.0f, 0.0f, 0.0f};
	Step(body, land);
	EXPECT_LE(body.Speed(), k_MaxSpeed);
	EXPECT_NEAR(glm::length(body.velocity), k_MaxSpeed, 1e-3f);
}

TEST(PhysicsBody, SpinIsCappedAtOneAndAHalfTurnsASecond)
{
	const FakeLand land(-1000.0f);
	auto body = MakeCube(0.5f, {0.0f, 0.0f, 0.0f});
	body.SetAngularVelocity({0.0f, 100.0f, 0.0f});
	const auto before = body.Axes()[0];
	Step(body, land);
	const float turned = std::acos(std::clamp(glm::dot(before, body.Axes()[0]), -1.0f, 1.0f));
	EXPECT_NEAR(turned, k_MaxSpin * k_StepSeconds, 1e-3f);
}

TEST(PhysicsBody, SpinDiesAwayByItsMaterial)
{
	const FakeLand land(-1000.0f);
	auto material = k_Stone;
	material.spinKeptPerSecond = 0.5f;
	auto body = MakeCube(0.5f, {0.0f, 100.0f, 0.0f}, material);
	body.SetAngularVelocity({0.0f, 1.0f, 0.0f});
	const float start = glm::length(body.angularMomentum);
	for (int i = 0; i < 200; ++i)
	{
		Step(body, land);
	}
	EXPECT_NEAR(glm::length(body.angularMomentum) / start, 0.5f, 1e-3f);
}

TEST(PhysicsBody, LightBodiesFloatAndHeavyOnesSink)
{
	const FakeSea sea;
	auto light = k_Stone;
	light.density = 0.3f;
	auto floating = MakeCube(0.5f, {0.0f, 0.2f, 0.0f}, light);
	EXPECT_EQ(StepUntil(floating, sea, StepResult::Delete, 600), -1);
	EXPECT_GT(floating.Centre().y, -floating.Radius());
	EXPECT_TRUE(floating.inWater);

	auto heavy = k_Stone;
	heavy.density = 2.5f;
	auto sinking = MakeCube(0.5f, {0.0f, 0.2f, 0.0f}, heavy);
	EXPECT_GT(StepUntil(sinking, sea, StepResult::Delete, 20000), 0);
}

TEST(PhysicsBody, FloatingThingsSoakUpWaterAndSinkInTheEnd)
{
	const FakeSea sea;
	auto wood = k_Stone;
	wood.density = 0.9f;
	auto body = MakeCube(0.5f, {0.0f, 0.2f, 0.0f}, wood);
	for (int i = 0; i < 200; ++i)
	{
		Step(body, sea);
	}
	// About 0.013 a second while in the sea
	EXPECT_NEAR(body.density, 0.9f + 200 * (1.0f / 15000.0f), 1e-3f);
	EXPECT_GT(StepUntil(body, sea, StepResult::Delete, 40000), 0);
}

TEST(PhysicsBody, ABodyFarUnderTheSeaIsDeleted)
{
	const FakeLand land(-1000.0f);
	auto body = MakeCube(0.5f, {0.0f, -4.0f, 0.0f});
	EXPECT_EQ(Step(body, land), StepResult::Delete);
}

TEST(PhysicsBody, TwoBodiesPushEachOtherEquallyAndOppositely)
{
	const FakeLand land(-1000.0f);
	auto material = k_Stone;
	material.drag = 0.0f;
	auto a = MakeCube(0.5f, {-0.45f, 0.0f, 0.0f}, material, 10.0f);
	auto b = MakeCube(0.5f, {0.6f, 0.0f, 0.0f}, material, 20.0f);
	a.velocity = {5.0f, 0.0f, 0.0f};
	for (auto* body : {&a, &b})
	{
		body->ClearForces();
		body->TouchGround(land);
	}
	a.TouchFaces(b);
	b.TouchFaces(a);
	a.ApplyContacts();
	b.ApplyContacts();
	EXPECT_LT(a.Force().x, 0.0f);
	EXPECT_GT(b.Force().x, 0.0f);
	const auto sum = a.Force() + b.Force();
	EXPECT_NEAR(sum.x, 0.0f, 1e-3f);
	EXPECT_NEAR(sum.y, -(10.0f + 20.0f) * k_Gravity, 1e-2f);
	EXPECT_EQ(a.lastHit, &b);
	EXPECT_EQ(b.lastHit, &a);
}

TEST(PhysicsBody, LookingAheadStopsAFastBodyAtAThinWall)
{
	const FakeLand land(-1000.0f);
	Shape wallShape;
	wallShape.points = {{0.0f, -5.0f, -5.0f}, {0.0f, -5.0f, 5.0f}, {0.0f, 5.0f, 5.0f}, {0.0f, 5.0f, -5.0f}};
	wallShape.faces = {{0, 1, 2}, {0, 2, 3}};
	wallShape.radius = glm::length(glm::vec2(5.0f));
	Body wall({.mass = 50000.0f, .material = k_Stone, .dynamic = false}, wallShape);
	wall.SetUpPose({.axes = glm::mat3(1.0f), .origin = {0.0f, 0.0f, 0.0f}});
	wall.resting = true;
	auto ball = MakeCube(0.25f, {-3.0f, 0.0f, 0.0f});
	ball.velocity = {40.0f, 0.0f, 0.0f};
	for (int i = 0; i < 200; ++i)
	{
		ball.ClearForces();
		wall.ClearForces();
		ball.TouchGround(land);
		wall.TouchGround(land);
		ball.TouchFaces(wall);
		ball.ApplyContacts();
		wall.ApplyContacts();
		static_cast<void>(ball.Integrate());
		ASSERT_LT(ball.Centre().x, 0.25f) << "through the wall at step " << i;
	}
	EXPECT_LT(ball.velocity.x, 0.0f);
}

TEST(PhysicsBody, OverlapIsMeasuredThroughTheTop)
{
	auto below = MakeCube(1.0f, {0.0f, 0.0f, 0.0f});
	auto above = MakeCube(0.5f, {0.0f, 1.0f, 0.0f});
	// The upper cube's lower points are half a unit into the lower one
	EXPECT_NEAR(above.OverlapDepth(below, {0.0f, -1.0f, 0.0f}), 0.5f, 1e-4f);
	above.Raise(0.5f);
	EXPECT_NEAR(above.OverlapDepth(below, {0.0f, -1.0f, 0.0f}), 0.0f, 1e-4f);
}

TEST(PhysicsBody, AdjustingToTheGroundOnlyRaisesWhenAsked)
{
	const FakeLand land(0.0f);
	auto high = MakeCube(0.5f, {0.0f, 3.0f, 0.0f});
	high.SettleOnLand(land, true, false);
	EXPECT_FLOAT_EQ(high.Centre().y, 3.0f);
	high.SettleOnLand(land, false, false);
	EXPECT_NEAR(high.Centre().y, 0.5f, 1e-5f);
	auto sunk = MakeCube(0.5f, {0.0f, 0.0f, 0.0f});
	sunk.SettleOnLand(land, true, false);
	EXPECT_NEAR(sunk.Centre().y, 0.5f, 1e-5f);
}

TEST(PhysicsBody, AlignsToTheSlope)
{
	const FakeLand slope(0.0f, 0.5f);
	auto body = MakeCube(0.5f, {0.0f, 3.0f, 0.0f});
	body.SettleOnLand(slope, false, true);
	EXPECT_NEAR(glm::dot(body.Axes()[1], slope.NormalAt({0.0f, 0.0f})), 1.0f, 1e-5f);
}

TEST(PhysicsBody, TheObjectPoseUndoesTheCentreOfMass)
{
	auto shape = Cube(0.5f);
	shape.centreOfMass = {0.0f, 1.0f, 0.0f};
	Body body({.scale = 2.0f, .mass = 1.0f, .material = k_Stone}, shape);
	const Pose placed {.axes = glm::mat3(2.0f), .origin = {3.0f, 4.0f, 5.0f}};
	body.SetUpPose(placed);
	EXPECT_NEAR(body.Centre().y, 6.0f, 1e-5f);
	const auto pose = body.ObjectPose();
	EXPECT_NEAR(glm::distance(pose.origin, placed.origin), 0.0f, 1e-5f);
	EXPECT_NEAR(pose.axes[1].y, 2.0f, 1e-5f);
	const auto drawn = body.DrawPose(0.5f, true);
	// An animated model is drawn turned a quarter about its up axis
	EXPECT_NEAR(drawn.axes[0].z, 2.0f, 1e-4f);
	EXPECT_NEAR(drawn.axes[2].x, -2.0f, 1e-4f);
}

TEST(PhysicsShapes, ATreeIsASpindle)
{
	const auto rooted = shapes::Tree(10.0f, 2.0f, 2.0f, true);
	EXPECT_EQ(rooted.points.size(), 16u);
	EXPECT_EQ(rooted.faces.size(), 24u);
	EXPECT_FLOAT_EQ(rooted.centreOfMass.y, 2.0f);
	EXPECT_FLOAT_EQ(rooted.radius, 6.0f);
	EXPECT_FLOAT_EQ(rooted.dragFactor, 0.3f);
	// Its spindle reaches a fifth of its height below its base
	EXPECT_FLOAT_EQ(rooted.points[12].y, -6.0f);
	const auto fallen = shapes::Tree(10.0f, 2.0f, 2.0f, false);
	EXPECT_FLOAT_EQ(fallen.centreOfMass.y, 2.5f);
	EXPECT_FLOAT_EQ(fallen.radius, 5.0f);
}

TEST(PhysicsShapes, LivingThingsAreTwelvePointBoxes)
{
	const auto villager = shapes::LivingBody(shapes::Living::Villager, 2.0f, 0.5f, 1.0f);
	EXPECT_EQ(villager.points.size(), 12u);
	EXPECT_EQ(villager.faces.size(), 20u);
	EXPECT_FLOAT_EQ(villager.centreOfMass.y, 1.0f);
	EXPECT_FLOAT_EQ(villager.dragFactor, 2.0f);
	// The first head point (0.8 r, y, 0.32 r) turned a quarter
	EXPECT_NEAR(villager.points[0].x, -0.16f, 1e-5f);
	EXPECT_NEAR(villager.points[0].z, 0.4f, 1e-5f);
	EXPECT_NEAR(villager.radius, glm::length(glm::vec3(0.16f, 1.0f, 0.4f)), 1e-5f);
	const auto animal = shapes::LivingBody(shapes::Living::Animal, 2.0f, 0.5f, 1.0f);
	EXPECT_NEAR(animal.points[4].x, -0.45f, 1e-5f);
	EXPECT_NEAR(animal.points[4].z, 0.15f, 1e-5f);
}

TEST(PhysicsShapes, ABuildingPieceIsASlab)
{
	const std::array<std::array<glm::vec3, 3>, 2> square {{
	    {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(2.0f, 2.0f, 0.0f)},
	    {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(2.0f, 2.0f, 0.0f), glm::vec3(0.0f, 2.0f, 0.0f)},
	}};
	const auto piece = shapes::Fragment(square);
	EXPECT_FLOAT_EQ(piece.area, 4.0f);
	EXPECT_FLOAT_EQ(piece.mass, 120.0f);
	ASSERT_EQ(piece.shape.points.size(), 8u);
	EXPECT_TRUE(piece.shape.faces.empty());
	// Each corner has a twin behind it
	EXPECT_FLOAT_EQ(piece.shape.points[1].z, -0.45f);
	EXPECT_FALSE(piece.tooThin);

	const std::array<std::array<glm::vec3, 3>, 1> sliver {{
	    {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(10.0f, 0.0f, 0.0f), glm::vec3(10.0f, 0.1f, 0.0f)},
	}};
	EXPECT_TRUE(shapes::Fragment(sliver).tooThin);
}

TEST(PhysicsShapes, AModelUsesItsCollisionPartsFirst)
{
	const std::vector<glm::vec3> drawn {{0, 0, 0}, {4, 0, 0}, {0, 4, 0}};
	const std::vector<glm::vec3> collision {{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {0, 0, 2}};
	const std::vector<uint32_t> triangle {0, 1, 2};
	const std::vector<shapes::ModelPart> parts {
	    {.positions = drawn, .indices = triangle, .isPhysics = false, .nearestDetail = true},
	    {.positions = collision, .indices = triangle, .isPhysics = true, .nearestDetail = false},
	};
	const auto shape = shapes::FromModel(parts, 2.0f);
	ASSERT_EQ(shape.points.size(), 4u);
	EXPECT_EQ(shape.faces.size(), 1u);
	EXPECT_FLOAT_EQ(shape.centreOfMass.x, 0.5f);
	EXPECT_NEAR(shape.points[1].x, 3.0f, 1e-5f);

	const std::vector<shapes::ModelPart> drawnOnly {
	    {.positions = drawn, .indices = triangle, .isPhysics = false, .nearestDetail = true},
	    {.positions = collision, .indices = triangle, .isPhysics = false, .nearestDetail = false},
	};
	EXPECT_EQ(shapes::FromModel(drawnOnly, 1.0f).points.size(), 3u);
}

TEST(PhysicsShapes, TheCreatureIsHitOnItsSkeleton)
{
	const Ellipsoid part {.boxMin = glm::vec3(-1.0f), .boxMax = glm::vec3(1.0f)};
	const std::array bones {part};
	const auto hit = EllipsoidHit(bones, {5.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f});
	ASSERT_TRUE(hit.has_value());
	EXPECT_NEAR(hit->second, 4.0f, 1e-4f);
	EXPECT_NEAR(hit->first.point.x, 1.0f, 1e-4f);
	EXPECT_NEAR(hit->first.normal.x, 1.0f, 1e-4f);
	EXPECT_FALSE(EllipsoidHit(bones, {5.0f, 3.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}).has_value());

	// A stone thrown at a resting creature touches it
	const FakeLand land(-1000.0f);
	const std::array origins {glm::vec3(0.0f)};
	Body creature({.mass = shapes::k_CreatureMass, .material = k_Stone, .dynamic = false},
	              shapes::Creature(3.0f, origins.size()));
	creature.PlacePoints(glm::vec3(0.0f), origins);
	// Its points sit on the centre in its own frame and stand at the parts' origins in the world
	EXPECT_EQ(creature.Points()[0].local, glm::vec3(0.0f));
	EXPECT_EQ(creature.Points()[0].world, origins[0]);
	creature.SetSkeleton({part});
	creature.resting = true;
	auto stone = MakeCube(0.25f, {-1.6f, 0.0f, 0.0f});
	stone.velocity = {10.0f, 0.0f, 0.0f};
	stone.ClearForces();
	creature.ClearForces();
	stone.TouchGround(land);
	creature.TouchGround(land);
	stone.TouchFaces(creature);
	EXPECT_GT(stone.Contacts(), 0);
	EXPECT_EQ(stone.lastHit, &creature);
}

TEST(PhysicsPairs, WhoTestsAgainstWhom)
{
	const PairMember thrown {.centre = {0, 0, 0}, .radius = 1.0f, .object = 1, .thrower = 2};
	const PairMember thrower {.centre = {1, 0, 0}, .radius = 1.0f, .object = 2};
	EXPECT_FALSE(TestsPoints(thrown, thrower));
	EXPECT_FALSE(TestsPoints(thrower, thrown));

	PairMember stone {.centre = {0, 0, 0}, .radius = 1.0f, .object = 3};
	PairMember house {.centre = {1.5f, 0, 0}, .radius = 1.0f, .resting = true, .checksPoints = false, .object = 4};
	EXPECT_TRUE(TestsPoints(stone, house));
	EXPECT_FALSE(TestsPoints(house, stone));
	house.centre = {2.0f, 0, 0};
	EXPECT_FALSE(TestsPoints(stone, house)) << "touching spheres don't count";

	stone.resting = true;
	house.centre = {1.0f, 0, 0};
	EXPECT_FALSE(TestsPoints(stone, house));
	stone.justSetUp = true;
	EXPECT_TRUE(TestsPoints(stone, house));

	const PairMember villager {.radius = 1.0f, .isVillager = true, .object = 5};
	const PairMember pushed {.radius = 1.0f, .pushedByLiving = true, .object = 6};
	EXPECT_FALSE(TestsPoints(villager, pushed));
	EXPECT_FALSE(TestsPoints(pushed, villager));
	const PairMember load {.radius = 1.0f, .noObjectCollision = true, .object = 7};
	EXPECT_FALSE(TestsPoints(load, stone));
	EXPECT_TRUE(TestsPoints(PairMember {.radius = 1.0f, .object = 8}, load));
}

TEST(PhysicsConstantsFile, KeepsThePartOfARowReadBeforeABadNumber)
{
	std::istringstream text("3 2\n1 2 3 x 5 6\n");
	const auto file = physconst::Parse(text, 24);
	ASSERT_TRUE(file.has_value());
	ASSERT_EQ(file->rows.size(), 1u);
	EXPECT_FLOAT_EQ(file->rows[0].density, 1.0f);
	EXPECT_FLOAT_EQ(file->rows[0].springK, 2.0f);
	EXPECT_FLOAT_EQ(file->rows[0].dampK, 3.0f);
	EXPECT_FLOAT_EQ(file->rows[0].friction, 0.0f);
	EXPECT_FLOAT_EQ(file->rows[0].drag, 0.0f);
}

TEST(PhysicsBody, PointsInALineStillGetAnInverseInertia)
{
	// Points in a line can't be turned about it: the inverse comes from a determinant kept at its least, never the
	// identity
	Shape shape;
	shape.points = {{-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
	shape.radius = 1.0f;
	Body body({.mass = 2.0f, .material = k_Stone}, shape);
	body.SetUpPose({});
	body.SetAngularVelocityInBodyAxes({0.0f, 1.0f, 0.0f});
	// The identity would turn its spin of 2 about y into 2; the game's inverse of the flat tensor gives none
	const auto spin = body.AngularVelocity();
	EXPECT_TRUE(std::isfinite(spin.y));
	EXPECT_NEAR(spin.y, 0.0f, 1e-3f);
}

TEST(PhysicsBody, ABodyWithNoPointsGetsNoInertiaButItsDragIsScaled)
{
	const Shape empty {.radius = 2.0f};
	auto material = k_Stone;
	material.drag = 1.0f;
	const Body body({.mass = 1.0f, .material = material}, empty);
	EXPECT_FLOAT_EQ(body.Inertia()[0][0], 0.0f);
	EXPECT_FLOAT_EQ(body.Drag(), 1.0f * 2.0f * 2.0f * 0.3f);
}

TEST(PhysicsBody, ASlidingCubeTumblesForwards)
{
	// Friction under a cube sliding along x turns it forwards, about the negative z axis, the right-hand way
	const FakeLand land(0.0f);
	auto body = MakeCube(0.5f, {0.0f, 0.49f, 0.0f});
	body.velocity = {5.0f, 0.0f, 0.0f};
	for (int i = 0; i < 20; ++i)
	{
		Step(body, land);
	}
	EXPECT_LT(body.AngularVelocity().z, 0.0f);
}

TEST(PhysicsBody, TheSeaIsFeltUnderTheFirstPointOverLandAtTheSeasLevel)
{
	// Sea under the first point but land above the sea's level at the centre is no water to the body
	class RaisedLake final: public Ground
	{
	public:
		[[nodiscard]] float HeightAt(glm::vec2) const override { return 0.5f; }
		[[nodiscard]] glm::vec3 NormalAt(glm::vec2) const override { return {0.0f, 1.0f, 0.0f}; }
		[[nodiscard]] bool IsSeaCell(glm::vec2) const override { return true; }
	};
	const RaisedLake lake;
	auto body = MakeCube(0.5f, {0.0f, -0.2f, 0.0f});
	Step(body, lake);
	EXPECT_FALSE(body.inWater);
	const FakeSea sea;
	auto floating = MakeCube(0.5f, {0.0f, -0.2f, 0.0f});
	Step(floating, sea);
	EXPECT_TRUE(floating.inWater);
}

TEST(PhysicsBody, ARestingSkeletonPutsEveryPointOnItsFirstPart)
{
	const FakeLand land(-1000.0f);
	const std::array origins {glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(4.0f, 5.0f, 6.0f)};
	Body creature({.mass = shapes::k_CreatureMass, .material = k_Stone, .dynamic = false},
	              shapes::Creature(3.0f, origins.size()));
	creature.PlacePoints(glm::vec3(2.0f), origins);
	Ellipsoid first {.boxMin = glm::vec3(-1.0f), .boxMax = glm::vec3(1.0f)};
	first.localToWorld[3] = glm::vec4(7.0f, 8.0f, 9.0f, 1.0f);
	first.worldToLocal = glm::inverse(first.localToWorld);
	creature.SetSkeleton({first, Ellipsoid {}});
	creature.resting = true;
	creature.ClearForces();
	creature.TouchGround(land);
	for (const auto& point : creature.Points())
	{
		EXPECT_EQ(point.world, glm::vec3(7.0f, 8.0f, 9.0f));
		EXPECT_FLOAT_EQ(point.length, 1.0f);
		EXPECT_FLOAT_EQ(point.aheadLength, 1.0f);
		EXPECT_FLOAT_EQ(point.depth, 0.0f);
	}
}

TEST(PhysicsBody, TheRestTestLoosensAfterLongInThePhysics)
{
	// A body barely moving rests only once its counter passes the long wait, when the threshold grows past its speed
	const FakeLand land(0.0f);
	auto body = MakeCube(0.5f, {0.0f, 0.5f, 0.0f});
	int steps = 0;
	bool rested = false;
	while (steps < 40000 && !rested)
	{
		body.velocity = {1.2f, 0.0f, 0.0f};
		rested = Step(body, land) == StepResult::Stopped;
		++steps;
	}
	ASSERT_TRUE(rested);
	// The threshold is the counter over 15000, so a speed of 1.2 rests once the counter passes 18000
	EXPECT_GT(body.RestCounter(), 15000);
}

TEST(PhysicsBody, SettlingOnTheLandLeavesThePointsWhereTheyWere)
{
	const FakeLand land(0.0f);
	auto body = MakeCube(0.5f, {0.0f, 3.0f, 0.0f});
	const auto before = body.Points()[0].world;
	body.SettleOnLand(land, false, false);
	EXPECT_NEAR(body.Centre().y, 0.5f, 1e-5f);
	EXPECT_EQ(body.Points()[0].world, before);
}

TEST(PhysicsBody, TheDrawnPoseLiesBetweenTheTurnsPoses)
{
	auto body = MakeCube(0.5f, {0.0f, 0.0f, 0.0f});
	body.BeginTurn();
	body.MoveCentre({2.0f, 0.0f, 0.0f});
	const auto drawn = body.DrawPose(0.5f, false);
	EXPECT_NEAR(drawn.origin.x, 1.0f, 1e-5f);
	// The quarter turn and its undoing
	const glm::mat3 axes(glm::vec3(0.6f, 0.0f, 0.8f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-0.8f, 0.0f, 0.6f));
	const auto back = QuarterTurnedBack(QuarterTurned(axes));
	for (int i = 0; i < 3; ++i)
	{
		EXPECT_NEAR(glm::distance(back[i], axes[i]), 0.0f, 1e-5f);
	}
}

TEST(PhysicsBody, FacesTakeTheirNormalsFromTheirCorners)
{
	Shape shape;
	shape.points = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
	shape.faces = {{0, 1, 2}};
	shape.radius = 1.0f;
	Body body({.mass = 1.0f, .material = k_Stone}, shape);
	body.SetUpPose({});
	EXPECT_NEAR(body.Faces()[0].localNormal.y, 1.0f, 1e-6f);
}

TEST(PhysicsBody, SpinKeptEachStepIsTheShareKeptEachSecondSpreadOverTheSteps)
{
	// Half the spin kept each second: after two hundred steps of a second half of it is left
	auto material = k_Stone;
	material.spinKeptPerSecond = 0.5f;
	const FakeLand land(-1000.0f);
	auto body = MakeCube(0.5f, {0.0f, 100.0f, 0.0f}, material);
	body.SetAngularVelocity({0.0f, 2.0f, 0.0f});
	for (int i = 0; i < 200; ++i)
	{
		Step(body, land);
	}
	EXPECT_NEAR(body.AngularVelocity().y, 1.0f, 1e-3f);
}

TEST(PhysicsShapes, APieceExactlyAtTheThinLimitIsKept)
{
	// Too thin is an area under 0.4 of the square of its reach
	const std::array<std::array<glm::vec3, 3>, 1> wide {{
	    {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
	}};
	const auto piece = shapes::Fragment(wide);
	EXPECT_EQ(piece.tooThin, piece.area < 0.4f * piece.shape.radius * piece.shape.radius);
}

TEST(PhysicsShapes, EachBoneBoxHoldsItsVerticesAndItsOrigin)
{
	const std::array<glm::vec3, 3> positions {glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(-1.0f, 0.5f, 0.5f),
	                                          glm::vec3(5.0f, 5.0f, 5.0f)};
	const std::array<uint16_t, 3> bones {0, 0, shapes::k_NoBone};
	const auto boxes = shapes::BoneBoxes(positions, bones, 2);
	ASSERT_EQ(boxes.size(), 2u);
	EXPECT_EQ(boxes[0].min, glm::vec3(-1.0f, 0.0f, 0.0f));
	EXPECT_EQ(boxes[0].max, glm::vec3(1.0f, 2.0f, 3.0f));
	// A bone with no vertices keeps a box at its origin
	EXPECT_EQ(boxes[1].min, glm::vec3(0.0f));
	EXPECT_EQ(boxes[1].max, glm::vec3(0.0f));
}
