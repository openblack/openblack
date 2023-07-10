/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "LivingActionSystem.h"

#include <spdlog/spdlog.h>

#include "ECS/Components/LivingAction.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "Enums.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

uint32_t VillagerInvalidState(LivingAction& action)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("ai"), "Villager #{}: Stuck in an invalid state",
	                    static_cast<uint32_t>(Locator::entitiesRegistry::value().ToEntity(action)));
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
		Locator::livingActionSystem::value().VillagerSetState(action, LivingAction::Index::Top, VillagerStates::DecideWhatToDo,
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
	std::function<bool(LivingAction&)> field0x50 = nullptr;
	std::function<bool(LivingAction&)> field0x60 = nullptr;
	std::function<int(LivingAction&)> transitionAnimation = nullptr;
	std::function<bool(LivingAction&)> validate = nullptr;
};

static const VillagerStateTableEntry k_TodoEntry = {
    .state = [](LivingAction& action) -> uint32_t {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented state function: {}",
	                       static_cast<uint32_t>(Locator::entitiesRegistry::value().ToEntity(action)),
	                       k_VillagerStateStrings.at(static_cast<size_t>(
	                           Locator::livingActionSystem::value().VillagerGetState(action, LivingAction::Index::Top))));
	    return 0;
    },
    .entryState = [](LivingAction& action, VillagerStates src, VillagerStates dst) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented entry state function ({} -> {})",
	                       static_cast<uint32_t>(Locator::entitiesRegistry::value().ToEntity(action)),
	                       k_VillagerStateStrings.at(static_cast<size_t>(src)),
	                       k_VillagerStateStrings.at(static_cast<size_t>(dst)));
	    return false;
    },
    .exitState = [](LivingAction& action) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented exit state function)",
	                       static_cast<uint32_t>(Locator::entitiesRegistry::value().ToEntity(action)));
	    return false;
    },
    .saveState = [](LivingAction& action) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented save state function: {}",
	                       static_cast<uint32_t>(Locator::entitiesRegistry::value().ToEntity(action)),
	                       k_VillagerStateStrings.at(static_cast<size_t>(
	                           Locator::livingActionSystem::value().VillagerGetState(action, LivingAction::Index::Top))));
	    return false;
    },
    .loadState = [](LivingAction& action) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented load state function: {}",
	                       static_cast<uint32_t>(Locator::entitiesRegistry::value().ToEntity(action)),
	                       k_VillagerStateStrings.at(static_cast<size_t>(
	                           Locator::livingActionSystem::value().VillagerGetState(action, LivingAction::Index::Top))));
	    return false;
    },
    .field0x50 = [](LivingAction& action) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented field0x50 state function: {}",
	                       static_cast<uint32_t>(Locator::entitiesRegistry::value().ToEntity(action)),
	                       k_VillagerStateStrings.at(static_cast<size_t>(
	                           Locator::livingActionSystem::value().VillagerGetState(action, LivingAction::Index::Top))));
	    return false;
    },
    .field0x60 = [](LivingAction& action) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented field0x60 state function: {}",
	                       static_cast<uint32_t>(Locator::entitiesRegistry::value().ToEntity(action)),
	                       k_VillagerStateStrings.at(static_cast<size_t>(
	                           Locator::livingActionSystem::value().VillagerGetState(action, LivingAction::Index::Top))));
	    return false;
    },
    .transitionAnimation = [](LivingAction& action) -> int {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented transition animation function: {}",
	                       static_cast<uint32_t>(Locator::entitiesRegistry::value().ToEntity(action)),
	                       k_VillagerStateStrings.at(static_cast<size_t>(
	                           Locator::livingActionSystem::value().VillagerGetState(action, LivingAction::Index::Top))));
	    return -1;
    },
    .validate = [](LivingAction& action) -> bool {
	    SPDLOG_LOGGER_WARN(spdlog::get("ai"), "Villager #{}: TODO: Unimplemented validate function: {}",
	                       static_cast<uint32_t>(Locator::entitiesRegistry::value().ToEntity(action)),
	                       k_VillagerStateStrings.at(static_cast<size_t>(
	                           Locator::livingActionSystem::value().VillagerGetState(action, LivingAction::Index::Top))));
	    return false;
    },
};

const static std::array<VillagerStateTableEntry, static_cast<size_t>(VillagerStates::_COUNT)> k_VillagerStateTable = {
    /* INVALID_STATE */ VillagerStateTableEntry {
        .state = &VillagerInvalidState,
    },
    /* MOVE_TO_POS */ k_TodoEntry,
    /* MOVE_TO_OBJECT */ k_TodoEntry,
    /* MOVE_ON_STRUCTURE */ k_TodoEntry,
    /* IN_SCRIPT */ k_TodoEntry,
    /* IN_DANCE */ k_TodoEntry,
    /* FLEEING_FROM_OBJECT_REACTION */ k_TodoEntry,
    /* LOOKING_AT_OBJECT_REACTION */ k_TodoEntry,
    /* FOLLOWING_OBJECT_REACTION */ k_TodoEntry,
    /* INSPECT_OBJECT_REACTION */ k_TodoEntry,
    /* FLYING */ k_TodoEntry,
    /* LANDED */ k_TodoEntry,
    /* LOOK_AT_FLYING_OBJECT_REACTION */ k_TodoEntry,
    /* SET_DYING */ k_TodoEntry,
    /* DYING */ k_TodoEntry,
    /* DEAD */ k_TodoEntry,
    /* DROWNING */ k_TodoEntry,
    /* DOWNED */ k_TodoEntry,
    /* BEING_EATEN */ k_TodoEntry,
    /* GOTO_FOOD_REACTION */ k_TodoEntry,
    /* ARRIVES_AT_FOOD_REACTION */ k_TodoEntry,
    /* GOTO_WOOD_REACTION */ k_TodoEntry,
    /* ARRIVES_AT_WOOD_REACTION */ k_TodoEntry,
    /* WAIT_FOR_ANIMATION */ k_TodoEntry,
    /* IN_HAND */ k_TodoEntry,
    /* GOTO_PICKUP_BALL_REACTION */ k_TodoEntry,
    /* ARRIVES_AT_PICKUP_BALL_REACTION */ k_TodoEntry,
    /* MOVE_IN_FLOCK */ k_TodoEntry,
    /* MOVE_ALONG_PATH */ k_TodoEntry,
    /* MOVE_ON_PATH */ k_TodoEntry,
    /* FLEEING_AND_LOOKING_AT_OBJECT_REACTION */ k_TodoEntry,
    /* GOTO_STORAGE_PIT_FOR_DROP_OFF */ k_TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_DROP_OFF */ k_TodoEntry,
    /* GOTO_STORAGE_PIT_FOR_FOOD */ k_TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_FOOD */ k_TodoEntry,
    /* ARRIVES_AT_HOME_WITH_FOOD */ k_TodoEntry,
    /* GO_HOME */ k_TodoEntry,
    /* ARRIVES_HOME */ k_TodoEntry,
    /* AT_HOME */ k_TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_BUILDING_MATERIALS */ k_TodoEntry,
    /* ARRIVES_AT_BUILDING_SITE */ k_TodoEntry,
    /* BUILDING */ k_TodoEntry,
    /* GOTO_STORAGE_PIT_FOR_WORSHIP_SUPPLIES */ k_TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_WORSHIP_SUPPLIES */ k_TodoEntry,
    /* GOTO_WORSHIP_SITE_WITH_SUPPLIES */ k_TodoEntry,
    /* MOVE_TO_WORSHIP_SITE_WITH_SUPPLIES */ k_TodoEntry,
    /* ARRIVES_AT_WORSHIP_SITE_WITH_SUPPLIES */ k_TodoEntry,
    /* FORESTER_MOVE_TO_FOREST */ k_TodoEntry,
    /* FORESTER_GOTO_FOREST */ k_TodoEntry,
    /* FORESTER_ARRIVES_AT_FOREST */ k_TodoEntry,
    /* FORESTER_CHOPS_TREE */ k_TodoEntry,
    /* FORESTER_CHOPS_TREE_FOR_BUILDING */ k_TodoEntry,
    /* FORESTER_FINISHED_FORESTERING */ k_TodoEntry,
    /* ARRIVES_AT_BIG_FOREST */ k_TodoEntry,
    /* ARRIVES_AT_BIG_FOREST_FOR_BUILDING */ k_TodoEntry,
    /* FISHERMAN_ARRIVES_AT_FISHING */ k_TodoEntry,
    /* FISHING */ k_TodoEntry,
    /* WAIT_FOR_COUNTER */ k_TodoEntry,
    /* GOTO_WORSHIP_SITE_FOR_WORSHIP */ k_TodoEntry,
    /* ARRIVES_AT_WORSHIP_SITE_FOR_WORSHIP */ k_TodoEntry,
    /* WORSHIPPING_AT_WORSHIP_SITE */ k_TodoEntry,
    /* GOTO_ALTAR_FOR_REST */ k_TodoEntry,
    /* ARRIVES_AT_ALTAR_FOR_REST */ k_TodoEntry,
    /* AT_ALTAR_REST */ k_TodoEntry,
    /* AT_ALTAR_FINISHED_REST */ k_TodoEntry,
    /* RESTART_WORSHIPPING_AT_WORSHIP_SITE */ k_TodoEntry,
    /* RESTART_WORSHIPPING_CREATURE */ k_TodoEntry,
    /* FARMER_ARRIVES_AT_FARM */ k_TodoEntry,
    /* FARMER_PLANTS_CROP */ k_TodoEntry,
    /* FARMER_DIGS_UP_CROP */ k_TodoEntry,
    /* MOVE_TO_FOOTBALL_PITCH_CONSTRUCTION */ k_TodoEntry,
    /* FOOTBALL_WALK_TO_POSITION */ k_TodoEntry,
    /* FOOTBALL_WAIT_FOR_KICK_OFF */ k_TodoEntry,
    /* FOOTBALL_ATTACKER */ k_TodoEntry,
    /* FOOTBALL_GOALIE */ k_TodoEntry,
    /* FOOTBALL_DEFENDER */ k_TodoEntry,
    /* FOOTBALL_WON_GOAL */ k_TodoEntry,
    /* FOOTBALL_LOST_GOAL */ k_TodoEntry,
    /* START_MOVE_TO_PICK_UP_BALL_FOR_DEAD_BALL */ k_TodoEntry,
    /* ARRIVED_AT_PICK_UP_BALL_FOR_DEAD_BALL */ k_TodoEntry,
    /* ARRIVED_AT_PUT_DOWN_BALL_FOR_DEAD_BALL_START */ k_TodoEntry,
    /* ARRIVED_AT_PUT_DOWN_BALL_FOR_DEAD_BALL_END */ k_TodoEntry,
    /* FOOTBALL_MATCH_PAUSED */ k_TodoEntry,
    /* FOOTBALL_WATCH_MATCH */ k_TodoEntry,
    /* FOOTBALL_MEXICAN_WAVE */ k_TodoEntry,
    /* CREATED */
    VillagerStateTableEntry {
        .state = &VillagerCreated,
        .field0x50 = k_TodoEntry.field0x50,
    },
    /* ARRIVES_IN_ABODE_TO_TRADE */ k_TodoEntry,
    /* ARRIVES_IN_ABODE_TO_PICK_UP_EXCESS */ k_TodoEntry,
    /* MAKE_SCARED_STIFF */ k_TodoEntry,
    /* SCARED_STIFF */ k_TodoEntry,
    /* WORSHIPPING_CREATURE */ k_TodoEntry,
    /* SHEPHERD_LOOK_FOR_FLOCK */ k_TodoEntry,
    /* SHEPHERD_MOVE_FLOCK_TO_WATER */ k_TodoEntry,
    /* SHEPHERD_MOVE_FLOCK_TO_FOOD */ k_TodoEntry,
    /* SHEPHERD_MOVE_FLOCK_BACK */ k_TodoEntry,
    /* SHEPHERD_DECIDE_WHAT_TO_DO_WITH_FLOCK */ k_TodoEntry,
    /* SHEPHERD_WAIT_FOR_FLOCK */ k_TodoEntry,
    /* SHEPHERD_SLAUGHTER_ANIMAL */ k_TodoEntry,
    /* SHEPHERD_FETCH_STRAY */ k_TodoEntry,
    /* SHEPHERD_GOTO_FLOCK */ k_TodoEntry,
    /* HOUSEWIFE_AT_HOME */ k_TodoEntry,
    /* HOUSEWIFE_GOTO_STORAGE_PIT */ k_TodoEntry,
    /* HOUSEWIFE_ARRIVES_AT_STORAGE_PIT */ k_TodoEntry,
    /* HOUSEWIFE_PICKUP_FROM_STORAGE_PIT */ k_TodoEntry,
    /* HOUSEWIFE_RETURN_HOME_WITH_FOOD */ k_TodoEntry,
    /* HOUSEWIFE_MAKE_DINNER */ k_TodoEntry,
    /* HOUSEWIFE_SERVES_DINNER */ k_TodoEntry,
    /* HOUSEWIFE_CLEARS_AWAY_DINNER */ k_TodoEntry,
    /* HOUSEWIFE_DOES_HOUSEWORK */ k_TodoEntry,
    /* HOUSEWIFE_GOSSIPS_AROUND_STORAGE_PIT */ k_TodoEntry,
    /* HOUSEWIFE_STARTS_GIVING_BIRTH */ k_TodoEntry,
    /* HOUSEWIFE_GIVING_BIRTH */ k_TodoEntry,
    /* HOUSEWIFE_GIVEN_BIRTH */ k_TodoEntry,
    /* CHILD_AT_CRECHE */ k_TodoEntry,
    /* CHILD_FOLLOWS_MOTHER */ k_TodoEntry,
    /* CHILD_BECOMES_ADULT */ k_TodoEntry,
    /* SITS_DOWN_TO_DINNER */ k_TodoEntry,
    /* EAT_FOOD */ k_TodoEntry,
    /* EAT_FOOD_AT_HOME */ k_TodoEntry,
    /* GOTO_BED_AT_HOME */ k_TodoEntry,
    /* SLEEPING_AT_HOME */ k_TodoEntry,
    /* WAKE_UP_AT_HOME */ k_TodoEntry,
    /* START_HAVING_SEX */ k_TodoEntry,
    /* HAVING_SEX */ k_TodoEntry,
    /* STOP_HAVING_SEX */ k_TodoEntry,
    /* START_HAVING_SEX_AT_HOME */ k_TodoEntry,
    /* HAVING_SEX_AT_HOME */ k_TodoEntry,
    /* STOP_HAVING_SEX_AT_HOME */ k_TodoEntry,
    /* WAIT_FOR_DINNER */ k_TodoEntry,
    /* HOMELESS_START */ k_TodoEntry,
    /* VAGRANT_START */ k_TodoEntry,
    /* MORN_DEATH */ k_TodoEntry,
    /* PERFORM_INSPECTION_REACTION */ k_TodoEntry,
    /* APPROACH_OBJECT_REACTION */ k_TodoEntry,
    /* INITIALISE_TELL_OTHERS_ABOUT_OBJECT */ k_TodoEntry,
    /* TELL_OTHERS_ABOUT_INTERESTING_OBJECT */ k_TodoEntry,
    /* APPROACH_VILLAGER_TO_TALK_TO */ k_TodoEntry,
    /* TELL_PARTICULAR_VILLAGER_ABOUT_OBJECT */ k_TodoEntry,
    /* INITIALISE_LOOK_AROUND_FOR_VILLAGER_TO_TELL */ k_TodoEntry,
    /* LOOK_AROUND_FOR_VILLAGER_TO_TELL */ k_TodoEntry,
    /* MOVE_TOWARDS_OBJECT_TO_LOOK_AT */ k_TodoEntry,
    /* INITIALISE_IMPRESSED_REACTION */ k_TodoEntry,
    /* PERFORM_IMPRESSED_REACTION */ k_TodoEntry,
    /* INITIALISE_FIGHT_REACTION */ k_TodoEntry,
    /* PERFORM_FIGHT_REACTION */ k_TodoEntry,
    /* HOMELESS_EAT_DINNER */ k_TodoEntry,
    /* INSPECT_CREATURE_REACTION */ k_TodoEntry,
    /* PERFORM_INSPECT_CREATURE_REACTION */ k_TodoEntry,
    /* APPROACH_CREATURE_REACTION */ k_TodoEntry,
    /* INITIALISE_BEWILDERED_BY_MAGIC_TREE_REACTION */ k_TodoEntry,
    /* PERFORM_BEWILDERED_BY_MAGIC_TREE_REACTION */ k_TodoEntry,
    /* TURN_TO_FACE_MAGIC_TREE */ k_TodoEntry,
    /* LOOK_AT_MAGIC_TREE */ k_TodoEntry,
    /* DANCE_FOR_EDITING_PURPOSES */ k_TodoEntry,
    /* MOVE_TO_DANCE_POS */ k_TodoEntry,
    /* INITIALISE_RESPECT_CREATURE_REACTION */ k_TodoEntry,
    /* PERFORM_RESPECT_CREATURE_REACTION */ k_TodoEntry,
    /* FINISH_RESPECT_CREATURE_REACTION */ k_TodoEntry,
    /* APPROACH_HAND_REACTION */ k_TodoEntry,
    /* FLEEING_FROM_CREATURE_REACTION */ k_TodoEntry,
    /* TURN_TO_FACE_CREATURE_REACTION */ k_TodoEntry,
    /* WATCH_FLYING_OBJECT_REACTION */ k_TodoEntry,
    /* POINT_AT_FLYING_OBJECT_REACTION */ k_TodoEntry,
    /* DECIDE_WHAT_TO_DO */ k_TodoEntry,
    /* INTERACT_DECIDE_WHAT_TO_DO */ k_TodoEntry,
    /* EAT_OUTSIDE */ k_TodoEntry,
    /* RUN_AWAY_FROM_OBJECT_REACTION */ k_TodoEntry,
    /* MOVE_TOWARDS_CREATURE_REACTION */ k_TodoEntry,
    /* AMAZED_BY_MAGIC_SHIELD_REACTION */ k_TodoEntry,
    /* VILLAGER_GOSSIPS */ k_TodoEntry,
    /* CHECK_INTERACT_WITH_ANIMAL */ k_TodoEntry,
    /* CHECK_INTERACT_WITH_WORSHIP_SITE */ k_TodoEntry,
    /* CHECK_INTERACT_WITH_ABODE */ k_TodoEntry,
    /* CHECK_INTERACT_WITH_FIELD */ k_TodoEntry,
    /* CHECK_INTERACT_WITH_FISH_FARM */ k_TodoEntry,
    /* CHECK_INTERACT_WITH_TREE */ k_TodoEntry,
    /* CHECK_INTERACT_WITH_BALL */ k_TodoEntry,
    /* CHECK_INTERACT_WITH_POT */ k_TodoEntry,
    /* CHECK_INTERACT_WITH_FOOTBALL */ k_TodoEntry,
    /* CHECK_INTERACT_WITH_VILLAGER */ k_TodoEntry,
    /* CHECK_INTERACT_WITH_MAGIC_LIVING */ k_TodoEntry,
    /* CHECK_INTERACT_WITH_ROCK */ k_TodoEntry,
    /* ARRIVES_AT_ROCK_FOR_WOOD */ k_TodoEntry,
    /* GOT_WOOD_FROM_ROCK */ k_TodoEntry,
    /* REENTER_BUILDING_STATE */ k_TodoEntry,
    /* ARRIVE_AT_PUSH_OBJECT */ k_TodoEntry,
    /* TAKE_WOOD_FROM_TREE */ k_TodoEntry,
    /* TAKE_WOOD_FROM_POT */ k_TodoEntry,
    /* TAKE_WOOD_FROM_TREE_FOR_BUILDING */ k_TodoEntry,
    /* TAKE_WOOD_FROM_POT_FOR_BUILDING */ k_TodoEntry,
    /* SHEPHERD_TAKE_ANIMAL_FOR_SLAUGHTER */ k_TodoEntry,
    /* SHEPHERD_TAKES_CONTROL_OF_FLOCK */ k_TodoEntry,
    /* SHEPHERD_RELEASES_CONTROL_OF_FLOCK */ k_TodoEntry,
    /* DANCE_BUT_NOT_WORSHIP */ k_TodoEntry,
    /* FAINTING_REACTION */ k_TodoEntry,
    /* START_CONFUSED_REACTION */ k_TodoEntry,
    /* CONFUSED_REACTION */ k_TodoEntry,
    /* AFTER_TAP_ON_ABODE */ k_TodoEntry,
    /* WEAK_ON_GROUND */ k_TodoEntry,
    /* SCRIPT_WANDER_AROUND_POSITION */ k_TodoEntry,
    /* SCRIPT_PLAY_ANIM */ k_TodoEntry,
    /* GO_TOWARDS_TELEPORT_REACTION */ k_TodoEntry,
    /* TELEPORT_REACTION */ k_TodoEntry,
    /* DANCE_WHILE_REACTING */ k_TodoEntry,
    /* CONTROLLED_BY_CREATURE */ k_TodoEntry,
    /* POINT_AT_DEAD_PERSON */ k_TodoEntry,
    /* GO_TOWARDS_DEAD_PERSON */ k_TodoEntry,
    /* LOOK_AT_DEAD_PERSON */ k_TodoEntry,
    /* MOURN_DEAD_PERSON */ k_TodoEntry,
    /* NOTHING_TO_DO */ k_TodoEntry,
    /* ARRIVES_AT_WORKSHOP_FOR_DROP_OFF */ k_TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_WORKSHOP_MATERIALS */ k_TodoEntry,
    /* SHOW_POISONED */ k_TodoEntry,
    /* HIDING_AT_WORSHIP_SITE */ k_TodoEntry,
    /* CROWD_REACTION */ k_TodoEntry,
    /* REACT_TO_FIRE */ k_TodoEntry,
    /* PUT_OUT_FIRE_BY_BEATING */ k_TodoEntry,
    /* PUT_OUT_FIRE_WITH_WATER */ k_TodoEntry,
    /* GET_WATER_TO_PUT_OUT_FIRE */ k_TodoEntry,
    /* ON_FIRE */ k_TodoEntry,
    /* MOVE_AROUND_FIRE */ k_TodoEntry,
    /* DISCIPLE_NOTHING_TO_DO */ k_TodoEntry,
    /* FOOTBALL_MOVE_TO_BALL */ k_TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_TRADER_PICK_UP */ k_TodoEntry,
    /* ARRIVES_AT_STORAGE_PIT_FOR_TRADER_DROP_OFF */ k_TodoEntry,
    /* BREEDER_DISCIPLE */ k_TodoEntry,
    /* MISSIONARY_DISCIPLE */ k_TodoEntry,
    /* REACT_TO_BREEDER */ k_TodoEntry,
    /* SHEPHERD_CHECK_ANIMAL_FOR_SLAUGHTER */ k_TodoEntry,
    /* INTERACT_DECIDE_WHAT_TO_DO_FOR_OTHER_VILLAGER */ k_TodoEntry,
    /* ARTIFACT_DANCE */ k_TodoEntry,
    /* FLEEING_FROM_PREDATOR_REACTION */ k_TodoEntry,
    /* WAIT_FOR_WOOD */ k_TodoEntry,
    /* INSPECT_OBJECT */ k_TodoEntry,
    /* GO_HOME_AND_CHANGE */ k_TodoEntry,
    /* WAIT_FOR_MATE */ k_TodoEntry,
    /* GO_AND_HIDE_IN_NEARBY_BUILDING */ k_TodoEntry,
    /* LOOK_TO_SEE_IF_IT_IS_SAFE */ k_TodoEntry,
    /* SLEEP_IN_TENT */ k_TodoEntry,
    /* PAUSE_FOR_A_SECOND */ k_TodoEntry,
    /* PANIC_REACTION */ k_TodoEntry,
    /* GET_FOOD_AT_WORSHIP_SITE */ k_TodoEntry,
    /* GOTO_CONGREGATE_IN_TOWN_AFTER_EMERGENCY */ k_TodoEntry,
    /* CONGREGATE_IN_TOWN_AFTER_EMERGENCY */ k_TodoEntry,
    /* SCRIPT_IN_CROWD */ k_TodoEntry,
    /* GO_AND_CHILLOUT_OUTSIDE_HOME */ k_TodoEntry,
    /* SIT_AND_CHILLOUT */ k_TodoEntry,
    /* SCRIPT_GO_AND_MOVE_ALONG_PATH */ k_TodoEntry,
    /* RESTART_MEETING */ k_TodoEntry,
    /* GOTO_ABODE_BURNING_REACTION */ k_TodoEntry,
    /* ARRIVES_AT_ABODE_BURNING_REACTION */ k_TodoEntry,
    /* REPAIRS_ABODE */ k_TodoEntry,
    /* ARRIVES_AT_SCAFFOLD_FOR_PICKUP */ k_TodoEntry,
    /* ARRIVES_AT_BUILDING_SITE_WITH_SCAFFOLD */ k_TodoEntry,
    /* MOVE_SCAFFOLD_TO_BUILDING_SITE */ k_TodoEntry,
};

void LivingActionSystem::Update()
{
	auto& registry = Locator::entitiesRegistry::value();

	registry.Each<LivingAction>([](LivingAction& action) { ++action.turnsSinceStateChange; });

	// TODO(#475): process food speedup

	registry.Each<const Villager, LivingAction>([this]([[maybe_unused]] const Villager& villager, LivingAction& action) {
		VillagerCallValidate(action, LivingAction::Index::Top);
	});
	// TODO(#476): same call but for other types of living

	registry.Each<const Villager, LivingAction>([this]([[maybe_unused]] const Villager& villager, LivingAction& action) {
		VillagerCallValidate(action, LivingAction::Index::Final);
	});
	// TODO(#476): same call but for other types of living

	// TODO(bwrsandman): Store result of this call in vector or with tag component
	registry.Each<const Villager, LivingAction>([this]([[maybe_unused]] const Villager& villager, LivingAction& action) {
		VillagerCallState(action, LivingAction::Index::Top);
	});
	// TODO(#476): same call but for other types of living
}

VillagerStates LivingActionSystem::VillagerGetState(const LivingAction& action, LivingAction::Index index) const
{
	return static_cast<VillagerStates>(action.states.at(static_cast<size_t>(index)));
}

void LivingActionSystem::VillagerSetState(LivingAction& action, LivingAction::Index index, VillagerStates state,
                                          bool skipTransition) const
{
	const auto previousState = static_cast<VillagerStates>(action.states.at(static_cast<size_t>(index)));
	if (previousState != state)
	{
		[[maybe_unused]] auto& registry = Locator::entitiesRegistry::value();
		SPDLOG_LOGGER_TRACE(spdlog::get("ai"), "Villager #{}: Setting state {} -> {}",
		                    static_cast<int>(registry.ToEntity(action)),
		                    k_VillagerStateStrings.at(static_cast<size_t>(previousState)),
		                    k_VillagerStateStrings.at(static_cast<size_t>(state)));

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
	const auto state = action.states.at(static_cast<size_t>(index));
	const auto& entry = k_VillagerStateTable.at(static_cast<size_t>(state));
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
	const auto state = action.states.at(static_cast<size_t>(index));
	const auto& entry = k_VillagerStateTable.at(static_cast<size_t>(state));
	const auto& callback = entry.entryState;
	if (!callback)
	{
		return false;
	}
	return callback(action, src, dst);
}

bool LivingActionSystem::VillagerCallExitState(LivingAction& action, LivingAction::Index index) const
{
	const auto& state = action.states.at(static_cast<size_t>(index));
	const auto& entry = k_VillagerStateTable.at(static_cast<size_t>(state));
	const auto& callback = entry.exitState;
	if (!callback)
	{
		return false;
	}
	return callback(action);
}

int LivingActionSystem::VillagerCallOutOfAnimation(LivingAction& action, LivingAction::Index index) const
{
	const auto state = action.states.at(static_cast<size_t>(index));
	const auto& entry = k_VillagerStateTable.at(static_cast<size_t>(state));
	const auto& callback = entry.transitionAnimation;
	if (!callback)
	{
		return -1;
	}
	return callback(action);
}

bool LivingActionSystem::VillagerCallValidate(LivingAction& action, LivingAction::Index index) const
{
	const auto state = action.states.at(static_cast<size_t>(index));
	const auto& entry = k_VillagerStateTable.at(static_cast<size_t>(state));
	const auto& callback = entry.validate;
	if (!callback)
	{
		return false;
	}
	return callback(action);
}
