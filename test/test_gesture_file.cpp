/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdlib>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>

#include <GestureFile.h>
#include <gtest/gtest.h>

using namespace openblack::gestures;

namespace
{
GestureTemplate MakeTemplate(uint8_t gesture, size_t points, uint32_t junkHighBytes)
{
	GestureTemplate entry;
	for (size_t i = 0; i < points; ++i)
	{
		entry.points.at(i) = {.x = static_cast<float>(i) / 10.0f,
		                      .y = 0.0f,
		                      .z = 0.5f,
		                      .turn = (i % 2 == 0) ? 1.5f : -1.5f,
		                      .direction = static_cast<uint32_t>(i % 8)};
	}
	// Left-over memory after the points is kept as it was
	entry.points.back().x = 123.0f;
	entry.pointCountField = static_cast<uint32_t>(points) | junkHighBytes;
	entry.gestureField = gesture | junkHighBytes;
	entry.positionModeField = 2u | junkHighBytes;
	entry.checkDirection = 1;
	entry.allowMirror = 0;
	entry.checkAspectRatio = 1;
	entry.aspectRatio = 0.75f;
	return entry;
}
} // namespace

TEST(GestureFile, WritesAndReadsBackTheSameTemplates)
{
	GestureFile file;
	file.AddTemplate(MakeTemplate(4, 6, 0xABCD0000u));
	file.AddTemplate(MakeTemplate(5, 3, 0));
	const auto bytes = file.Write();
	ASSERT_EQ(bytes.size(), sizeof(int32_t) + (2 * GestureFile::k_RecordSize));
	ASSERT_EQ(GestureFile::k_RecordSize, 1628u);

	GestureFile read;
	ASSERT_EQ(read.Open(bytes), GestureFileResult::Success);
	ASSERT_EQ(read.GetTemplates().size(), 2u);
	const auto& circle = read.GetTemplates().front();
	EXPECT_EQ(circle.Gesture(), 4);
	EXPECT_EQ(circle.PointCount(), 6u);
	EXPECT_EQ(circle.PositionModeValue(), 2);
	EXPECT_TRUE(circle.ChecksDirection());
	EXPECT_FALSE(circle.AllowsMirror());
	EXPECT_TRUE(circle.ChecksAspectRatio());
	EXPECT_FLOAT_EQ(circle.aspectRatio, 0.75f);
	EXPECT_FLOAT_EQ(circle.Points()[3].x, 0.3f);
	EXPECT_EQ(circle.Points()[5].direction, 5u);
	EXPECT_EQ(read.Write(), bytes);
}

TEST(GestureFile, RefusesFilesOfTheWrongSize)
{
	GestureFile file;
	EXPECT_EQ(file.Open(std::vector<uint8_t> {1, 0}), GestureFileResult::ErrFileTooSmall);
	std::vector<uint8_t> bytes(sizeof(int32_t) + GestureFile::k_RecordSize - 1, 0);
	bytes[0] = 1;
	EXPECT_EQ(file.Open(bytes), GestureFileResult::ErrSizeMismatch);
	EXPECT_TRUE(file.GetTemplates().empty());
}

TEST(GestureFile, RefusesTooManyPoints)
{
	GestureFile file;
	file.AddTemplate(MakeTemplate(4, 3, 0));
	auto bytes = file.Write();
	// The point count's low byte, just after the points
	bytes.at(sizeof(int32_t) + (GestureTemplate::k_MaxPoints * sizeof(TemplatePoint))) = 81;
	GestureFile read;
	EXPECT_EQ(read.Open(bytes), GestureFileResult::ErrTooManyPoints);
}

TEST(GestureFile, ReadsTheGamesTemplates)
{
	const char* gamePath = std::getenv("OPENBLACK_GAME_PATH");
	if (gamePath == nullptr)
	{
		GTEST_SKIP() << "OPENBLACK_GAME_PATH is not set";
	}
	const auto path = std::filesystem::path(gamePath) / "Data" / "Gestures.jty";
	if (!std::filesystem::exists(path))
	{
		GTEST_SKIP() << "No gesture templates in the game folder";
	}
	GestureFile file;
	ASSERT_EQ(file.Open(path), GestureFileResult::Success);
	const auto& templates = file.GetTemplates();
	ASSERT_EQ(templates.size(), 81u);
	const auto countOf = [&templates](uint8_t gesture) {
		return std::ranges::count_if(templates, [gesture](const auto& entry) { return entry.Gesture() == gesture; });
	};
	EXPECT_EQ(countOf(4), 1);   // circle
	EXPECT_EQ(countOf(5), 2);   // scribble
	EXPECT_EQ(countOf(13), 10); // heart
	EXPECT_EQ(countOf(15), 1);  // square spiral, the leash gesture
	for (const auto& entry : templates)
	{
		EXPECT_EQ(entry.PositionModeValue(), 2);
		EXPECT_GE(entry.PointCount(), 6u);
		EXPECT_GE(entry.Gesture(), 1);
		EXPECT_LE(entry.Gesture(), 23);
		for (const auto& point : entry.Points())
		{
			EXPECT_LT(point.direction, 8u);
		}
	}
	// Only the circle and the stars may be drawn mirrored
	EXPECT_TRUE(std::ranges::all_of(
	    templates, [](const auto& entry) { return !entry.AllowsMirror() || entry.Gesture() == 4 || entry.Gesture() == 8; }));

	// Written back byte for byte
	std::ifstream stream(path, std::ios::binary);
	const std::vector<uint8_t> original((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
	EXPECT_EQ(file.Write(), original);
}
