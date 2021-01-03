/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "LivingActionSystem.h"

#include <spdlog/spdlog.h>

#include "ECS/Components/LivingAction.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "Enums.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

uint32_t VillagerInvalidState(LivingAction& action)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("ai"), "Villager #{}: Stuck in an invalid state",
	                    static_cast<uint32_t>(Game::instance()->GetEntityRegistry().ToEntity(action)));
	assert(false);
	return 0;
}

uint32_t VillagerCreated(LivingAction& action)
{
	if (action.turnsUntilStateChange > 0)
	{
		--action.turnsUntilStateChange;
	}
	else
	{
		LivingActionSystem::instance().VillagerSetState(action, LivingAction::Index::Top, VillagerStates::DecideWhatToDo,
		                                                false);
	}
	return 0;
}

struct VillagerStateTableEntry
{
	std::function<uint32_t(LivingAction&)> state = nullptr;
	std::function<bool(LivingAction&, VillagerStates, VillagerStates)> entryState = nullptr;
	std::function<bool(LivingAction&)> exitState = nullptr;
	std::function<bool(LivingAction&)> saveState = nullptr;
	std::function<bool(LivingAction&)> loadState = nullptr;
	std::function<bool(LivingAction&)> field_0x50 = nullptr;
	std::function<bool(LivingAction&)> field_0x60 = nullptr;
	std::function<int(LivingAction&)> transitionAnimation = nullptr;
	std::function<bool(LivingAction&)> validate = nullptr;
};

static VillagerStateTableEntry TodoEntry = {
    .state = [](LivingAction& action) -> uint32_t {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented state function: {}",
	                       static_cast<uint32_t>(Game::instance()->GetEntityRegistry().ToEntity(action)),
	                       VillagerStateStrings[static_cast<size_t>(
	                           LivingActionSystem::instance().VillagerGetState(action, LivingAction::Index::Top))]);
	    return 0;
    },
    .entryState = [](LivingAction& action, VillagerStates src, VillagerStates dst) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented entry state function ({} -> {})",
	                       static_cast<uint32_t>(Game::instance()->GetEntityRegistry().ToEntity(action)),
	                       VillagerStateStrings[static_cast<size_t>(src)], VillagerStateStrings[static_cast<size_t>(dst)]);
	    return false;
    },
    .exitState = [](LivingAction& action) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented exit state function)",
	                       static_cast<uint32_t>(Game::instance()->GetEntityRegistry().ToEntity(action)));
	    return false;
    },
    .saveState = [](LivingAction& action) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented save state function: {}",
	                       static_cast<uint32_t>(Game::instance()->GetEntityRegistry().ToEntity(action)),
	                       VillagerStateStrings[static_cast<size_t>(
	                           LivingActionSystem::instance().VillagerGetState(action, LivingAction::Index::Top))]);
	    return false;
    },
    .loadState = [](LivingAction& action) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented load state function: {}",
	                       static_cast<uint32_t>(Game::instance()->GetEntityRegistry().ToEntity(action)),
	                       VillagerStateStrings[static_cast<size_t>(
	                           LivingActionSystem::instance().VillagerGetState(action, LivingAction::Index::Top))]);
	    return false;
    },
    .field_0x50 = [](LivingAction& action) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented field_0x50 state function: {}",
	                       static_cast<uint32_t>(Game::instance()->GetEntityRegistry().ToEntity(action)),
	                       VillagerStateStrings[static_cast<size_t>(
	                           LivingActionSystem::instance().VillagerGetState(action, LivingAction::Index::Top))]);
	    return false;
    },
    .field_0x60 = [](LivingAction& action) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented field_0x60 state function: {}",
	                       static_cast<uint32_t>(Game::instance()->GetEntityRegistry().ToEntity(action)),
	                       VillagerStateStrings[static_cast<size_t>(
	                           LivingActionSystem::instance().VillagerGetState(action, LivingAction::Index::Top))]);
	    return false;
    },
    .transitionAnimation = [](LivingAction& action) -> int {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented transition animation function: {}",
	                       static_cast<uint32_t>(Game::instance()->GetEntityRegistry().ToEntity(action)),
	                       VillagerStateStrings[static_cast<size_t>(
	                           LivingActionSystem::instance().VillagerGetState(action, LivingAction::Index::Top))]);
	    return -1;
    },
    .validate = [](LivingAction& action) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented validate function: {}",
	                       static_cast<uint32_t>(Game::instance()->GetEntityRegistry().ToEntity(action)),
	                       VillagerStateStrings[static_cast<size_t>(
	                           LivingActionSystem::instance().VillagerGetState(action, LivingAction::Index::Top))]);
	    return false;
    },
};

std::array<VillagerStateTableEntry, static_cast<size_t>(VillagerStates::_COUNT)> VillagerStateTable = {
    /* INVALID_STATE */ VillagerStateTableEntry {
        .state = &VillagerInvalidState,
    },
    /* MOVE_TO_POS */ TodoEntry,
    /* MOVE_TO_OBJECT */ TodoEntry,
    /* MOVE_ON_STRUCTURE */ TodoEntry,
    /* IN_SCRIPT */ TodoEntry,
    /* IN_DANCE */ TodoEntry,
    /* FLEEING_FROM_OBJECT_REACTION */ TodoEntry,
    /* LOOKING_AT_OBJECT_REACTION */ TodoEntry,
    /* FOLLOWING_OBJECT_REACTION */ TodoEntry,
    /* INSPECT_OBJECT_REACTION */ TodoEntry,
    /* FLYING */ TodoEntry,
    /* LANDED */ TodoEntry,
    /* LOOK_AT_FLYING_OBJECT_REACTION */ TodoEntry,
    /* SET_DYING */ TodoEntry,
    /* DYING */ TodoEntry,
    /* DEAD */ TodoEntry,
    /* DROWNING */ TodoEntry,
    /* DOWNED */ TodoEntry,
    /* BEING_EATEN */ TodoEntry,
    /* GOTO_FOOD_REACTION */ TodoEntry,
    /* ARRIVES_AT_FOOD_REACTION */ TodoEntry,
    /* GOTO_WOOD_REACTION */ TodoEntry,
    /* ARRIVES_AT_WOOD_REACTION */ TodoEntry,
    /* WAIT_FOR_ANIMATION */ TodoEntry,
    /* IN_HAND */ TodoEntry,
    /* GOTO_PICKUP_BALL_REACTION */ TodoEntry,
    /* ARRIVES_AT_PICKUP_BALL_REACTION */ TodoEntry,
    /* MOVE_IN_FLOCK */ TodoEntry,
    /* MOVE_ALONG_PATH */ TodoEntry,
    /* MOVE_ON_PATH */ TodoEntry,
    /* FLEEING_AND_LOOKING_AT_OBJECT_REACTION */ TodoEntry,
    /* GOTO_STORAGE_PIT_FOR_DROP_OFF */ TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_DROP_OFF */ TodoEntry,
    /* GOTO_STORAGE_PIT_FOR_FOOD */ TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_FOOD */ TodoEntry,
    /* ARRIVES_AT_HOME_WITH_FOOD */ TodoEntry,
    /* GO_HOME */ TodoEntry,
    /* ARRIVES_HOME */ TodoEntry,
    /* AT_HOME */ TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_BUILDING_MATERIALS */ TodoEntry,
    /* ARRIVES_AT_BUILDING_SITE */ TodoEntry,
    /* BUILDING */ TodoEntry,
    /* GOTO_STORAGE_PIT_FOR_WORSHIP_SUPPLIES */ TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_WORSHIP_SUPPLIES */ TodoEntry,
    /* GOTO_WORSHIP_SITE_WITH_SUPPLIES */ TodoEntry,
    /* MOVE_TO_WORSHIP_SITE_WITH_SUPPLIES */ TodoEntry,
    /* ARRIVES_AT_WORSHIP_SITE_WITH_SUPPLIES */ TodoEntry,
    /* FORESTER_MOVE_TO_FOREST */ TodoEntry,
    /* FORESTER_GOTO_FOREST */ TodoEntry,
    /* FORESTER_ARRIVES_AT_FOREST */ TodoEntry,
    /* FORESTER_CHOPS_TREE */ TodoEntry,
    /* FORESTER_CHOPS_TREE_FOR_BUILDING */ TodoEntry,
    /* FORESTER_FINISHED_FORESTERING */ TodoEntry,
    /* ARRIVES_AT_BIG_FOREST */ TodoEntry,
    /* ARRIVES_AT_BIG_FOREST_FOR_BUILDING */ TodoEntry,
    /* FISHERMAN_ARRIVES_AT_FISHING */ TodoEntry,
    /* FISHING */ TodoEntry,
    /* WAIT_FOR_COUNTER */ TodoEntry,
    /* GOTO_WORSHIP_SITE_FOR_WORSHIP */ TodoEntry,
    /* ARRIVES_AT_WORSHIP_SITE_FOR_WORSHIP */ TodoEntry,
    /* WORSHIPPING_AT_WORSHIP_SITE */ TodoEntry,
    /* GOTO_ALTAR_FOR_REST */ TodoEntry,
    /* ARRIVES_AT_ALTAR_FOR_REST */ TodoEntry,
    /* AT_ALTAR_REST */ TodoEntry,
    /* AT_ALTAR_FINISHED_REST */ TodoEntry,
    /* RESTART_WORSHIPPING_AT_WORSHIP_SITE */ TodoEntry,
    /* RESTART_WORSHIPPING_CREATURE */ TodoEntry,
    /* FARMER_ARRIVES_AT_FARM */ TodoEntry,
    /* FARMER_PLANTS_CROP */ TodoEntry,
    /* FARMER_DIGS_UP_CROP */ TodoEntry,
    /* MOVE_TO_FOOTBALL_PITCH_CONSTRUCTION */ TodoEntry,
    /* FOOTBALL_WALK_TO_POSITION */ TodoEntry,
    /* FOOTBALL_WAIT_FOR_KICK_OFF */ TodoEntry,
    /* FOOTBALL_ATTACKER */ TodoEntry,
    /* FOOTBALL_GOALIE */ TodoEntry,
    /* FOOTBALL_DEFENDER */ TodoEntry,
    /* FOOTBALL_WON_GOAL */ TodoEntry,
    /* FOOTBALL_LOST_GOAL */ TodoEntry,
    /* START_MOVE_TO_PICK_UP_BALL_FOR_DEAD_BALL */ TodoEntry,
    /* ARRIVED_AT_PICK_UP_BALL_FOR_DEAD_BALL */ TodoEntry,
    /* ARRIVED_AT_PUT_DOWN_BALL_FOR_DEAD_BALL_START */ TodoEntry,
    /* ARRIVED_AT_PUT_DOWN_BALL_FOR_DEAD_BALL_END */ TodoEntry,
    /* FOOTBALL_MATCH_PAUSED */ TodoEntry,
    /* FOOTBALL_WATCH_MATCH */ TodoEntry,
    /* FOOTBALL_MEXICAN_WAVE */ TodoEntry,
    /* CREATED */
    VillagerStateTableEntry {
        .state = &VillagerCreated,
        .field_0x50 = TodoEntry.field_0x50,
    },
    /* ARRIVES_IN_ABODE_TO_TRADE */ TodoEntry,
    /* ARRIVES_IN_ABODE_TO_PICK_UP_EXCESS */ TodoEntry,
    /* MAKE_SCARED_STIFF */ TodoEntry,
    /* SCARED_STIFF */ TodoEntry,
    /* WORSHIPPING_CREATURE */ TodoEntry,
    /* SHEPHERD_LOOK_FOR_FLOCK */ TodoEntry,
    /* SHEPHERD_MOVE_FLOCK_TO_WATER */ TodoEntry,
    /* SHEPHERD_MOVE_FLOCK_TO_FOOD */ TodoEntry,
    /* SHEPHERD_MOVE_FLOCK_BACK */ TodoEntry,
    /* SHEPHERD_DECIDE_WHAT_TO_DO_WITH_FLOCK */ TodoEntry,
    /* SHEPHERD_WAIT_FOR_FLOCK */ TodoEntry,
    /* SHEPHERD_SLAUGHTER_ANIMAL */ TodoEntry,
    /* SHEPHERD_FETCH_STRAY */ TodoEntry,
    /* SHEPHERD_GOTO_FLOCK */ TodoEntry,
    /* HOUSEWIFE_AT_HOME */ TodoEntry,
    /* HOUSEWIFE_GOTO_STORAGE_PIT */ TodoEntry,
    /* HOUSEWIFE_ARRIVES_AT_STORAGE_PIT */ TodoEntry,
    /* HOUSEWIFE_PICKUP_FROM_STORAGE_PIT */ TodoEntry,
    /* HOUSEWIFE_RETURN_HOME_WITH_FOOD */ TodoEntry,
    /* HOUSEWIFE_MAKE_DINNER */ TodoEntry,
    /* HOUSEWIFE_SERVES_DINNER */ TodoEntry,
    /* HOUSEWIFE_CLEARS_AWAY_DINNER */ TodoEntry,
    /* HOUSEWIFE_DOES_HOUSEWORK */ TodoEntry,
    /* HOUSEWIFE_GOSSIPS_AROUND_STORAGE_PIT */ TodoEntry,
    /* HOUSEWIFE_STARTS_GIVING_BIRTH */ TodoEntry,
    /* HOUSEWIFE_GIVING_BIRTH */ TodoEntry,
    /* HOUSEWIFE_GIVEN_BIRTH */ TodoEntry,
    /* CHILD_AT_CRECHE */ TodoEntry,
    /* CHILD_FOLLOWS_MOTHER */ TodoEntry,
    /* CHILD_BECOMES_ADULT */ TodoEntry,
    /* SITS_DOWN_TO_DINNER */ TodoEntry,
    /* EAT_FOOD */ TodoEntry,
    /* EAT_FOOD_AT_HOME */ TodoEntry,
    /* GOTO_BED_AT_HOME */ TodoEntry,
    /* SLEEPING_AT_HOME */ TodoEntry,
    /* WAKE_UP_AT_HOME */ TodoEntry,
    /* START_HAVING_SEX */ TodoEntry,
    /* HAVING_SEX */ TodoEntry,
    /* STOP_HAVING_SEX */ TodoEntry,
    /* START_HAVING_SEX_AT_HOME */ TodoEntry,
    /* HAVING_SEX_AT_HOME */ TodoEntry,
    /* STOP_HAVING_SEX_AT_HOME */ TodoEntry,
    /* WAIT_FOR_DINNER */ TodoEntry,
    /* HOMELESS_START */ TodoEntry,
    /* VAGRANT_START */ TodoEntry,
    /* MORN_DEATH */ TodoEntry,
    /* PERFORM_INSPECTION_REACTION */ TodoEntry,
    /* APPROACH_OBJECT_REACTION */ TodoEntry,
    /* INITIALISE_TELL_OTHERS_ABOUT_OBJECT */ TodoEntry,
    /* TELL_OTHERS_ABOUT_INTERESTING_OBJECT */ TodoEntry,
    /* APPROACH_VILLAGER_TO_TALK_TO */ TodoEntry,
    /* TELL_PARTICULAR_VILLAGER_ABOUT_OBJECT */ TodoEntry,
    /* INITIALISE_LOOK_AROUND_FOR_VILLAGER_TO_TELL */ TodoEntry,
    /* LOOK_AROUND_FOR_VILLAGER_TO_TELL */ TodoEntry,
    /* MOVE_TOWARDS_OBJECT_TO_LOOK_AT */ TodoEntry,
    /* INITIALISE_IMPRESSED_REACTION */ TodoEntry,
    /* PERFORM_IMPRESSED_REACTION */ TodoEntry,
    /* INITIALISE_FIGHT_REACTION */ TodoEntry,
    /* PERFORM_FIGHT_REACTION */ TodoEntry,
    /* HOMELESS_EAT_DINNER */ TodoEntry,
    /* INSPECT_CREATURE_REACTION */ TodoEntry,
    /* PERFORM_INSPECT_CREATURE_REACTION */ TodoEntry,
    /* APPROACH_CREATURE_REACTION */ TodoEntry,
    /* INITIALISE_BEWILDERED_BY_MAGIC_TREE_REACTION */ TodoEntry,
    /* PERFORM_BEWILDERED_BY_MAGIC_TREE_REACTION */ TodoEntry,
    /* TURN_TO_FACE_MAGIC_TREE */ TodoEntry,
    /* LOOK_AT_MAGIC_TREE */ TodoEntry,
    /* DANCE_FOR_EDITING_PURPOSES */ TodoEntry,
    /* MOVE_TO_DANCE_POS */ TodoEntry,
    /* INITIALISE_RESPECT_CREATURE_REACTION */ TodoEntry,
    /* PERFORM_RESPECT_CREATURE_REACTION */ TodoEntry,
    /* FINISH_RESPECT_CREATURE_REACTION */ TodoEntry,
    /* APPROACH_HAND_REACTION */ TodoEntry,
    /* FLEEING_FROM_CREATURE_REACTION */ TodoEntry,
    /* TURN_TO_FACE_CREATURE_REACTION */ TodoEntry,
    /* WATCH_FLYING_OBJECT_REACTION */ TodoEntry,
    /* POINT_AT_FLYING_OBJECT_REACTION */ TodoEntry,
    /* DECIDE_WHAT_TO_DO */ TodoEntry,
    /* INTERACT_DECIDE_WHAT_TO_DO */ TodoEntry,
    /* EAT_OUTSIDE */ TodoEntry,
    /* RUN_AWAY_FROM_OBJECT_REACTION */ TodoEntry,
    /* MOVE_TOWARDS_CREATURE_REACTION */ TodoEntry,
    /* AMAZED_BY_MAGIC_SHIELD_REACTION */ TodoEntry,
    /* VILLAGER_GOSSIPS */ TodoEntry,
    /* CHECK_INTERACT_WITH_ANIMAL */ TodoEntry,
    /* CHECK_INTERACT_WITH_WORSHIP_SITE */ TodoEntry,
    /* CHECK_INTERACT_WITH_ABODE */ TodoEntry,
    /* CHECK_INTERACT_WITH_FIELD */ TodoEntry,
    /* CHECK_INTERACT_WITH_FISH_FARM */ TodoEntry,
    /* CHECK_INTERACT_WITH_TREE */ TodoEntry,
    /* CHECK_INTERACT_WITH_BALL */ TodoEntry,
    /* CHECK_INTERACT_WITH_POT */ TodoEntry,
    /* CHECK_INTERACT_WITH_FOOTBALL */ TodoEntry,
    /* CHECK_INTERACT_WITH_VILLAGER */ TodoEntry,
    /* CHECK_INTERACT_WITH_MAGIC_LIVING */ TodoEntry,
    /* CHECK_INTERACT_WITH_ROCK */ TodoEntry,
    /* ARRIVES_AT_ROCK_FOR_WOOD */ TodoEntry,
    /* GOT_WOOD_FROM_ROCK */ TodoEntry,
    /* REENTER_BUILDING_STATE */ TodoEntry,
    /* ARRIVE_AT_PUSH_OBJECT */ TodoEntry,
    /* TAKE_WOOD_FROM_TREE */ TodoEntry,
    /* TAKE_WOOD_FROM_POT */ TodoEntry,
    /* TAKE_WOOD_FROM_TREE_FOR_BUILDING */ TodoEntry,
    /* TAKE_WOOD_FROM_POT_FOR_BUILDING */ TodoEntry,
    /* SHEPHERD_TAKE_ANIMAL_FOR_SLAUGHTER */ TodoEntry,
    /* SHEPHERD_TAKES_CONTROL_OF_FLOCK */ TodoEntry,
    /* SHEPHERD_RELEASES_CONTROL_OF_FLOCK */ TodoEntry,
    /* DANCE_BUT_NOT_WORSHIP */ TodoEntry,
    /* FAINTING_REACTION */ TodoEntry,
    /* START_CONFUSED_REACTION */ TodoEntry,
    /* CONFUSED_REACTION */ TodoEntry,
    /* AFTER_TAP_ON_ABODE */ TodoEntry,
    /* WEAK_ON_GROUND */ TodoEntry,
    /* SCRIPT_WANDER_AROUND_POSITION */ TodoEntry,
    /* SCRIPT_PLAY_ANIM */ TodoEntry,
    /* GO_TOWARDS_TELEPORT_REACTION */ TodoEntry,
    /* TELEPORT_REACTION */ TodoEntry,
    /* DANCE_WHILE_REACTING */ TodoEntry,
    /* CONTROLLED_BY_CREATURE */ TodoEntry,
    /* POINT_AT_DEAD_PERSON */ TodoEntry,
    /* GO_TOWARDS_DEAD_PERSON */ TodoEntry,
    /* LOOK_AT_DEAD_PERSON */ TodoEntry,
    /* MOURN_DEAD_PERSON */ TodoEntry,
    /* NOTHING_TO_DO */ TodoEntry,
    /* ARRIVES_AT_WORKSHOP_FOR_DROP_OFF */ TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_WORKSHOP_MATERIALS */ TodoEntry,
    /* SHOW_POISONED */ TodoEntry,
    /* HIDING_AT_WORSHIP_SITE */ TodoEntry,
    /* CROWD_REACTION */ TodoEntry,
    /* REACT_TO_FIRE */ TodoEntry,
    /* PUT_OUT_FIRE_BY_BEATING */ TodoEntry,
    /* PUT_OUT_FIRE_WITH_WATER */ TodoEntry,
    /* GET_WATER_TO_PUT_OUT_FIRE */ TodoEntry,
    /* ON_FIRE */ TodoEntry,
    /* MOVE_AROUND_FIRE */ TodoEntry,
    /* DISCIPLE_NOTHING_TO_DO */ TodoEntry,
    /* FOOTBALL_MOVE_TO_BALL */ TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_TRADER_PICK_UP */ TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_TRADER_DROP_OFF */ TodoEntry,
    /* BREEDER_DISCIPLE */ TodoEntry,
    /* MISSIONARY_DISCIPLE */ TodoEntry,
    /* REACT_TO_BREEDER */ TodoEntry,
    /* SHEPHERD_CHECK_ANIMAL_FOR_SLAUGHTER */ TodoEntry,
    /* INTERACT_DECIDE_WHAT_TO_DO_FOR_OTHER_VILLAGER */ TodoEntry,
    /* ARTIFACT_DANCE */ TodoEntry,
    /* FLEEING_FROM_PREDATOR_REACTION */ TodoEntry,
    /* WAIT_FOR_WOOD */ TodoEntry,
    /* INSPECT_OBJECT */ TodoEntry,
    /* GO_HOME_AND_CHANGE */ TodoEntry,
    /* WAIT_FOR_MATE */ TodoEntry,
    /* GO_AND_HIDE_IN_NEARBY_BUILDING */ TodoEntry,
    /* LOOK_TO_SEE_IF_IT_IS_SAFE */ TodoEntry,
    /* SLEEP_IN_TENT */ TodoEntry,
    /* PAUSE_FOR_A_SECOND */ TodoEntry,
    /* PANIC_REACTION */ TodoEntry,
    /* GET_FOOD_AT_WORSHIP_SITE */ TodoEntry,
    /* GOTO_CONGREGATE_IN_TOWN_AFTER_EMERGENCY */ TodoEntry,
    /* CONGREGATE_IN_TOWN_AFTER_EMERGENCY */ TodoEntry,
    /* SCRIPT_IN_CROWD */ TodoEntry,
    /* GO_AND_CHILLOUT_OUTSIDE_HOME */ TodoEntry,
    /* SIT_AND_CHILLOUT */ TodoEntry,
    /* SCRIPT_GO_AND_MOVE_ALONG_PATH */ TodoEntry,
    /* RESTART_MEETING */ TodoEntry,
    /* GOTO_ABODE_BURNING_REACTION */ TodoEntry,
    /* ARRIVES_AT_ABODE_BURNING_REACTION */ TodoEntry,
    /* REPAIRS_ABODE */ TodoEntry,
    /* ARRIVES_AT_SCAFFOLD_FOR_PICKUP */ TodoEntry,
    /* ARRIVES_AT_BUILDING_SITE_WITH_SCAFFOLD */ TodoEntry,
    /* MOVE_SCAFFOLD_TO_BUILDING_SITE */ TodoEntry,
};

LivingActionSystem& LivingActionSystem::instance()
{
	static LivingActionSystem* instance = nullptr;
	if (instance == nullptr)
	{
		instance = new LivingActionSystem();
	}
	return *instance;
}

LivingActionSystem::LivingActionSystem() = default;

void LivingActionSystem::Update()
{
	auto& registry = Game::instance()->GetEntityRegistry();

	registry.Each<LivingAction>([](LivingAction& action) { ++action.turnsSinceStateChange; });

	// TODO: process food speedup

	registry.Each<const Villager, LivingAction>([this]([[maybe_unused]] const Villager& villager, LivingAction& action) {
		VillagerCallValidate(action, LivingAction::Index::Top);
	});
	// TODO: same call but for other types of living

	registry.Each<const Villager, LivingAction>([this]([[maybe_unused]] const Villager& villager, LivingAction& action) {
		VillagerCallValidate(action, LivingAction::Index::Final);
	});
	// TODO: same call but for other types of living

	// TODO(bwrsandman): Store result of this call in vector or with tag component
	registry.Each<const Villager, LivingAction>([this]([[maybe_unused]] const Villager& villager, LivingAction& action) {
		VillagerCallState(action, LivingAction::Index::Top);
	});
	// TODO: same call but for other types of living
}

VillagerStates LivingActionSystem::VillagerGetState(const LivingAction& action, LivingAction::Index index) const
{
	assert(index < LivingAction::Index::_Count);
	return static_cast<VillagerStates>(action.states[static_cast<size_t>(index)]);
}

void LivingActionSystem::VillagerSetState(LivingAction& action, LivingAction::Index index, VillagerStates state,
                                          bool skipTransition) const
{
	assert(index < LivingAction::Index::_Count);
	if (static_cast<VillagerStates>(action.states[static_cast<size_t>(index)]) != state)
	{
		[[maybe_unused]] auto& registry = Game::instance()->GetEntityRegistry();
		SPDLOG_LOGGER_TRACE(spdlog::get("ai"), "Villager #{}: Setting state {} -> {}",
		                    static_cast<int>(registry.ToEntity(action)),
		                    VillagerStateStrings[static_cast<size_t>(action.states[static_cast<size_t>(index)])],
		                    VillagerStateStrings[static_cast<size_t>(state)]);

		const auto previousState = static_cast<VillagerStates>(action.states[static_cast<size_t>(index)]);
		if (index == LivingAction::Index::Top)
		{
			action.turnsSinceStateChange = 0;
			if (skipTransition)
			{
				return;
			}

			if (VillagerCallExitState(action, index))
			{
				return;
			}

			VillagerCallEntryState(action, index, previousState, state);
		}
	}
}

uint32_t LivingActionSystem::VillagerCallState(LivingAction& action, LivingAction::Index index) const
{
	const auto& state = action.states[static_cast<size_t>(index)];
	const auto& entry = VillagerStateTable[static_cast<size_t>(state)];
	const auto& callback = entry.state;
	if (!callback)
	{
		return 0;
	}
	return callback(action);
}

bool LivingActionSystem::VillagerCallEntryState(LivingAction& action, LivingAction::Index index, VillagerStates src,
                                                VillagerStates dst) const
{
	const auto& state = action.states[static_cast<size_t>(index)];
	const auto& entry = VillagerStateTable[static_cast<size_t>(state)];
	const auto& callback = entry.entryState;
	if (!callback)
	{
		return false;
	}
	return callback(action, src, dst);
}

bool LivingActionSystem::VillagerCallExitState(LivingAction& action, LivingAction::Index index) const
{
	const auto& state = action.states[static_cast<size_t>(index)];
	const auto& entry = VillagerStateTable[static_cast<size_t>(state)];
	const auto& callback = entry.exitState;
	if (!callback)
	{
		return false;
	}
	return callback(action);
}

int LivingActionSystem::VillagerCallOutOfAnimation(LivingAction& action, LivingAction::Index index) const
{
	const auto& state = action.states[static_cast<size_t>(index)];
	const auto& entry = VillagerStateTable[static_cast<size_t>(state)];
	const auto& callback = entry.transitionAnimation;
	if (!callback)
	{
		return false;
	}
	return callback(action);
}

bool LivingActionSystem::VillagerCallValidate(LivingAction& action, LivingAction::Index index) const
{
	const auto& state = action.states[static_cast<size_t>(index)];
	const auto& entry = VillagerStateTable[static_cast<size_t>(state)];
	const auto& callback = entry.validate;
	if (!callback)
	{
		return false;
	}
	return callback(action);
}
