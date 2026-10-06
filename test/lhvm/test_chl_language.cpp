/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <ChlConstants.h>
#include <ChlForms.h>
#include <ChlSyntax.h>
#include <LHVMNatives.h>
#include <gtest/gtest.h>

#include "CHLApi.h"

using namespace openblack::lhvm;
using namespace openblack::lhvm::chl;

TEST(ChlLanguage, NativeTableMatchesTheGameBindings)
{
	openblack::chlapi::CHLApi api;
	const auto& bound = api.GetFunctionsTable();
	const auto natives = DefaultNativeSignatures();
	ASSERT_EQ(natives.size(), bound.size());
	for (size_t i = 0; i < natives.size(); ++i)
	{
		EXPECT_EQ(natives[i].name, bound[i].name) << i;
		EXPECT_EQ(natives[i].stackIn, bound[i].stackIn) << bound[i].name;
		EXPECT_EQ(natives[i].stackOut, bound[i].stackOut) << bound[i].name;
	}
}

TEST(ChlLanguage, PatternItems)
{
	const auto items = ParsePattern("move $0 position to $1 [radius $2?1] {with sound}$3 (good=1|evil=2|=0)$4 $5=86400");
	ASSERT_EQ(items.size(), 9);
	EXPECT_EQ(items[0].kind, PatternItemKind::Word);
	EXPECT_EQ(items[0].text, "move");
	EXPECT_EQ(items[1].kind, PatternItemKind::Argument);
	EXPECT_EQ(items[1].argument, 0);
	EXPECT_EQ(items[5].kind, PatternItemKind::Optional);
	ASSERT_EQ(items[5].items.size(), 2);
	EXPECT_TRUE(items[5].items[1].optionalArgument);
	EXPECT_EQ(items[5].items[1].value, 1.0);
	EXPECT_EQ(items[6].kind, PatternItemKind::Flag);
	EXPECT_EQ(items[6].text, "with sound");
	EXPECT_EQ(items[6].argument, 3);
	EXPECT_EQ(items[7].kind, PatternItemKind::Choice);
	ASSERT_EQ(items[7].choices.size(), 3);
	EXPECT_EQ(items[7].choices[1].first, "evil");
	EXPECT_EQ(items[7].choices[1].second, 2.0);
	EXPECT_EQ(items[7].choices[2].first, "");
	EXPECT_EQ(items[8].kind, PatternItemKind::Fixed);
	EXPECT_EQ(items[8].value, 86400.0);

	const auto enumArg = ParsePattern("create $0:SCRIPT_OBJECT_TYPE $1 at $2");
	ASSERT_EQ(enumArg.size(), 5);
	EXPECT_EQ(enumArg[1].enumName, "SCRIPT_OBJECT_TYPE");
	EXPECT_TRUE(ParsePattern("broken {flag").empty());
}

namespace
{
void CollectArguments(const std::vector<PatternItem>& items, std::vector<int>& out)
{
	for (const auto& item : items)
	{
		if (item.argument >= 0)
		{
			out.push_back(item.argument);
		}
		CollectArguments(item.items, out);
	}
}
} // namespace

TEST(ChlLanguage, EveryFormFitsItsNative)
{
	const auto natives = DefaultNativeSignatures();
	for (const auto& form : StatementForms())
	{
		const auto items = ParsePattern(form.pattern);
		ASSERT_FALSE(items.empty()) << form.pattern;
		// A few names are shared by functions with different parameters; the form must fit one of them
		size_t params = 0;
		bool found = false;
		for (const auto& candidate : natives)
		{
			if (candidate.name == form.native)
			{
				found = true;
				params = std::max(params, candidate.params.size());
			}
		}
		ASSERT_TRUE(found) << form.native;
		std::vector<int> arguments;
		CollectArguments(items, arguments);
		for (const auto argument : arguments)
		{
			EXPECT_LT(static_cast<size_t>(argument), std::max<size_t>(params, 1)) << form.pattern;
		}
	}
	EXPECT_FALSE(FormsForNative("CREATE").empty());
	EXPECT_TRUE(FormsForNative("NOT_A_FUNCTION").empty());
}

TEST(ChlLanguage, ConstantsFromHeaders)
{
	ConstantTable constants;
	const auto added = constants.LoadHeader(R"(
		// comment
		enum SCRIPT_OBJECT_TYPE
		{
			SCRIPT_OBJECT_TYPE_NONE,
			SCRIPT_OBJECT_TYPE_MARKER, /* one */
			SCRIPT_OBJECT_TYPE_HOUSE = 0x10,
			SCRIPT_OBJECT_TYPE_NEXT,
		};
		enum OTHER { OTHER_A = -2, OTHER_B = OTHER_A + 5 };
		#define LIMIT 42
	)");
	EXPECT_EQ(added, 7);
	EXPECT_EQ(constants.ValueOf("SCRIPT_OBJECT_TYPE_HOUSE"), 16);
	EXPECT_FALSE(constants.ValueOf("HOUSE").has_value());
	EXPECT_EQ(constants.ValueOf("SCRIPT_OBJECT_TYPE_NEXT"), 17);
	EXPECT_EQ(constants.ValueOf("OTHER_B"), 3);
	EXPECT_EQ(constants.ValueOf("LIMIT"), 42);
	EXPECT_EQ(constants.NameOf("SCRIPT_OBJECT_TYPE", 1), "SCRIPT_OBJECT_TYPE_MARKER");
	EXPECT_EQ(constants.NameOf("OTHER", -2), "OTHER_A");
	EXPECT_FALSE(constants.NameOf("SCRIPT_OBJECT_TYPE", 99).has_value());
}

TEST(ChlLanguage, ConstantsFromInfoTables)
{
	ConstantTable constants;
	const auto added = constants.LoadInfo("#comment\nENUM_MAGIC_TYPE\tValue\nMAGIC_TYPE_FIRE\t3\t\t\n"
	                                      "ENUM_HELP_TEXT_NARRATOR\tValue\nHELP_TEXT_NARRATOR_HELLO\t7\r\n"
	                                      "ENUM_HELP_TEXT\tValue\nHELP_TEXT_BYE\t8\n");
	EXPECT_EQ(added, 3);
	EXPECT_EQ(constants.NameOf("MAGIC_TYPE", 3), "MAGIC_TYPE_FIRE");
	EXPECT_EQ(constants.NameOf("HELP_TEXT*", 7), "HELP_TEXT_NARRATOR_HELLO");
	EXPECT_EQ(constants.NameOf("HELP_TEXT*", 8), "HELP_TEXT_BYE");
	EXPECT_EQ(constants.ValueOf("HELP_TEXT_BYE"), 8);
}

TEST(ChlLanguage, Syntax)
{
	EXPECT_EQ(GetOperator(Op::And).spelling, "and");
	EXPECT_LT(GetOperator(Op::Not).precedence, GetOperator(Op::Eq).precedence);
	EXPECT_EQ(FindOperator("-", true), Op::Neg);
	EXPECT_EQ(FindOperator("-", false), Op::Sub);
	EXPECT_EQ(ScriptKindKeyword(ScriptKind::ChallengeHelpScript), "challenge help script");
	EXPECT_EQ(ParseScriptKind("help script"), ScriptKind::HelpScript);
	EXPECT_EQ(ParseScriptKind("quest help script"), ScriptKind::ChallengeHelpScript);
	EXPECT_EQ(ScriptKindKeyword(ScriptKind::MultiplayerScript), "multiplayer script");
	EXPECT_EQ(FormatNumber(15.0f), "15");
	EXPECT_EQ(FormatNumber(0.25f), "0.25");
	EXPECT_EQ(FormatNumber(2000000.0f), "2000000");
	EXPECT_EQ(FormatNumber(900000.0f), "900000");
	EXPECT_EQ(FormatNumber(0.00001f), "0.00001");
}
