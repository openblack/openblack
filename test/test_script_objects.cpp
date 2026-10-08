/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "ECS/ScriptObjectTable.h"

using namespace openblack::ecs::script_objects;

TEST(ScriptObjects, OnlyTheNativesThatMoveOrSetObjectsTakeControl)
{
	EXPECT_TRUE(TakesControl(17));  // set script state
	EXPECT_TRUE(TakesControl(33));  // move a thing
	EXPECT_TRUE(TakesControl(218)); // start a refereed match
	EXPECT_FALSE(TakesControl(27)); // create
	EXPECT_FALSE(TakesControl(110));
}

TEST(ScriptObjects, AnObjectInAScriptKeepsItsPlace)
{
	Table table;
	const auto first = table.Register(42, true, false);
	ASSERT_TRUE(first.has_value());
	EXPECT_EQ(*first, 1);
	// Not yet in a script, it would be given another place, as the game does
	EXPECT_EQ(table.Register(42, false, false), 2);
	EXPECT_EQ(table.Register(42, false, true), first);
	EXPECT_TRUE(table.At(*first).createdByScript);
}

TEST(ScriptObjects, TheFirstReferenceIsTold)
{
	Table table;
	const auto place = *table.Register(7, false, false);
	EXPECT_EQ(table.AddReference(place), Referenced::First);
	EXPECT_EQ(table.AddReference(place), Referenced::Again);
	table.RemoveReference(place);
	table.RemoveReference(place);
	table.RemoveReference(place);
	EXPECT_EQ(table.At(place).count, 0);
	// Let go of every reference, the place is still the object's: places are only freed when the land's scripts end
	EXPECT_EQ(table.Find(7), place);
	EXPECT_EQ(table.AddReference(0), Referenced::Nothing);
}

TEST(ScriptObjects, TheTableFillsAndIsClearedWithTheLand)
{
	Table table;
	for (uint32_t object = 1; object < k_Places; ++object)
	{
		ASSERT_TRUE(table.Register(object, false, false).has_value());
	}
	EXPECT_FALSE(table.Register(9999, false, false).has_value());
	table.Clear();
	EXPECT_TRUE(table.Register(9999, false, false).has_value());
}

TEST(ScriptObjects, ADeadTreeTakesItsTreesPlace)
{
	Table table;
	const auto place = *table.Register(5, false, false);
	table.Replace(5, 6);
	EXPECT_EQ(table.Find(6), place);
	EXPECT_FALSE(table.Find(5).has_value());
}
