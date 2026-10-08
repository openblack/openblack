/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <optional>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

#include "3D/LandLine.h"
#include "3D/ScreenPick.h"
#include "ECS/Systems/PickingSystemInterface.h"
#include "Graphics/Sun.h"

namespace land_line = openblack::land_line;
namespace screen_pick = openblack::screen_pick;

namespace
{
/// Land of the same height everywhere on the map
land_line::CellLookup Flat(uint8_t height)
{
	return [height](int32_t, int32_t) -> std::optional<land_line::CellHeights> {
		return land_line::CellHeights {.here = height, .acrossX = height, .acrossZ = height, .acrossBoth = height};
	};
}

std::optional<land_line::CellHeights> Sea(int32_t /*unused*/, int32_t /*unused*/)
{
	return std::nullopt;
}

constexpr float k_Metres = land_line::k_CellSize;
constexpr float k_Height = land_line::k_HeightUnit;
} // namespace

TEST(LandLine, ALineDownMeetsFlatLandWhereItReachesItsHeight)
{
	// From 10 height units above flat land of 20, down at 45 degrees along x
	const glm::vec3 from(100.5f * k_Metres, 30.0f * k_Height, 50.5f * k_Metres);
	const glm::vec3 to(from.x + 5.0f * k_Metres, 25.0f * k_Height, from.z);
	const auto hit = land_line::LandAlong(from, to, Flat(20));
	ASSERT_TRUE(hit.has_value());
	// Ten height units down is twice the five cells the line drops five units over
	EXPECT_NEAR(hit->x, 110.5f * k_Metres, 0.01f);
	EXPECT_NEAR(hit->y, 50.5f * k_Metres, 0.01f);
}

TEST(LandLine, ALineIsCarriedOnToTheMapsEdge)
{
	// The line stops short of the land, but the game carries it on
	const glm::vec3 from(10.5f * k_Metres, 30.0f * k_Height, 10.5f * k_Metres);
	const glm::vec3 to(11.5f * k_Metres, 29.0f * k_Height, 11.5f * k_Metres);
	const auto hit = land_line::LandAlong(from, to, Flat(20));
	ASSERT_TRUE(hit.has_value());
	EXPECT_NEAR(hit->x, 20.5f * k_Metres, 0.01f);
	EXPECT_NEAR(hit->y, 20.5f * k_Metres, 0.01f);
}

TEST(LandLine, ALineAboveTheLandOrOverTheSeaMissesIt)
{
	const glm::vec3 from(100.5f * k_Metres, 30.0f * k_Height, 50.5f * k_Metres);
	const glm::vec3 up(110.5f * k_Metres, 40.0f * k_Height, 50.5f * k_Metres);
	EXPECT_FALSE(land_line::LandAlong(from, up, Flat(20)).has_value());
	EXPECT_FALSE(land_line::LandAlong(from, {from.x + 10.0f, 0.0f, from.z}, Sea).has_value());
}

TEST(LandLine, NothingBehindTheLinesStartCounts)
{
	// Starting just above the land and rising, the land below and behind the start isn't met
	EXPECT_FALSE(land_line::HitInCell(100, 50, {100.5f, 21.0f, 50.5f}, {100.6f, 22.0f, 50.5f}, Flat(20)).has_value());
}

TEST(LandLine, ACellsTrianglesReachATenthPastItsEdges)
{
	// A line through the cell's plane just past its far x edge still meets this cell's land
	const auto hit = land_line::HitInCell(5, 5, {6.05f, 21.0f, 5.5f}, {6.05f, 19.0f, 5.5f}, Flat(20));
	ASSERT_TRUE(hit.has_value());
	EXPECT_NEAR(hit->x, 6.05f, 1e-4f);
	// Further out it doesn't
	EXPECT_FALSE(land_line::HitInCell(5, 5, {6.15f, 21.0f, 5.5f}, {6.15f, 19.0f, 5.5f}, Flat(20)).has_value());
}

TEST(LandLine, ALineLevelWithTheNearTriangleMissesTheWholeCell)
{
	// Along a level line at the land's own height the first triangle is never met, and the game gives up on the cell
	EXPECT_FALSE(land_line::HitInCell(5, 5, {5.1f, 20.0f, 5.9f}, {5.9f, 20.0f, 5.1f}, Flat(20)).has_value());
}

TEST(LandLine, TheFirstCellAlongTheLineIsTheOneMet)
{
	// Two hills on the line: the nearer is met
	const auto cells = [](int32_t x, int32_t z) -> std::optional<land_line::CellHeights> {
		// Two ramps rising across x
		const bool ramp = z == 50 && (x == 103 || x == 108);
		const uint8_t rise = ramp ? 200 : 0;
		return land_line::CellHeights {.here = 0, .acrossX = rise, .acrossZ = 0, .acrossBoth = rise};
	};
	const glm::vec3 from(100.5f * k_Metres, 100.0f * k_Height, 50.5f * k_Metres);
	const glm::vec3 to(101.5f * k_Metres, 100.0f * k_Height, 50.5f * k_Metres);
	const auto hit = land_line::LandAlong(from, to, cells);
	ASSERT_TRUE(hit.has_value());
	EXPECT_NEAR(hit->x, 103.5f * k_Metres, 0.05f);
}

TEST(LandLine, ALineFromOffTheMapIsCutToItsEdge)
{
	const glm::vec3 from(-50.0f * k_Metres, 100.0f * k_Height, 50.5f * k_Metres);
	const glm::vec3 to(-40.0f * k_Metres, 90.0f * k_Height, 50.5f * k_Metres);
	const auto hit = land_line::LandAlong(from, to, Flat(20));
	ASSERT_TRUE(hit.has_value());
	EXPECT_NEAR(hit->x, 30.0f * k_Metres, 0.05f);
}

TEST(LandLine, TheSeaIsMetGoingDownNearTheCamera)
{
	const glm::vec3 camera(1000.0f, 100.0f, 1000.0f);
	const auto sea = land_line::LandOrSeaAlong(camera, {1010.0f, 90.0f, 1000.0f}, camera, Sea);
	ASSERT_TRUE(sea.has_value());
	EXPECT_NEAR(sea->x, 1100.0f, 1e-3f);
	// Not looking up, nor further than 7500 across
	EXPECT_FALSE(land_line::LandOrSeaAlong(camera, {1010.0f, 110.0f, 1000.0f}, camera, Sea).has_value());
	EXPECT_FALSE(land_line::LandOrSeaAlong(camera, {1100.0f, 99.0f, 1000.0f}, camera, Sea).has_value());
	// The pixel's point goes on to the sea wherever it is, while looking down
	const auto under = land_line::UnderPixel(camera, {1100.0f, 99.0f, 1000.0f}, true, Sea, [](glm::vec2) { return 0.0f; });
	ASSERT_TRUE(under.has_value());
	EXPECT_NEAR(under->x, 11000.0f, 0.5f);
	EXPECT_FALSE(
	    land_line::UnderPixel(camera, {1100.0f, 99.0f, 1000.0f}, false, Sea, [](glm::vec2) { return 0.0f; }).has_value());
}

TEST(LandLine, ThePointUnderTheCursorIsKeptNearTheMap)
{
	const auto kept = land_line::KeptInReach({2560.0f + 10000.0f, 0.0f, 2560.0f});
	EXPECT_NEAR(kept.x, 2560.0f + 7680.0f, 0.01f);
	const auto inside = land_line::KeptInReach({100.0f, 5.0f, 200.0f});
	EXPECT_FLOAT_EQ(inside.x, 100.0f);
	EXPECT_FLOAT_EQ(inside.y, 5.0f);
}

namespace
{
/// A camera at the origin looking down -z, 800 by 600 pixels, the cursor at the middle
screen_pick::View View(glm::vec2 cursor = {400.0f, 300.0f})
{
	constexpr float k_Near = 1.0f;
	const auto projection = glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, k_Near, 1000.0f);
	const auto view = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	return {.worldToClip = projection * view,
	        .resolution = {800.0f, 600.0f},
	        .near = k_Near,
	        .xScale = projection[0][0],
	        .camera = glm::vec3(0.0f),
	        .cursor = cursor};
}

std::vector<screen_pick::ClipCorner> Corners(const screen_pick::View& view, std::initializer_list<glm::vec3> points)
{
	std::vector<screen_pick::ClipCorner> corners;
	for (const auto& point : points)
	{
		corners.push_back(screen_pick::ToClip(view, point, glm::vec2(0.25f)));
	}
	return corners;
}
} // namespace

TEST(ScreenPick, TheCursorMustBeInsideTheSpheresCircle)
{
	const auto view = View();
	EXPECT_TRUE(screen_pick::CursorOverSphere(view, {0.0f, 0.0f, -10.0f}, 1.0f, {0.0f, 0.0f, -10.0f}));
	EXPECT_FALSE(screen_pick::CursorOverSphere(view, {5.0f, 0.0f, -10.0f}, 1.0f, {5.0f, 0.0f, -10.0f}));
	// Behind the near plane
	EXPECT_FALSE(screen_pick::CursorOverSphere(view, {0.0f, 0.0f, 10.0f}, 1.0f, {0.0f, 0.0f, 10.0f}));
	// A camera inside the sphere is over it wherever the cursor is
	EXPECT_TRUE(screen_pick::CursorOverSphere(View({0.0f, 0.0f}), {0.0f, 0.0f, -1.0f}, 3.0f, {0.0f, 0.0f, -1.0f}));
}

TEST(ScreenPick, ATriangleFacingTheCameraIsHitAtItsDepth)
{
	const auto view = View();
	const auto facing = Corners(view, {{-1.0f, -1.0f, -10.0f}, {1.0f, -1.0f, -10.0f}, {0.0f, 1.0f, -10.0f}});
	const std::array<uint16_t, 3> forwards {0, 2, 1};
	const std::array<uint16_t, 3> backwards {0, 1, 2};
	// One way round the triangle faces the camera on the screen, the other way it is dropped unless two-sided
	const auto front = screen_pick::FirstHit(view, {.corners = facing, .indices = forwards});
	const auto back = screen_pick::FirstHit(view, {.corners = facing, .indices = backwards});
	ASSERT_NE(front.has_value(), back.has_value());
	const auto& hit = front.has_value() ? front : back;
	EXPECT_NEAR(*hit, 10.0f, 1e-3f);
	const auto twoSided =
	    screen_pick::FirstHit(view, {.corners = facing, .indices = front.has_value() ? backwards : forwards, .twoSided = true});
	EXPECT_TRUE(twoSided.has_value());
}

TEST(ScreenPick, ATriangleReachingBehindTheNearPlaneIsCutToIt)
{
	const auto view = View();
	// A floor under the camera running from behind it to far ahead, seen with the cursor low on the screen
	const auto low = View({400.0f, 500.0f});
	const auto corners = Corners(low, {{-5.0f, -1.0f, 5.0f}, {5.0f, -1.0f, 5.0f}, {0.0f, -1.0f, -50.0f}});
	const auto hitA = screen_pick::FirstHit(low, {.corners = corners, .indices = std::array<uint16_t, 3> {0, 1, 2}});
	const auto hitB = screen_pick::FirstHit(low, {.corners = corners, .indices = std::array<uint16_t, 3> {0, 2, 1}});
	ASSERT_TRUE(hitA.has_value() || hitB.has_value());
	const float depth = hitA.has_value() ? *hitA : *hitB;
	EXPECT_GT(depth, view.near);
	EXPECT_LT(depth, 50.0f);
}

TEST(ScreenPick, TheFirstTriangleCoveringTheCursorWinsNotTheNearest)
{
	const auto view = View();
	const auto corners = Corners(view, {{-1.0f, -1.0f, -20.0f},
	                                    {1.0f, -1.0f, -20.0f},
	                                    {0.0f, 1.0f, -20.0f},
	                                    {-1.0f, -1.0f, -5.0f},
	                                    {1.0f, -1.0f, -5.0f},
	                                    {0.0f, 1.0f, -5.0f}});
	const std::array<uint16_t, 6> indices {0, 2, 1, 3, 5, 4};
	const auto hit = screen_pick::FirstHit(view, {.corners = corners, .indices = indices, .twoSided = true});
	ASSERT_TRUE(hit.has_value());
	EXPECT_NEAR(*hit, 20.0f, 1e-3f);
}

TEST(ScreenPick, AHoleInTheTextureLetsTheCursorThrough)
{
	const auto view = View();
	const auto corners = Corners(view, {{-1.0f, -1.0f, -10.0f}, {1.0f, -1.0f, -10.0f}, {0.0f, 1.0f, -10.0f}});
	screen_pick::AlphaMask empty {};
	screen_pick::AlphaMask solid {};
	solid.fill(true);
	const std::array<uint16_t, 3> indices {0, 2, 1};
	EXPECT_FALSE(
	    screen_pick::FirstHit(view, {.corners = corners, .indices = indices, .twoSided = true, .mask = &empty}).has_value());
	EXPECT_TRUE(
	    screen_pick::FirstHit(view, {.corners = corners, .indices = indices, .twoSided = true, .mask = &solid}).has_value());
}

TEST(ScreenPick, ATexturesMaskIsSolidWhereItsAlphaIs)
{
	std::vector<uint16_t> texels(256 * 256, 0x0FFF);
	// The top left texel of the second sampled column is solid
	texels[4] = 0xF000;
	const auto mask = screen_pick::MaskOf(texels, 256, 256);
	EXPECT_FALSE(screen_pick::Solid(mask, {0.0f, 0.0f}));
	EXPECT_TRUE(screen_pick::Solid(mask, {1.0f / 64.0f, 0.0f}));
	// Coordinates outside are kept on the mask's edge
	EXPECT_FALSE(screen_pick::Solid(mask, {-1.0f, -1.0f}));
}

TEST(ScreenPick, TheLandWinsOnlyWhenNearerAndOffTheObjectsFootprint)
{
	const glm::vec2 origin(100.0f, 100.0f);
	const glm::vec2 extents(3.0f, 4.0f);
	// Farther land never wins
	EXPECT_FALSE(screen_pick::LandHidesObject(20.0f, 10.0f, {200.0f, 200.0f}, origin, extents));
	// Nearer land on the footprint doesn't, off it does
	EXPECT_FALSE(screen_pick::LandHidesObject(5.0f, 10.0f, {102.0f, 102.0f}, origin, extents));
	EXPECT_TRUE(screen_pick::LandHidesObject(5.0f, 10.0f, {110.0f, 100.0f}, origin, extents));
	// As near counts as nearer
	EXPECT_TRUE(screen_pick::LandHidesObject(10.0f, 10.0f, {110.0f, 100.0f}, origin, extents));
	// The footprint's edge itself is off it
	EXPECT_TRUE(screen_pick::LandHidesObject(5.0f, 10.0f, {105.0f, 100.0f}, origin, extents));
}

TEST(ScreenPick, ALineFeelsTheNearestTriangleAndItsNormalAlongTheLine)
{
	const std::array<glm::vec3, 6> corners {{
	    {-1.0f, 0.0f, -1.0f},
	    {1.0f, 0.0f, -1.0f},
	    {0.0f, 0.0f, 1.0f},
	    {-1.0f, 2.0f, -1.0f},
	    {1.0f, 2.0f, -1.0f},
	    {0.0f, 2.0f, 1.0f},
	}};
	const std::array<uint16_t, 6> indices {0, 1, 2, 3, 4, 5};
	const auto hit = screen_pick::NearestIntersection(corners, indices, {0.0f, 10.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, false);
	ASSERT_TRUE(hit.has_value());
	EXPECT_NEAR(hit->point.y, 2.0f, 1e-5f);
	EXPECT_NEAR(hit->distance, 8.0f, 1e-5f);
	EXPECT_GT(hit->normal.y * -1.0f, 0.0f);
	// The upper triangle, a quarter along its first side and half along its third
	EXPECT_EQ(hit->firstIndex, 3u);
	EXPECT_NEAR(hit->s, 0.25f, 1e-5f);
	EXPECT_NEAR(hit->t, 0.5f, 1e-5f);
	// A line along the faces' plane meets neither
	EXPECT_FALSE(
	    screen_pick::NearestIntersection(corners, indices, {-5.0f, 2.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, false).has_value());
	// Nothing behind the start, unless asked
	EXPECT_FALSE(
	    screen_pick::NearestIntersection(corners, indices, {0.0f, 10.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, false).has_value());
	EXPECT_TRUE(screen_pick::NearestIntersection(corners, indices, {0.0f, 10.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, true).has_value());
}

TEST(SunGlare, TheSamplesLieAcrossTheWorldsXAndUpItsY)
{
	namespace sun = openblack::graphics::sun;
	const glm::vec3 position(-30000.0f, 7500.0f, -30000.0f);
	const auto samples = sun::GlareSamples(position, position + glm::vec3(0.0f, 0.0f, 100.0f), 2.0f);
	// Moved on away from the camera by the near plane's distance
	EXPECT_NEAR(samples[0].z, position.z - 2.0f, 1e-3f);
	EXPECT_NEAR(samples[1].x, position.x - 500.0f, 1e-3f);
	EXPECT_NEAR(samples[1].y, position.y - 500.0f, 1e-3f);
	EXPECT_NEAR(samples[4].x, position.x + 500.0f, 1e-3f);
	EXPECT_NEAR(samples[4].y, position.y + 500.0f, 1e-3f);
	// Never lower than 10
	const auto low = sun::GlareSamples({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 100.0f}, 1.0f);
	EXPECT_FLOAT_EQ(low[1].y, 10.0f);
	// Hidden by what is nearer than about 122 near planes
	EXPECT_NEAR(sun::GlareHidingDepth(1.0f), 65535.0f / 535.5f, 1e-2f);
}

namespace
{
/// The cells a line's walk asks the land of, in order, over land that is never met
std::vector<glm::ivec2> WalkedCells(glm::vec3 from, glm::vec3 to)
{
	std::vector<glm::ivec2> asked;
	const land_line::CellLookup record = [&asked](int32_t x, int32_t z) -> std::optional<land_line::CellHeights> {
		asked.emplace_back(x, z);
		return std::nullopt;
	};
	EXPECT_FALSE(land_line::FirstHit(from, to, record).has_value());
	return asked;
}

/// The cells a straight line crosses from its start, nearest first, found by stepping finely along it
std::vector<glm::ivec2> CellsCrossed(glm::vec2 from, glm::vec2 to, size_t count)
{
	std::vector<glm::ivec2> cells;
	constexpr int k_Steps = 100000;
	for (int i = 0; i <= k_Steps && cells.size() < count; ++i)
	{
		const auto at = glm::mix(from, to, static_cast<float>(i) / static_cast<float>(k_Steps));
		const glm::ivec2 cell(static_cast<int32_t>(at.x), static_cast<int32_t>(at.y));
		if (cells.empty() || cells.back() != cell)
		{
			cells.push_back(cell);
		}
	}
	return cells;
}
} // namespace

TEST(LandLine, TheWalkCrossesTheCellsTheLineDoesInEachDirection)
{
	// One line each way across x and z, none of them along an axis or through a cell's corner
	const std::array<glm::vec2, 4> towards = {glm::vec2(2.0f, 1.0f), glm::vec2(2.0f, -1.0f), glm::vec2(-2.0f, 1.0f),
	                                          glm::vec2(-2.0f, -1.0f)};
	for (const auto& step : towards)
	{
		const glm::vec2 start(100.5f, 100.2f);
		const auto end = start + step * 1.5f;
		const auto walked = WalkedCells({start.x, 50.0f, start.y}, {end.x, 50.0f, end.y});
		ASSERT_GE(walked.size(), 8u);
		// The line is carried on to the map's edge, so the first cells are those the line crosses on its way there
		const auto crossed = CellsCrossed(start, start + step * 50.0f, 8);
		for (size_t i = 0; i < crossed.size(); ++i)
		{
			EXPECT_EQ(walked[i], crossed[i]) << "towards " << step.x << ", " << step.y << " at cell " << i;
		}
	}
}

TEST(ScreenPick, AnObjectFurtherThanTheNearestByMoreThanItsRadiusIsNotTested)
{
	const auto view = View();
	const std::array<screen_pick::Candidate, 3> candidates = {{
	    {.centre = {0.0f, 0.0f, -10.0f}, .radius = 1.0f, .origin = {0.0f, 0.0f, -10.0f}},
	    {.centre = {0.0f, 0.0f, -30.0f}, .radius = 1.0f, .origin = {0.0f, 0.0f, -30.0f}},
	    {.centre = {0.0f, 0.0f, -10.5f}, .radius = 1.0f, .origin = {0.0f, 0.0f, -10.5f}},
	}};
	std::vector<size_t> tested;
	const auto picked = screen_pick::PickAmong(view, candidates, [&tested](size_t i) -> std::optional<float> {
		tested.push_back(i);
		return i == 2 ? 10.0f : 10.0f + static_cast<float>(i);
	});
	ASSERT_TRUE(picked.has_value());
	// The far one is passed by; the third, as near as the first, doesn't take its place
	EXPECT_EQ(tested, (std::vector<size_t> {0, 2}));
	EXPECT_EQ(picked->index, 0u);
	EXPECT_FLOAT_EQ(picked->distance, 10.0f);
}

TEST(ScreenPick, OnlyAStrictlyNearerObjectTakesThePick)
{
	const auto view = View();
	const std::array<screen_pick::Candidate, 2> candidates = {{
	    {.centre = {0.0f, 0.0f, -12.0f}, .radius = 3.0f, .origin = {0.0f, 0.0f, -12.0f}},
	    {.centre = {0.0f, 0.0f, -10.0f}, .radius = 3.0f, .origin = {0.0f, 0.0f, -10.0f}},
	}};
	const auto picked =
	    screen_pick::PickAmong(view, candidates, [](size_t i) -> std::optional<float> { return i == 0 ? 12.0f : 9.0f; });
	ASSERT_TRUE(picked.has_value());
	EXPECT_EQ(picked->index, 1u);
	// A miss is nothing picked
	EXPECT_FALSE(
	    screen_pick::PickAmong(view, candidates, [](size_t) -> std::optional<float> { return std::nullopt; }).has_value());
}

TEST(Picking, GrippingTheLandKeepsWhatWasPickedAndFollowsOnlyTheLand)
{
	using Pick = openblack::ecs::systems::PickingSystemInterface::Pick;
	namespace picking = openblack::ecs::systems::picking;
	const Pick before {.object = entt::entity {7},
	                   .point = glm::vec3(1.0f, 2.0f, 3.0f),
	                   .distance = 50.0f,
	                   .land = glm::vec3(4.0f, 5.0f, 6.0f),
	                   .hoverObject = entt::entity {7},
	                   .hoverSeconds = 2.0f};
	const auto moved = picking::Locked(before, glm::vec3(10.0f, 0.0f, 10.0f), 30.0f);
	EXPECT_EQ(moved.object, before.object);
	EXPECT_EQ(moved.point, glm::vec3(10.0f, 0.0f, 10.0f));
	EXPECT_FLOAT_EQ(moved.distance, 30.0f);
	EXPECT_FLOAT_EQ(moved.hoverSeconds, 2.0f);
	// Off the land the last point is kept
	const auto kept = picking::Locked(before, std::nullopt, 30.0f);
	EXPECT_EQ(kept.point, before.point);
	EXPECT_FLOAT_EQ(kept.distance, before.distance);
}

TEST(Picking, TheHoverTimeRunsWhileTheSameThingStaysPicked)
{
	using Pick = openblack::ecs::systems::PickingSystemInterface::Pick;
	namespace picking = openblack::ecs::systems::picking;
	Pick previous;
	Pick next;
	// Nothing picked counts as staying the same
	picking::CarryHover(previous, next, 0.5f);
	EXPECT_FLOAT_EQ(next.hoverSeconds, 0.5f);
	previous = next;
	next = {.object = entt::entity {3}};
	picking::CarryHover(previous, next, 0.5f);
	EXPECT_FLOAT_EQ(next.hoverSeconds, 0.0f);
	EXPECT_EQ(next.hoverObject, entt::entity {3});
	previous = next;
	next = {.object = entt::entity {3}};
	picking::CarryHover(previous, next, 0.25f);
	EXPECT_FLOAT_EQ(next.hoverSeconds, 0.25f);
}
