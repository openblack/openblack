/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "LHVMViewer.h"

#include <array>

#include <imgui.h>
#include <imgui_memory_editor.h>
#include <imgui_user.h>

#include <Game.h>

using namespace openblack::gui;

const ImVec4 Disassembly_ColorBG = ImVec4(0.152f, 0.156f, 0.133f, 1.0f);
const ImVec4 Disassembly_ColorFG = ImVec4(0.972f, 0.972f, 0.949f, 1.0f);
const ImVec4 Disassembly_ColorComment = ImVec4(0.458f, 0.443f, 0.368f, 1.0f);
const ImVec4 Disassembly_ColorFuncName = ImVec4(0.650f, 0.886f, 0.180f, 1.0f);
const ImVec4 Disassembly_ColorKeyword = ImVec4(0.976f, 0.149f, 0.447f, 1.0f);
const ImVec4 Disassembly_ColorVariable = ImVec4(0.972f, 0.972f, 0.949f, 1.0f);
const ImVec4 Disassembly_ColorConstant = ImVec4(0.682f, 0.505f, 1.0f, 1.0f);

static const std::array<std::string, 464> Function_Names = {
    "NONE",
    "SET_CAMERA_POSITION",
    "SET_CAMERA_FOCUS",
    "MOVE_CAMERA_POSITION",
    "MOVE_CAMERA_FOCUS",
    "GET_CAMERA_POSITION",
    "GET_CAMERA_FOCUS",
    "SPIRIT_EJECT",
    "SPIRIT_HOME",
    "SPIRIT_POINT_POS",
    "SPIRIT_POINT_GAME_THING",
    "GAME_THING_FIELD_OF_VIEW",
    "POS_FIELD_OF_VIEW",
    "RUN_TEXT",
    "TEMP_TEXT",
    "TEXT_READ",
    "GAME_THING_CLICKED",
    "SET_SCRIPT_STATE",
    "SET_SCRIPT_STATE_POS",
    "SET_SCRIPT_FLOAT",
    "SET_SCRIPT_ULONG",
    "GET_PROPERTY",
    "SET_PROPERTY",
    "GET_POSITION",
    "SET_POSITION",
    "GET_DISTANCE",
    "CALL",
    "CREATE",
    "RANDOM",
    "DLL_GETTIME",
    "START_CAMERA_CONTROL",
    "END_CAMERA_CONTROL",
    "SET_WIDESCREEN",
    "MOVE_GAME_THING",
    "SET_FOCUS",
    "HAS_CAMERA_ARRIVED",
    "FLOCK_CREATE",
    "FLOCK_ATTACH",
    "FLOCK_DETACH",
    "FLOCK_DISBAND",
    "ID_SIZE",
    "FLOCK_MEMBER",
    "GET_HAND_POSITION",
    "PLAY_SOUND_EFFECT",
    "START_MUSIC",
    "STOP_MUSIC",
    "ATTACH_MUSIC",
    "DETACH_MUSIC",
    "OBJECT_DELETE",
    "FOCUS_FOLLOW",
    "POSITION_FOLLOW",
    "CALL_NEAR",
    "SPECIAL_EFFECT_POSITION",
    "SPECIAL_EFFECT_OBJECT",
    "DANCE_CREATE",
    "CALL_IN",
    "CHANGE_INNER_OUTER_PROPERTIES",
    "SNAPSHOT",
    "GET_ALIGNMENT",
    "SET_ALIGNMENT",
    "INFLUENCE_OBJECT",
    "INFLUENCE_POSITION",
    "GET_INFLUENCE",
    "SET_INTERFACE_INTERACTION",
    "PLAYED",
    "RANDOM_ULONG",
    "SET_GAMESPEED",
    "CALL_IN_NEAR",
    "OVERRIDE_STATE_ANIMATION",
    "CREATURE_CREATE_RELATIVE_TO_CREATURE",
    "CREATURE_LEARN_EVERYTHING",
    "CREATURE_SET_KNOWS_ACTION",
    "CREATURE_SET_AGENDA_PRIORITY",
    "CREATURE_TURN_OFF_ALL_DESIRES",
    "CREATURE_LEARN_DISTINCTION_ABOUT_ACTIVITY_OBJECT",
    "CREATURE_DO_ACTION",
    "IN_CREATURE_HAND",
    "CREATURE_SET_DESIRE_VALUE",
    "CREATURE_SET_DESIRE_ACTIVATED",
    "CREATURE_SET_DESIRE_ACTIVATED",
    "CREATURE_SET_DESIRE_MAXIMUM",
    "CONVERT_CAMERA_POSITION",
    "CONVERT_CAMERA_FOCUS",
    "CREATURE_SET_PLAYER",
    "START_COUNTDOWN_TIMER",
    "CREATURE_INITIALISE_NUM_TIMES_PERFORMED_ACTION",
    "CREATURE_GET_NUM_TIMES_ACTION_PERFORMED",
    "REMOVE_COUNTDOWN_TIMER",
    "GET_OBJECT_DROPPED",
    "CLEAR_DROPPED_BY_OBJECT",
    "CREATE_REACTION",
    "REMOVE_REACTION",
    "GET_COUNTDOWN_TIMER",
    "START_DUAL_CAMERA",
    "UPDATE_DUAL_CAMERA",
    "RELEASE_DUAL_CAMERA",
    "SET_CREATURE_HELP",
    "GET_TARGET_OBJECT",
    "CREATURE_DESIRE_IS",
    "COUNTDOWN_TIMER_EXISTS",
    "LOOK_GAME_THING",
    "GET_OBJECT_DESTINATION",
    "CREATURE_FORCE_FINISH",
    "HIDE_COUNTDOWN_TIMER",
    "GET_ACTION_TEXT_FOR_OBJECT",
    "CREATE_DUAL_CAMERA_WITH_POINT",
    "SET_CAMERA_TO_FACE_OBJECT",
    "MOVE_CAMERA_TO_FACE_OBJECT",
    "GET_MOON_PERCENTAGE",
    "POPULATE_CONTAINER",
    "ADD_REFERENCE",
    "REMOVE_REFERENCE",
    "SET_GAME_TIME",
    "GET_GAME_TIME",
    "GET_REAL_TIME",
    "GET_REAL_DAY",
    "GET_REAL_DAY",
    "GET_REAL_MONTH",
    "GET_REAL_YEAR",
    "RUN_CAMERA_PATH",
    "START_DIALOGUE",
    "END_DIALOGUE",
    "IS_SPIRIT_READY",
    "CHANGE_WEATHER_PROPERTIES",
    "CHANGE_LIGHTNING_PROPERTIES",
    "CHANGE_TIME_FADE_PROPERTIES",
    "CHANGE_CLOUD_PROPERTIES",
    "SET_HEADING_AND_SPEED",
    "START_GAME_SPEED",
    "END_GAME_SPEED",
    "BUILD_BUILDING",
    "SET_AFFECTED_BY_WIND",
    "WIDESCREEN_TRANSISTION_FINISHED",
    "GET_RESOURCE",
    "ADD_RESOURCE",
    "REMOVE_RESOURCE",
    "GET_TARGET_RELATIVE_POS",
    "STOP_POINTING",
    "STOP_LOOKING",
    "LOOK_AT_POSITION",
    "PLAY_SPIRIT_ANIM",
    "CALL_IN_NOT_NEAR",
    "SET_CAMERA_ZONE",
    "GET_OBJECT_STATE",
    "REVEAL_COUNTDOWN_TIMER",
    "SET_TIMER_TIME",
    "CREATE_TIMER",
    "GET_TIMER_TIME_REMAINING",
    "GET_TIMER_TIME_SINCE_SET",
    "MOVE_MUSIC",
    "GET_INCLUSION_DISTANCE",
    "GET_LAND_HEIGHT",
    "LOAD_MAP",
    "STOP_ALL_SCRIPTS_EXCLUDING",
    "STOP_ALL_SCRIPTS_IN_FILES_EXCLUDING",
    "STOP_SCRIPT",
    "CLEAR_CLICKED_OBJECT",
    "CLEAR_CLICKED_POSITION",
    "POSITION_CLICKED",
    "RELEASE_FROM_SCRIPT",
    "GET_OBJECT_HAND_IS_OVER",
    "ID_POISONED_SIZE",
    "IS_POISONED",
    "CALL_POISONED_IN",
    "CALL_NOT_POISONED_IN",
    "SPIRIT_PLAYED",
    "CLING_SPIRIT",
    "FLY_SPIRIT",
    "SET_ID_MOVEABLE",
    "SET_ID_PICKUPABLE",
    "IS_ON_FIRE",
    "IS_FIRE_NEAR",
    "STOP_SCRIPTS_IN_FILES",
    "SET_POISONED",
    "SET_TEMPERATURE",
    "SET_ON_FIRE",
    "SET_TARGET",
    "WALK_PATH",
    "FOCUS_AND_POSITION_FOLLOW",
    "GET_WALK_PATH_PERCENTAGE",
    "CAMERA_PROPERTIES",
    "ENABLE_DISABLE_MUSIC",
    "GET_MUSIC_OBJ_DISTANCE",
    "GET_MUSIC_ENUM_DISTANCE",
    "SET_MUSIC_PLAY_POSITION",
    "ATTACH_OBJECT_LEASH_TO_OBJECT",
    "ATTACH_OBJECT_LEASH_TO_HAND",
    "DETACH_OBJECT_LEASH",
    "SET_CREATURE_ONLY_DESIRE",
    "SET_CREATURE_ONLY_DESIRE_OFF",
    "RESTART_MUSIC",
    "MUSIC_PLAYED",
    "IS_OF_TYPE",
    "CLEAR_HIT_OBJECT",
    "GAME_THING_HIT",
    "SPELL_AT_THING",
    "SPELL_AT_POS",
    "CALL_PLAYER_CREATURE",
    "GET_SLOWEST_SPEED",
    "GET_OBJECT_HELD",
    "HELP_SYSTEM_ON",
    "SHAKE_CAMERA",
    "SET_ANIMATION_MODIFY",
    "SET_AVI_SEQUENCE",
    "PLAY GESTURE",
    "DEV_FUNCTION",
    "HAS_MOUSE_WHEEL",
    "NUM_MOUSE_BUTTONS",
    "SET_CREATURE_DEV_STAGE",
    "SET_FIXED_CAM_ROTATION",
    "SWAP_CREATURE",
    "GET_ARENA",
    "GET_FOOTBALL_PITCH",
    "STOP_ALL_GAMES",
    "ATTACH_TO_GAME",
    "DETACH_FROM_GAME",
    "DETACH_UNDEFINED_FROM_GAME",
    "SET_ONLY_FOR_SCRIPTS",
    "START_MATCH_WITH_REFEREE",
    "GAME_TEAM_SIZE",
    "GAME_TYPE",
    "GAME_SUB_TYPE",
    "IS_LEASHED",
    "SET_CREATURE_HOME",
    "GET_HIT_OBJECT",
    "GET_OBJECT_WHICH_HIT",
    "GET_NEAREST_TOWN_OF_PLAYER",
    "SPELL_AT_POINT",
    "SET_ATTACK_OWN_TOWN",
    "IS_FIGHTING",
    "SET_MAGIC_RADIUS",
    "TEMP_TEXT_WITH_NUMBER",
    "RUN_TEXT_WITH_NUMBER",
    "CREATURE_SPELL_REVERSION",
    "GET_DESIRE",
    "GET_EVENTS_PER_SECOND",
    "GET_TIME_SINCE",
    "GET_TOTAL_EVENTS",
    "UPDATE SNAPSHOT",
    "CREATE_REWARD",
    "CREATE_REWARD_IN_TOWN",
    "SET_FADE",
    "SET_FADE_IN",
    "FADE_FINISHED",
    "SET_PLAYER_MAGIC",
    "HAS_PLAYER_MAGIC",
    "SPIRIT_SPEAKS",
    "BELIEF_FOR_PLAYER",
    "GET_HELP",
    "SET_LEASH_WORKS",
    "LOAD_MY_CREATURE",
    "OBJECT_RELATIVE_BELIEF",
    "CREATE_WITH_ANGLE_AND_SCALE",
    "SET_HELP_SYSTEM",
    "SET_VIRTUAL_INFLUENCE",
    "SET_ACTIVE",
    "THING_VALID",
    "VORTEX_FADE_OUT",
    "REMOVE_REACTION_OF_TYPE",
    "CREATURE_LEARN_EVERYTHING_EXCLUDING",
    "PLAYED_PERCENTAGE",
    "OBJECT_CAST_BY_OBJECT",
    "IS_WIND_MAGIC_AT_POS",
    "CREATE_MIST",
    "SET_MIST_FADE",
    "GET_OBJECT_FADE",
    "PLAY HAND DEMO",
    "IS_PLAYING_HAND_DEMO",
    "GET_ARSE_POSITION",
    "IS_LEASHED_TO_OBJECT",
    "GET_INTERACTION_MAGNITUDE",
    "IS_CREATURE_AVAILABLE",
    "CREATE_HIGHLIGHT",
    "GET_OBJECT_HELD",
    "GET_ACTION_COUNT",
    "GET_OBJECT_LEASH_TYPE",
    "SET_FOCUS_FOLLOW",
    "SET_POSITION_FOLLOW",
    "SET_FOCUS_AND_POSITION_FOLLOW",
    "SET_CAMERA_LENS",
    "MOVE_CAMERA_LENS",
    "CREATURE_REACTION",
    "CREATURE_IN_DEV_SCRIPT",
    "STORE_CAMERA_DETAILS",
    "RESTORE_CAMERA_DETAILS",
    "START_ANGLE_SOUND",
    "SET_CAMERA_POS_FOC_LENS",
    "MOVE_CAMERA_POS_FOC_LENS",
    "GAME_TIME_ON_OFF",
    "MOVE_GAME_TIME",
    "SET_HIGH_GRAPHICS_DETAIL",
    "SET_SKELETON",
    "IS_SKELETON",
    "PLAYER_SPELL_CAST_TIME",
    "PLAYER_SPELL_LAST_CAST",
    "GET_LAST_SPELL_CAST_POS",
    "ADD_SPOT_VISUAL_TARGET_POS",
    "ADD_SPOT_VISUAL_TARGET_OBJECT",
    "SET_INDESTRUCTABLE",
    "SET_GRAPHICS_CLIPPING",
    "SPIRIT_APPEAR",
    "SPIRIT_DISAPPEAR",
    "SET_FOCUS_ON_OBJECT",
    "RELEASE_OBJECT_FOCUS",
    "IMMERSION_EXISTS",
    "SET_DRAW_LEASH",
    "SET_DRAW_HIGHLIGHT",
    "SET_OPEN_CLOSE",
    "SET_INTRO_BUILDING",
    "CREATURE_FORCE_FRIENDS",
    "MOVE_COMPUTER_PLAYER_POSITION",
    "ENABLE_DISABLE_COMPUTER_PLAYER",
    "GET_COMPUTER_PLAYER_POSITION",
    "SET_COMPUTER_PLAYER_POSITION",
    "GET_STORED_CAMERA_POSITION",
    "GET_STORED_CAMERA_FOCUS",
    "CALL_NEAR_IN_STATE",
    "SET_CREATURE_SOUND",
    "CREATURE_INTERACTING_WITH",
    "SET_SUN_DRAW",
    "OBJECT_INFO_BITS",
    "SET_HURT_BY_FIRE",
    "CONFINED_OBJECT",
    "CLEAR_CONFINED_OBJECT",
    "GET_OBJECT_FLOCK",
    "SET_PLAYER_BELIEF",
    "PLAY_JC_SPECIAL",
    "IS_PLAYING_JC_SPECIAL",
    "VORTEX_PARAMETERS",
    "LOAD_CREATURE",
    "IS_SPELL_CHARGING",
    "IS_THAT_SPELL_CHARGING",
    "OPPOSING_CREATURE",
    "FLOCK_WITHIN_LIMITS",
    "HIGHLIGHT_PROPERTIES",
    "LAST_MUSIC_LINE",
    "HAND_DEMO_TRIGGER",
    "GET_BELLY_POSITION",
    "SET_CREATURE_CREED_PROPERTIES",
    "GAME_THING_CAN_VIEW_CAMERA",
    "GAME_PLAY_SAY_SOUND_EFFECT",
    "SET_TOWN_DESIRE_BOOST",
    "IS_LOCKED_INTERACTION",
    "SET_CREATURE_NAME",
    "COMPUTER_PLAYER_READY",
    "ENABLE_DISABLE_COMPUTER_PLAYER",
    "CLEAR_ACTOR_MIND",
    "ENTER_EXIT_CITADEL",
    "START_ANGLE_SOUND",
    "THING_JC_SPECIAL",
    "MUSIC_PLAYED",
    "UPDATE SNAPSHOT_PICTURE",
    "STOP_SCRIPTS_IN_FILES_EXCLUDING",
    "CREATE_RANDOM_VILLAGER_OF_TRIBE",
    "TOGGLE_LEASH",
    "GAME_SET_MANA",
    "SET_MAGIC_PROPERTIES",
    "SET_GAME_SOUND",
    "SEX_IS_MALE",
    "GET_FIRST_HELP",
    "GET_LAST_HELP",
    "IS_ACTIVE",
    "SET_BOOKMARK_POSITION",
    "SET_SCAFFOLD_PROPERTIES",
    "SET_COMPUTER_PLAYER_PERSONALITY",
    "SET_COMPUTER_PLAYER_SUPPRESSION",
    "FORCE_COMPUTER_PLAYER_ACTION",
    "QUEUE_COMPUTER_PLAYER_ACTION",
    "GET_TOWN_WITH_ID",
    "SET_DISCIPLE",
    "RELEASE_COMPUTER_PLAYER",
    "SET_COMPUTER_PLAYER_SPEED",
    "SET_FOCUS_FOLLOW_COMPUTER_PLAYER",
    "SET_POSITION_FOLLOW_COMPUTER_PLAYER",
    "CALL_COMPUTER_PLAYER",
    "CALL_BUILDING_IN_TOWN",
    "SET_CAN_BUILD_WORSHIPSITE",
    "GET_FACING_CAMERA_POSITION",
    "SET_COMPUTER_PLAYER_ATTITUDE",
    "GET_COMPUTER_PLAYER_ATTITUDE",
    "LOAD_COMPUTER_PLAYER_PERSONALITY",
    "SAVE_COMPUTER_PLAYER_PERSONALITY",
    "SET_PLAYER_ALLY",
    "CALL_FLYING",
    "SET_OBJECT_FADE_IN",
    "IS_AFFECTED_BY_SPELL",
    "SET_MAGIC_IN_OBJECT",
    "ID_ADULT_SIZE",
    "OBJECT_CAPACITY",
    "OBJECT_ADULT_CAPACITY",
    "SET_CREATURE_AUTO_FIGHTING",
    "IS_AUTO_FIGHTING",
    "SET_CREATURE_QUEUE_FIGHT_MOVE",
    "SET_CREATURE_QUEUE_FIGHT_SPELL",
    "SET_CREATURE_QUEUE_FIGHT_STEP",
    "GET_CREATURE_FIGHT_ACTION",
    "CREATURE_FIGHT_QUEUE_HITS",
    "SQUARE_ROOT",
    "GET_PLAYER_ALLY",
    "SET_PLAYER_WIND_RESISTANCE",
    "GET_PLAYER_WIND_RESISTANCE",
    "PAUSE_UNPAUSE_CLIMATE_SYSTEM",
    "PAUSE_UNPAUSE_STORM_CREATION_IN_CLIMATE_SYSTEM",
    "GET_MANA_FOR_SPELL",
    "KILL_STORMS_IN_AREA",
    "INSIDE_TEMPLE",
    "RESTART_OBJECT",
    "SET_GAME_TIME_PROPERTIES",
    "RESET_GAME_TIME_PROPERTIES",
    "SOUND_EXISTS",
    "GET_TOWN_WORSHIP_DEATHS",
    "GAME_CLEAR_DIALOGUE",
    "GAME_CLOSE_DIALOGUE",
    "GET_HAND_STATE",
    "SET_INTERFACE_CITADEL",
    "MAP_SCRIPT_FUNCTION",
    "WITHIN_ROTATION",
    "GET_PLAYER_TOWN_TOTAL",
    "SPIRIT_SCREEN_POINT",
    "KEY_DOWN",
    "SET_FIGHT_EXIT",
    "GET_OBJECT_CLICKED",
    "GET_MANA",
    "CLEAR_PLAYER_SPELL_CHARGING",
    "STOP_SOUND_EFFECT",
    "GET_TOTEM_STATUE",
    "SET_SET_ON_FIRE",
    "SET_LAND_BALANCE",
    "SET_OBJECT_BELIEF_SCALE",
    "START_IMMERSION",
    "STOP_IMMERSION",
    "STOP_ALL_IMMERSION",
    "SET_CREATURE_IN_TEMPLE",
    "GAME_DRAW_TEXT",
    "GAME_DRAW_TEMP_TEXT",
    "FADE_ALL_DRAW_TEXT",
    "SET_DRAW_TEXT_COLOUR",
    "SET_CLIPPING_WINDOW",
    "CLEAR_CLIPPING_WINDOW",
    "SAVE_GAME_IN_SLOT",
    "SET_OBJECT_CARRYING",
    "POS_VALID_FOR_CREATURE",
    "GET_TIME_SINCE_OBJECT_ATTACKED",
    "GET_TOWN_AND_VILLAGER_HEALTH_TOTAL",
    "GAME_ADD_FOR_BUILDING",
    "ENABLE_DISABLE_ALIGNMENT_MUSIC",
    "GET_DEAD_LIVING",
    "ATTACH_SOUND_TAG",
    "DETACH_SOUND_TAG",
    "GET_SACRIFICE_TOTAL",
    "GAME_SOUND_PLAYING",
    "GET_TEMPLE_POSITION",
    "CREATURE_AUTOSCALE",
    "GET_SPELL_ICON_IN_TEMPLE",
    "GAME_CLEAR_COMPUTER_PLAYER_ACTIONS",
    "GET_FIRST_IN_CONTAINER",
    "GET_NEXT_IN_CONTAINER",
    "GET_TEMPLE_ENTRANCE_POSITION",
    "SAY_SOUND_EFFECT_PLAYING",
    "SET_HAND_DEMO_KEYS",
    "CAN_SKIP_TUTORIAL",
    "CAN_SKIP_CREATURE_TRAINING",
    "IS_KEEPING_OLD_CREATURE",
    "CURRENT_PROFILE_HAS_CREATURE",
};

LHVMViewer::LHVMViewer()
    : DebugWindow("LHVM Viewer", ImVec2(720.0f, 612.0f))
    , _selectedScriptID(1)
    , _scrollToSelected(false)
    , _resetScriptDisassemblyScroll(false)
{
}

void LHVMViewer::Draw(Game& game)
{
	auto lhvm = game.GetLhvm();
	if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("Scripts"))
		{
			DrawScriptsTab(lhvm);

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Variables"))
		{
			// left
			static size_t selected = 0;
			ImGui::BeginChild("left pane", ImVec2(240, 0), true);

			auto variables = lhvm.GetVariables();
			for (size_t i = 0; i < variables.size(); i++)
			{
				if (ImGui::Selectable(variables[i].c_str(), selected == i))
					selected = i;
			}

			ImGui::EndChild();
			ImGui::SameLine();

			// right
			ImGui::BeginChild("item view"); // Leave room for 1 line below us
			ImGui::Text("Variable ID: %s", std::to_string(selected).c_str());
			ImGui::Text("Variable Name: %s", variables[selected].c_str());
			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Data"))
		{
			static MemoryEditor lhvm_data_editor;
			auto const& data = lhvm.GetData();
			lhvm_data_editor.DrawContents((void*)data.data(), data.size(), 0);

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

void LHVMViewer::Update([[maybe_unused]] openblack::Game& game, [[maybe_unused]] const openblack::Renderer& renderer) {}

void LHVMViewer::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) {}

void LHVMViewer::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) {}

void LHVMViewer::DrawScriptsTab(const openblack::LHVM::LHVM& lhvm)
{
	static auto vector_getter = [](void* vec, int idx, const char** out_text) {
		auto& vector = *static_cast<std::vector<std::string>*>(vec);
		if (idx < 0 || idx >= static_cast<int>(vector.size()))
		{
			return false;
		}
		*out_text = vector.at(idx).c_str();
		return true;
	};

	auto scripts = lhvm.GetScripts();

	ImGui::PushItemWidth(200);

	ImGui::ListBoxHeader("##scripts", static_cast<int>(scripts.size()), 32);

	for (auto const& script : scripts)
	{
		if (ImGui::Selectable(script.GetName().c_str(), script.GetScriptID() == _selectedScriptID))
			SelectScript(script.GetScriptID());

		if (_scrollToSelected && _selectedScriptID == script.GetScriptID())
		{
			ImGui::SetScrollHereY(0.25f);
			_scrollToSelected = false;
		}
	}

	ImGui::ListBoxFooter();

	ImGui::SameLine();

	auto script = scripts[_selectedScriptID - 1];

	ImGui::BeginChild("##script");
	ImGui::Text("ID: %d", script.GetScriptID());
	ImGui::SameLine();
	ImGui::Text("Name: %s", script.GetName().c_str());
	ImGui::Text("File: %s", script.GetFileName().c_str());
	ImGui::Text("Variable offset: %d", script.GetVariableOffset());
	ImGui::SameLine();
	ImGui::Text("Parameter count: %d", script.GetParameterCount());
	if (ImGui::BeginTabBar("##ScriptTabs", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("Code"))
		{
			DrawScriptDisassembly(lhvm, script);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Local Variables"))
		{
			auto script_vars = script.GetVariables();

			static int selected_var = 0;
			ImGui::PushItemWidth(-1);
			ImGui::ListBox("", &selected_var, vector_getter, static_cast<void*>(&script_vars),
			               static_cast<int>(script_vars.size()), 21);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
	ImGui::EndChild();
}

void LHVMViewer::DrawScriptDisassembly(const openblack::LHVM::LHVM& lhvm, openblack::LHVM::VMScript& script)
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Disassembly_ColorBG);
	ImGui::PushStyleColor(ImGuiCol_Text, Disassembly_ColorFG);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, Disassembly_ColorBG);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, Disassembly_ColorComment);

	ImGui::BeginChild("##code");

	if (_resetScriptDisassemblyScroll)
	{
		ImGui::SetScrollHereY(0.0f);
		_resetScriptDisassemblyScroll = false;
	}

	auto const& code = lhvm.GetInstructions();

	for (unsigned int i = script.GetInstructionAddress(); i < code.size(); i++)
	{
		auto const& instruction = code[i];

		ImGui::TextColored(Disassembly_ColorComment, "0x%04x:", i);
		ImGui::SameLine();

		switch (instruction.GetOpcode())
		{
		case LHVM::VMInstruction::Opcode::PUSH:
			ImGui::TextColored(Disassembly_ColorKeyword, "PUSH");
			ImGui::SameLine();

			if (instruction.GetAccess() == LHVM::VMInstruction::Access::VARIABLE)
				DrawVariable(lhvm, script, instruction.GetData());
			else if (instruction.GetAccess() == LHVM::VMInstruction::Access::STACK)
				ImGui::TextColored(Disassembly_ColorConstant, "%s",
				                   DataToString(instruction.GetData(), instruction.GetDataType()).c_str());

			break;

		case LHVM::VMInstruction::Opcode::POP:
			ImGui::TextColored(Disassembly_ColorKeyword, "POP");
			if (instruction.GetAccess() == LHVM::VMInstruction::Access::VARIABLE)
			{
				ImGui::SameLine();
				DrawVariable(lhvm, script, instruction.GetData());
			}

			break;
		case LHVM::VMInstruction::Opcode::ADD:
			if (instruction.GetDataType() == LHVM::VMInstruction::DataType::INT)
				ImGui::TextColored(Disassembly_ColorKeyword, "ADDI");
			else if (instruction.GetDataType() == LHVM::VMInstruction::DataType::FLOAT)
				ImGui::TextColored(Disassembly_ColorKeyword, "ADDF");
			else if (instruction.GetDataType() == LHVM::VMInstruction::DataType::VECTOR)
				ImGui::TextColored(Disassembly_ColorKeyword, "ADDV");
			else
				ImGui::TextColored(Disassembly_ColorKeyword, "ADD");
			break;
		case LHVM::VMInstruction::Opcode::CALL:
			ImGui::TextColored(Disassembly_ColorKeyword, "CALL");
			ImGui::SameLine();
			ImGui::TextColored(Disassembly_ColorFuncName, "%s", Function_Names[instruction.GetData()].c_str());
			break;
		case LHVM::VMInstruction::Opcode::RUN:
		{
			auto const& run_script = lhvm.GetScripts().at(instruction.GetData() - 1);

			ImGui::TextColored(Disassembly_ColorKeyword, "RUN");
			ImGui::SameLine();
			if (ImGui::TextButtonColored(Disassembly_ColorFuncName, run_script.GetName().c_str()))
				SelectScript(instruction.GetData());

			ImGui::SameLine();
			ImGui::TextColored(Disassembly_ColorComment, "// expecting %d parameters", run_script.GetParameterCount());

			break;
		}

		case LHVM::VMInstruction::Opcode::JUMP:
			ImGui::TextColored(Disassembly_ColorKeyword, "JUMP");
			ImGui::SameLine();
			ImGui::TextColored(Disassembly_ColorConstant, "0x%04x", instruction.GetData());
			break;
		default:
			ImGui::TextColored(Disassembly_ColorKeyword, "%s", instruction.Disassemble().c_str());
			break;
		}

		if (instruction.GetOpcode() == LHVM::VMInstruction::Opcode::END)
			break;
	}

	ImGui::EndChild();

	ImGui::PopStyleColor(4);
}

void LHVMViewer::DrawVariable(const openblack::LHVM::LHVM& lhvm, openblack::LHVM::VMScript& script, uint32_t idx)
{
	// local variable
	if (idx > script.GetVariableOffset())
	{
		std::string variable = script.GetVariables()[idx - script.GetVariableOffset() - 1];
		ImGui::TextColored(Disassembly_ColorVariable, "local %s", variable.c_str());
		return;
	}

	// global variable
	std::string variable = lhvm.GetVariables()[idx - 1];
	ImGui::TextColored(Disassembly_ColorVariable, "global %s", variable.c_str());
}

std::string LHVMViewer::DataToString(uint32_t data, openblack::LHVM::VMInstruction::DataType type)
{
	switch (type)
	{
	case LHVM::VMInstruction::DataType::INT:
		return std::to_string(data);
	case LHVM::VMInstruction::DataType::FLOAT:
	case LHVM::VMInstruction::DataType::VECTOR:
	{
		// Strict Aliasing using the union trick
		union
		{
			uint32_t u;
			float f;
		} f2u = {data};
		return std::to_string(f2u.f) + "f";
	}
	case LHVM::VMInstruction::DataType::BOOLEAN:
		return data ? "true" : "false";
	default:
		return std::to_string(data) + " (unk type)";
	}
}

void LHVMViewer::SelectScript(uint32_t idx)
{
	_selectedScriptID = idx;
	_scrollToSelected = true;
	_resetScriptDisassemblyScroll = true;
}
