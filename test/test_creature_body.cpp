/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "3D/CreatureBody.h"

using namespace openblack;
using namespace openblack::creature;

using A = CreatureBody::Appearance;

TEST(CreatureBody, NamesTheMeshOfEachAppearance)
{
	EXPECT_EQ(GetIdFromMeshName("A_Tiger2_Base"), GetIdFromType(CreatureType::Tiger, A::Base));
	EXPECT_EQ(GetIdFromMeshName("C_Cow_Boned"), GetIdFromType(CreatureType::Cow, A::Base));
	EXPECT_EQ(GetIdFromMeshName("A_Bear_Boned_Fat"), GetIdFromType(CreatureType::BrownBear, A::Fat));
	EXPECT_EQ(GetIdFromMeshName("C_wolf_evil2"), GetIdFromType(CreatureType::Wolf, A::Evil));
	EXPECT_EQ(GetIdFromMeshName("C_CHIMP_strong"), GetIdFromType(CreatureType::Chimp, A::Strong));
}

TEST(CreatureBody, TheOgresVariantsKeepItsBaseName)
{
	EXPECT_EQ(GetIdFromMeshName("A_Greek_Boned_base"), GetIdFromType(CreatureType::Ogre, A::Base));
	EXPECT_EQ(GetIdFromMeshName("A_Greek_Boned_Base_Evil"), GetIdFromType(CreatureType::Ogre, A::Evil));
	EXPECT_EQ(GetIdFromMeshName("A_Greek_Boned_Base_Thin"), GetIdFromType(CreatureType::Ogre, A::Thin));
}

TEST(CreatureBody, TheCreatureTablesStartWithTheGiantApe)
{
	EXPECT_EQ(InfoRow(CreatureType::GiantApe), 0u);
	EXPECT_EQ(InfoRow(CreatureType::Cow), 1u);
	EXPECT_EQ(InfoRow(CreatureType::Gorilla), 16u);
}
