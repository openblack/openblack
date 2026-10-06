/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <string>

#include <Graphics/UniformTable.h>
#include <gtest/gtest.h>

using openblack::graphics::UniformHandle;
using openblack::graphics::UniformTable;

TEST(UniformTable, EmptyFindsNothing)
{
	const UniformTable table;
	EXPECT_FALSE(table.Find("u_tint").has_value());
	EXPECT_FALSE(table.Contains(""));
}

TEST(UniformTable, FindsEachByName)
{
	UniformTable table;
	for (uint32_t i = 0; i < 40; ++i)
	{
		table.Add("u_uniform" + std::to_string(i), UniformHandle {.id = i + 100});
	}
	EXPECT_EQ(table.Size(), 40u);
	for (uint32_t i = 0; i < 40; ++i)
	{
		const auto found = table.Find("u_uniform" + std::to_string(i));
		ASSERT_TRUE(found.has_value()) << i;
		EXPECT_EQ(found->id, i + 100);
	}
	EXPECT_FALSE(table.Find("u_uniform40").has_value());
	EXPECT_FALSE(table.Find("u_uniform").has_value());
}

TEST(UniformTable, FirstHandleOfANameStays)
{
	UniformTable table;
	table.Add("s_diffuse", UniformHandle {.id = 1});
	table.Add("s_diffuse", UniformHandle {.id = 2});
	EXPECT_EQ(table.Size(), 1u);
	EXPECT_EQ(table.Find("s_diffuse")->id, 1u);
}

TEST(UniformTable, NamesArePrefixesOfEachOther)
{
	UniformTable table;
	table.Add("u_haze", UniformHandle {.id = 1});
	table.Add("u_hazeColour", UniformHandle {.id = 2});
	EXPECT_EQ(table.Find("u_haze")->id, 1u);
	EXPECT_EQ(table.Find("u_hazeColour")->id, 2u);
	// A name given as a longer string's start is only that name
	const std::string longer = "u_hazeColour";
	EXPECT_EQ(table.Find(std::string_view(longer).substr(0, 6))->id, 1u);
}
