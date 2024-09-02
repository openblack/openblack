/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CHLApi.h"

#include <cmath>
#include <cstdint>

#include <sstream>
#include <string>
#include <unordered_set>

#include <LHVM.h>
#include <LHVMTypes.h>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>
#include <glm/vec3.hpp>
#include <spdlog/spdlog.h>

#include "3D/LandIslandInterface.h"
#include "3D/TempleInteriorInterface.h"
#include "Camera/Camera.h"
#include "ECS/Archetypes/MobileStaticArchetype.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "Enums.h"
#include "Locator.h"
#include "ScriptHeaders/ScriptEnums.h"

namespace openblack::chlapi
{

using namespace openblack::ecs::archetypes;

using openblack::Locator;
using openblack::MobileStaticInfo;
using openblack::ecs::components::Transform;
using openblack::ecs::systems::HandSystemInterface;
using openblack::lhvm::DataType;
using openblack::lhvm::VMValue;
using openblack::script::ObjectType;

#define CREATE_FUNCTION_BINDING(NAME, STACKIN, STACKOUT, FUNCTION)       \
	{                                                                    \
		_functionsTable.emplace_back(FUNCTION, STACKIN, STACKOUT, NAME); \
	}

const std::vector<lhvm::NativeFunction>& CHLApi::GetFunctionsTable()
{
	return _functionsTable;
}

std::unordered_set<std::string> GetUniqueWords(const std::string& strings)
{
	std::unordered_set<std::string> result;
	std::istringstream iss(strings);
	std::string word;
	while (std::getline(iss, word, ' '))
	{
		result.insert(word);
	}
	return result;
}

glm::vec3 PopVec()
{
	auto& lhvm = Locator::vm::value();
	const auto z = lhvm.Popf();
	const auto y = lhvm.Popf();
	const auto x = lhvm.Popf();
	return {x, y, z};
}

void PushVec(const glm::vec3& vec)
{
	auto& lhvm = Locator::vm::value();
	lhvm.Pushv(vec.x);
	lhvm.Pushv(vec.y);
	lhvm.Pushv(vec.z);
}

std::string PopString()
{
	auto& lhvm = Locator::vm::value();
	return lhvm.GetString(lhvm.Pop().intVal);
}

std::vector<float> PopVarArg(const int32_t argc)
{
	std::vector<float> vals;
	vals.resize(argc);
	auto& lhvm = Locator::vm::value();
	for (int i = argc - 1; i >= 0; i--)
	{
		vals[i] = lhvm.Popf();
	}
	return vals;
}

entt::entity CreateScriptObject(const ObjectType type, uint32_t subtype, const glm::vec3& position, float altitude,
                                float xAngleRadians, float yAngleRadians, const float zAngleRadians, const float scale)
{
	// TODO(Daniels118): handle all types
	switch (type)
	{
	case ObjectType::MobileStatic:
	case ObjectType::Rock: // TODO(Daniels118): add a Rock archetype
		return MobileStaticArchetype::Create(position, static_cast<MobileStaticInfo>(subtype), altitude, xAngleRadians,
		                                     yAngleRadians, zAngleRadians, scale);
	default:
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "CreateScriptObject not implemented for type {}", static_cast<int>(type));
	}
	return static_cast<entt::entity>(0);
}

VMValue Pop(DataType& type)
{
	auto& lhvm = Locator::vm::value();
	return lhvm.Pop(type);
}

VMValue Pop()
{
	auto& lhvm = Locator::vm::value();
	return lhvm.Pop();
}

float Popf()
{
	auto& lhvm = Locator::vm::value();
	return lhvm.Popf();
}

void Push(VMValue value, DataType type)
{
	auto& lhvm = Locator::vm::value();
	lhvm.Push(value, type);
}

void Pushf(float value)
{
	auto& lhvm = Locator::vm::value();
	lhvm.Pushf(value);
}

void Pushv(float value)
{
	auto& lhvm = Locator::vm::value();
	lhvm.Pushv(value);
}

void Pushi(int32_t value)
{
	auto& lhvm = Locator::vm::value();
	lhvm.Pushi(value);
}

void Pusho(uint32_t value)
{
	auto& lhvm = Locator::vm::value();
	lhvm.Pusho(value);
}

void Pushb(bool value)
{
	auto& lhvm = Locator::vm::value();
	lhvm.Pushb(value);
}

CHLApi::CHLApi()
{
	_functionsTable.reserve(464);
	InitFunctionsTable0();
	InitFunctionsTable1();
	InitFunctionsTable2();
	InitFunctionsTable3();
	InitFunctionsTable4();
}

void None() {} // 000 NONE

void SetCameraPosition() // 001 SET_CAMERA_POSITION
{
	const auto position = PopVec();
	// TODO(Daniels118): check if cinema mode is enabled
	auto& camera = Locator::camera::value();
	camera.SetOrigin(position);
}

void SetCameraFocus() // 002 SET_CAMERA_FOCUS
{
	const auto position = PopVec();
	// TODO(Daniels118): check if cinema mode is enabled
	auto& camera = Locator::camera::value();
	camera.SetFocus(position);
}

void MoveCameraPosition() // 003 MOVE_CAMERA_POSITION
{
	// const auto time = Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void MoveCameraFocus() // 004 MOVE_CAMERA_FOCUS
{
	// const auto time = Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetCameraPosition() // 005 GET_CAMERA_POSITION
{
	auto& camera = Locator::camera::value();
	const auto position = camera.GetOrigin();
	PushVec(position);
}

void GetCameraFocus() // 006 GET_CAMERA_FOCUS
{
	auto& camera = Locator::camera::value();
	const auto focus = camera.GetFocus();
	PushVec(focus);
}

void SpiritEject() // 007 SPIRIT_EJECT
{
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SpiritHome() // 008 SPIRIT_HOME
{
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SpiritPointPos() // 009 SPIRIT_POINT_POS
{
	// const auto inWorld = static_cast<bool>(Pop().intVal);
	// const auto position = PopVec();
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SpiritPointGameThing() // 010 SPIRIT_POINT_GAME_THING
{
	// const auto inWorld = static_cast<bool>(Pop().intVal);
	// const auto target = Pop().uintVal;
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GameThingFieldOfView() // 011 GAME_THING_FIELD_OF_VIEW
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void PosFieldOfView() // 012 POS_FIELD_OF_VIEW
{
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void RunText() // 013 RUN_TEXT
{
	// const auto withInteraction = Pop().intVal;
	// const auto textID = Pop().intVal;
	// const auto singleLine = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void TempText() // 014 TEMP_TEXT
{
	// const auto withInteraction = Pop().intVal;
	// const auto string = PopString();
	// const auto singleLine = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void TextRead() // 015 TEXT_READ
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void GameThingClicked() // 016 GAME_THING_CLICKED
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SetScriptState() // 017 SET_SCRIPT_STATE
{
	// const auto state = Pop().intVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetScriptStatePos() // 018 SET_SCRIPT_STATE_POS
{
	// const auto position = PopVec();
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetScriptFloat() // 019 SET_SCRIPT_FLOAT
{
	// const auto value = Popf();
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetScriptUlong() // 020 SET_SCRIPT_ULONG
{
	// const auto loop = Pop().intVal;
	// const auto animation = Pop().intVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetProperty() // 021 GET_PROPERTY
{
	// const auto object = Pop().uintVal;
	// const auto prop = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void SetProperty() // 022 SET_PROPERTY
{
	// const auto val = Popf();
	// const auto object = Pop().uintVal;
	// const auto prop = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetPosition() // 023 GET_POSITION
{
	const auto objId = Pop().uintVal;

	glm::vec3 position(0.0f);
	if (objId != 0)
	{
		auto& registry = Locator::entitiesRegistry::value();
		auto* transform = registry.TryGet<Transform>(static_cast<entt::entity>(objId));
		if (transform != nullptr)
		{
			position = transform->position;
		}
	}

	PushVec(position);
}

void SetPosition() // 024 SET_POSITION
{
	auto position = PopVec();
	const auto objId = Pop().uintVal;

	if (objId != 0)
	{
		const auto& island = Locator::terrainSystem::value();
		position.y = island.GetHeightAt(glm::vec2(position.x, position.z));
		auto& registry = Locator::entitiesRegistry::value();
		auto* transform = registry.TryGet<Transform>(static_cast<entt::entity>(objId));
		if (transform != nullptr)
		{
			transform->position = position;
		}
	}
}

void GetDistance() // 025 GET_DISTANCE
{
	const auto p1 = PopVec();
	const auto p0 = PopVec();
	const auto distance = glm::length(p1 - p0);
	Pushf(distance);
}

void Call() // 026 CALL
{
	// const auto excludingScripted = static_cast<bool>(Pop().intVal);
	// const auto position = PopVec();
	// const auto subtype = Pop().intVal;
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void Create() // 027 CREATE
{
	const auto position = PopVec();
	const auto subtype = Pop().intVal;
	const auto type = static_cast<ObjectType>(Pop().intVal);

	const auto object = CreateScriptObject(type, subtype, position, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	Pusho(static_cast<uint32_t>(object));
}

void Random() // 028 RANDOM
{
	const auto max = Popf();
	const auto min = Popf();
	const float random = min + (max - min) * static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	Pushf(random);
}

void DllGettime() // 029 DLL_GETTIME
{
	// TODO(Daniels118): need a way to access Game::GetTurn()
	// Pushf(static_cast<float>(_turnCount) / 10.0f); // TODO(Daniels118): should it be divided by 10 or not?
}

void StartCameraControl() // 030 START_CAMERA_CONTROL
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void EndCameraControl() // 031 END_CAMERA_CONTROL
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetWidescreen() // 032 SET_WIDESCREEN
{
	// const auto enabled = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void MoveGameThing() // 033 MOVE_GAME_THING
{
	// const auto radius = Popf();
	// const auto position = PopVec();
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetFocus() // 034 SET_FOCUS
{
	// const auto position = PopVec();
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void HasCameraArrived() // 035 HAS_CAMERA_ARRIVED
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void FlockCreate() // 036 FLOCK_CREATE
{
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void FlockAttach() // 037 FLOCK_ATTACH
{
	// const auto asLeader = static_cast<bool>(Pop().intVal);
	// const auto flock = Pop().uintVal;
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void FlockDetach() // 038 FLOCK_DETACH
{
	// const auto flock = Pop().uintVal;
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void FlockDisband() // 039 FLOCK_DISBAND
{
	// const auto flock = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void IdSize() // 040 ID_SIZE
{
	// const auto container = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void FlockMember() // 041 FLOCK_MEMBER
{
	// const auto flock = Pop().uintVal;
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void GetHandPosition() // 042 GET_HAND_POSITION
{
	const auto handEntity = Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(HandSystemInterface::Side::Left)];
	auto& handTransform = Locator::entitiesRegistry::value().Get<Transform>(handEntity);

	PushVec(handTransform.position);
}

void PlaySoundEffect() // 043 PLAY_SOUND_EFFECT
{
	// const auto withPosition = static_cast<bool>(Pop().intVal);
	// const auto position = PopVec();
	// const auto soundbank = Pop().intVal;
	// const auto sound = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StartMusic() // 044 START_MUSIC
{
	// const auto music = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StopMusic() // 045 STOP_MUSIC
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void AttachMusic() // 046 ATTACH_MUSIC
{
	// const auto target = Pop().uintVal;
	// const auto music = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void DetachMusic() // 047 DETACH_MUSIC
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ObjectDelete() // 048 OBJECT_DELETE
{
	// const auto withFade = Pop().intVal;
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void FocusFollow() // 049 FOCUS_FOLLOW
{
	// const auto target = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void PositionFollow() // 050 POSITION_FOLLOW
{
	// const auto target = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CallNear() // 051 CALL_NEAR
{
	// const auto excludingScripted = static_cast<bool>(Pop().intVal);
	// const auto radius = Popf();
	// const auto position = PopVec();
	// const auto subtype = Pop().intVal;
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SpecialEffectPosition() // 052 SPECIAL_EFFECT_POSITION
{
	// const auto duration = Popf();
	// const auto position = PopVec();
	// const auto effect = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SpecialEffectObject() // 053 SPECIAL_EFFECT_OBJECT
{
	// const auto duration = Popf();
	// const auto target = Pop().uintVal;
	// const auto effect = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void DanceCreate() // 054 DANCE_CREATE
{
	// const auto duration = Popf();
	// const auto position = PopVec();
	// const auto type = Pop().intVal;
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void CallIn() // 055 CALL_IN
{
	// const auto excludingScripted = static_cast<bool>(Pop().intVal);
	// const auto container = Pop().uintVal;
	// const auto subtype = Pop().intVal;
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void ChangeInnerOuterProperties() // 056 CHANGE_INNER_OUTER_PROPERTIES
{
	// const auto calm = Popf();
	// const auto outer = Popf();
	// const auto inner = Popf();
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void Snapshot() // 057 SNAPSHOT
{
	// const auto challengeId = Pop().intVal;
	// const auto argc = Pop().intVal;
	// const auto argv = PopVarArg(argc);
	// const auto reminderScript = PopString();
	// const auto titleStrID = Pop().intVal;
	// const auto alignment = Popf();
	// const auto success = Popf();
	// const auto focus = PopVec();
	// const auto position = PopVec();
	// const auto quest = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetAlignment() // 058 GET_ALIGNMENT
{
	// const auto zero = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void SetAlignment() // 059 SET_ALIGNMENT
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void InfluenceObject() // 060 INFLUENCE_OBJECT
{
	// const auto anti = Pop().intVal;
	// const auto zero = Pop().intVal;
	// const auto radius = Popf();
	// const auto target = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void InfluencePosition() // 061 INFLUENCE_POSITION
{
	// const auto anti = Pop().intVal;
	// const auto zero = Pop().intVal;
	// const auto radius = Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void GetInfluence() // 062 GET_INFLUENCE
{
	// const auto position = PopVec();
	// const auto raw = static_cast<bool>(Pop().intVal);
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void SetInterfaceInteraction() // 063 SET_INTERFACE_INTERACTION
{
	// const auto level = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void Played() // 064 PLAYED
{
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void RandomUlong() // 065 RANDOM_ULONG
{
	// const auto max = Pop().intVal;
	// const auto min = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void SetGamespeed() // 066 SET_GAMESPEED
{
	// const auto speed = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CallInNear() // 067 CALL_IN_NEAR
{
	// const auto excludingScripted = static_cast<bool>(Pop().intVal);
	// const auto radius = Popf();
	// const auto pos = PopVec();
	// const auto container = Pop().uintVal;
	// const auto subtype = Pop().intVal;
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void OverrideStateAnimation() // 068 OVERRIDE_STATE_ANIMATION
{
	// const auto animType = Pop().intVal;
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureCreateRelativeToCreature() // 069 CREATURE_CREATE_RELATIVE_TO_CREATURE
{
	// const auto type = Pop().intVal;
	// const auto position = PopVec();
	// const auto scale = Popf();
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void CreatureLearnEverything() // 070 CREATURE_LEARN_EVERYTHING
{
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureSetKnowsAction() // 071 CREATURE_SET_KNOWS_ACTION
{
	// const auto knows = Pop().intVal;
	// const auto action = Pop().intVal;
	// const auto typeOfAction = Pop().intVal;
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureSetAgendaPriority() // 072 CREATURE_SET_AGENDA_PRIORITY
{
	// const auto priority = Popf();
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureTurnOffAllDesires() // 073 CREATURE_TURN_OFF_ALL_DESIRES
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureLearnDistinctionAboutActivityObject() // 074 CREATURE_LEARN_DISTINCTION_ABOUT_ACTIVITY_OBJECT
{
	// const auto unk3 = Pop().intVal;
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureDoAction() // 075 CREATURE_DO_ACTION
{
	// const auto withObject = Pop().uintVal;
	// const auto target = Pop().uintVal;
	// const auto unk1 = Pop().intVal;
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void InCreatureHand() // 076 IN_CREATURE_HAND
{
	// const auto creature = Pop().uintVal;
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void CreatureSetDesireValue() // 077 CREATURE_SET_DESIRE_VALUE
{
	// const auto value = Popf();
	// const auto desire = Pop().intVal;
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureSetDesireActivated78() // 078 CREATURE_SET_DESIRE_ACTIVATED
{
	// const auto active = Pop().intVal;
	// const auto desire = Pop().intVal;
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureSetDesireActivated79() // 079 CREATURE_SET_DESIRE_ACTIVATED
{
	// const auto active = Pop().intVal;
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureSetDesireMaximum() // 080 CREATURE_SET_DESIRE_MAXIMUM
{
	// const auto value = Popf();
	// const auto desire = Pop().intVal;
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ConvertCameraPosition() // 081 CONVERT_CAMERA_POSITION
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushv(0.0f); // x
	Pushv(0.0f); // y
	Pushv(0.0f); // z
}

void ConvertCameraFocus() // 082 CONVERT_CAMERA_FOCUS
{
	// const auto camera_enum = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushv(0.0f); // x
	Pushv(0.0f); // y
	Pushv(0.0f); // z
}

void CreatureSetPlayer() // 083 CREATURE_SET_PLAYER
{
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StartCountdownTimer() // 084 START_COUNTDOWN_TIMER
{
	// const auto timeout = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureInitialiseNumTimesPerformedAction() // 085 CREATURE_INITIALISE_NUM_TIMES_PERFORMED_ACTION
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureGetNumTimesActionPerformed() // 086 CREATURE_GET_NUM_TIMES_ACTION_PERFORMED
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void RemoveCountdownTimer() // 087 REMOVE_COUNTDOWN_TIMER
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetObjectDropped() // 088 GET_OBJECT_DROPPED
{
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void ClearDroppedByObject() // 089 CLEAR_DROPPED_BY_OBJECT
{
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreateReaction() // 090 CREATE_REACTION
{
	// const auto reaction = Pop().intVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void RemoveReaction() // 091 REMOVE_REACTION
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetCountdownTimer() // 092 GET_COUNTDOWN_TIMER
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void StartDualCamera() // 093 START_DUAL_CAMERA
{
	// const auto obj2 = Pop().uintVal;
	// const auto obj1 = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void UpdateDualCamera() // 094 UPDATE_DUAL_CAMERA
{
	// const auto obj2 = Pop().uintVal;
	// const auto obj1 = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ReleaseDualCamera() // 095 RELEASE_DUAL_CAMERA
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetCreatureHelp() // 096 SET_CREATURE_HELP
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetTargetObject() // 097 GET_TARGET_OBJECT
{
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void CreatureDesireIs() // 098 CREATURE_DESIRE_IS
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void CountdownTimerExists() // 099 COUNTDOWN_TIMER_EXISTS
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void LookGameThing() // 100 LOOK_GAME_THING
{
	// const auto target = Pop().uintVal;
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetObjectDestination() // 101 GET_OBJECT_DESTINATION
{
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushv(0.0f); // x
	Pushv(0.0f); // y
	Pushv(0.0f); // z
}

void CreatureForceFinish() // 102 CREATURE_FORCE_FINISH
{
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void HideCountdownTimer() // 103 HIDE_COUNTDOWN_TIMER
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetActionTextForObject() // 104 GET_ACTION_TEXT_FOR_OBJECT
{
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void CreateDualCameraWithPoint() // 105 CREATE_DUAL_CAMERA_WITH_POINT
{
	// const auto position = PopVec();
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetCameraToFaceObject() // 106 SET_CAMERA_TO_FACE_OBJECT
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void MoveCameraToFaceObject() // 107 MOVE_CAMERA_TO_FACE_OBJECT
{
	// const auto time = Popf();
	// const auto distance = Popf();
	// const auto target = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetMoonPercentage() // 108 GET_MOON_PERCENTAGE
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void PopulateContainer() // 109 POPULATE_CONTAINER
{
	// const auto subtype = Pop().intVal;
	// const auto type = Pop().intVal;
	// const auto quantity = Popf();
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void AddReference() // 110 ADD_REFERENCE
{
	const auto objId = Pop().uintVal;
	// TODO(Daniels118): implement this - HIGH PRIORITY
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(objId);
}

void RemoveReference() // 111 REMOVE_REFERENCE
{
	const auto objId = Pop().uintVal;
	// TODO(Daniels118): implement this - HIGH PRIORITY
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(objId);
}

void SetGameTime() // 112 SET_GAME_TIME
{
	// const auto time = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetGameTime() // 113 GET_GAME_TIME
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetRealTime() // 114 GET_REAL_TIME
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetRealDay115() // 115 GET_REAL_DAY
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetRealDay116() // 116 GET_REAL_DAY
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetRealMonth() // 117 GET_REAL_MONTH
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetRealYear() // 118 GET_REAL_YEAR
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void RunCameraPath() // 119 RUN_CAMERA_PATH
{
	// const auto cameraEnum = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StartDialogue() // 120 START_DIALOGUE
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void EndDialogue() // 121 END_DIALOGUE
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void IsDialogueReady() // 122 IS_DIALOGUE_READY
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void ChangeWeatherProperties() // 123 CHANGE_WEATHER_PROPERTIES
{
	// const auto fallspeed = Popf();
	// const auto overcast = Popf();
	// const auto snowfall = Popf();
	// const auto rainfall = Popf();
	// const auto temperature = Popf();
	// const auto storm = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ChangeLightningProperties() // 124 CHANGE_LIGHTNING_PROPERTIES
{
	// const auto forkmax = Popf();
	// const auto forkmin = Popf();
	// const auto sheetmax = Popf();
	// const auto sheetmin = Popf();
	// const auto storm = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ChangeTimeFadeProperties() // 125 CHANGE_TIME_FADE_PROPERTIES
{
	// const auto fadeTime = Popf();
	// const auto duration = Popf();
	// const auto storm = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ChangeCloudProperties() // 126 CHANGE_CLOUD_PROPERTIES
{
	// const auto elevation = Popf();
	// const auto blackness = Popf();
	// const auto numClouds = Popf();
	// const auto storm = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetHeadingAndSpeed() // 127 SET_HEADING_AND_SPEED
{
	// const auto speed = Popf();
	// const auto position = PopVec();
	// const auto unk0 = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StartGameSpeed() // 128 START_GAME_SPEED
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void EndGameSpeed() // 129 END_GAME_SPEED
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void BuildBuilding() // 130 BUILD_BUILDING
{
	// const auto desire = Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetAffectedByWind() // 131 SET_AFFECTED_BY_WIND
{
	// const auto object = Pop().uintVal;
	// const auto enabled = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void WidescreenTransistionFinished() // 132 WIDESCREEN_TRANSISTION_FINISHED
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void GetResource() // 133 GET_RESOURCE
{
	// const auto container = Pop().uintVal;
	// const auto resource = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void AddResource() // 134 ADD_RESOURCE
{
	// const auto container = Pop().uintVal;
	// const auto quantity = Popf();
	// const auto resource = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void RemoveResource() // 135 REMOVE_RESOURCE
{
	// const auto container = Pop().uintVal;
	// const auto quantity = Popf();
	// const auto resource = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetTargetRelativePos() // 136 GET_TARGET_RELATIVE_POS
{
	// const auto angle = Popf();
	// const auto distance = Popf();
	// const auto to = PopVec();
	// const auto from = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushv(0.0f); // x
	Pushv(0.0f); // y
	Pushv(0.0f); // z
}

void StopPointing() // 137 STOP_POINTING
{
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StopLooking() // 138 STOP_LOOKING
{
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void LookAtPosition() // 139 LOOK_AT_POSITION
{
	// const auto position = PopVec();
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void PlaySpiritAnim() // 140 PLAY_SPIRIT_ANIM
{
	// const auto unk4 = Pop().intVal;
	// const auto unk3 = Pop().intVal;
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CallInNotNear() // 141 CALL_IN_NOT_NEAR
{
	// const auto excludingScripted = static_cast<bool>(Pop().intVal);
	// const auto radius = Popf();
	// const auto pos = PopVec();
	// const auto container = Pop().uintVal;
	// const auto subtype = Pop().intVal;
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SetCameraZone() // 142 SET_CAMERA_ZONE
{
	// const auto filename = PopString();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetObjectState() // 143 GET_OBJECT_STATE
{
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void RevealCountdownTimer() // 144 REVEAL_COUNTDOWN_TIMER
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetTimerTime() // 145 SET_TIMER_TIME
{
	// const auto time = Popf();
	// const auto timer = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreateTimer() // 146 CREATE_TIMER
{
	// const auto timeout = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void GetTimerTimeRemaining() // 147 GET_TIMER_TIME_REMAINING
{
	// const auto timer = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetTimerTimeSinceSet() // 148 GET_TIMER_TIME_SINCE_SET
{
	// const auto timer = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void MoveMusic() // 149 MOVE_MUSIC
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetInclusionDistance() // 150 GET_INCLUSION_DISTANCE
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetLandHeight() // 151 GET_LAND_HEIGHT
{
	const auto position = PopVec();

	const auto& island = Locator::terrainSystem::value();
	const auto elevation = island.GetHeightAt(glm::vec2(position.x, position.z));

	Pushf(elevation);
}

void LoadMap() // 152 LOAD_MAP
{
	// const auto path = PopString();

	// auto& fileSystem = Locator::filesystem::value();
	// auto mapPath = fileSystem.GetGamePath() / path;
	// TODO(Daniels118): LoadMap(mapPath);
}

void StopAllScriptsExcluding() // 153 STOP_ALL_SCRIPTS_EXCLUDING
{
	const auto scriptNames = PopString();

	const auto names = GetUniqueWords(scriptNames);
	auto& lhvm = Locator::vm::value();
	lhvm.StopScripts([&names](const std::string& name, [[maybe_unused]] const std::string& filename) -> bool {
		return !names.contains(name);
	});
}

void StopAllScriptsInFilesExcluding() // 154 STOP_ALL_SCRIPTS_IN_FILES_EXCLUDING
{
	const auto sourceFilenames = PopString();

	const auto filenames = GetUniqueWords(sourceFilenames);
	auto& lhvm = Locator::vm::value();
	lhvm.StopScripts([&filenames]([[maybe_unused]] const std::string& name, const std::string& filename) -> bool {
		return !filenames.contains(filename);
	});
}

void StopScript() // 155 STOP_SCRIPT
{
	const auto scriptName = PopString();
	auto& lhvm = Locator::vm::value();
	lhvm.StopScripts([&scriptName](const std::string& name, [[maybe_unused]] const std::string& filename) -> bool {
		return name == scriptName;
	});
}

void ClearClickedObject() // 156 CLEAR_CLICKED_OBJECT
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ClearClickedPosition() // 157 CLEAR_CLICKED_POSITION
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void PositionClicked() // 158 POSITION_CLICKED
{
	// const auto unk3 = Pop().intVal;
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void ReleaseFromScript() // 159 RELEASE_FROM_SCRIPT
{
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetObjectHandIsOver() // 160 GET_OBJECT_HAND_IS_OVER
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void IdPoisonedSize() // 161 ID_POISONED_SIZE
{
	// const auto container = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void IsPoisoned() // 162 IS_POISONED
{
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void CallPoisonedIn() // 163 CALL_POISONED_IN
{
	// const auto excludingScripted = static_cast<bool>(Pop().intVal);
	// const auto container = Pop().uintVal;
	// const auto subtype = Pop().intVal;
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void CallNotPoisonedIn() // 164 CALL_NOT_POISONED_IN
{
	// const auto excludingScripted = static_cast<bool>(Pop().intVal);
	// const auto container = Pop().uintVal;
	// const auto subtype = Pop().intVal;
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SpiritPlayed() // 165 SPIRIT_PLAYED
{
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void ClingSpirit() // 166 CLING_SPIRIT
{
	// const auto yPercent = Popf();
	// const auto xPercent = Popf();
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void FlySpirit() // 167 FLY_SPIRIT
{
	// const auto yPercent = Popf();
	// const auto xPercent = Popf();
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetIdMoveable() // 168 SET_ID_MOVEABLE
{
	// const auto obj = Pop().uintVal;
	// const auto moveable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetIdPickupable() // 169 SET_ID_PICKUPABLE
{
	// const auto obj = Pop().uintVal;
	// const auto pickupable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void IsOnFire() // 170 IS_ON_FIRE
{
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void IsFireNear() // 171 IS_FIRE_NEAR
{
	// const auto radius = Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void StopScriptsInFiles() // 172 STOP_SCRIPTS_IN_FILES
{
	const auto sourceFilenames = PopString();

	const auto filenames = GetUniqueWords(sourceFilenames);
	auto& lhvm = Locator::vm::value();
	lhvm.StopScripts([&filenames]([[maybe_unused]] const std::string& name, const std::string& filename) -> bool {
		return filenames.contains(filename);
	});
}

void SetPoisoned() // 173 SET_POISONED
{
	// const auto obj = Pop().uintVal;
	// const auto poisoned = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetTemperature() // 174 SET_TEMPERATURE
{
	// const auto temperature = Popf();
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetOnFire() // 175 SET_ON_FIRE
{
	// const auto burnSpeed = Popf();
	// const auto object = Pop().uintVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetTarget() // 176 SET_TARGET
{
	// const auto time = Popf();
	// const auto position = PopVec();
	// const auto obj = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void WalkPath() // 177 WALK_PATH
{
	// const auto valTo = Popf();
	// const auto valFrom = Popf();
	// const auto camera_enum = Pop().intVal;
	// const auto forward = static_cast<bool>(Pop().intVal);
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void FocusAndPositionFollow() // 178 FOCUS_AND_POSITION_FOLLOW
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetWalkPathPercentage() // 179 GET_WALK_PATH_PERCENTAGE
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void CameraProperties() // 180 CAMERA_PROPERTIES
{
	// const auto enableBehind = static_cast<bool>(Pop().intVal);
	// const auto angle = Popf();
	// const auto speed = Popf();
	// const auto distance = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void EnableDisableMusic() // 181 ENABLE_DISABLE_MUSIC
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetMusicObjDistance() // 182 GET_MUSIC_OBJ_DISTANCE
{
	// const auto source = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetMusicEnumDistance() // 183 GET_MUSIC_ENUM_DISTANCE
{
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void SetMusicPlayPosition() // 184 SET_MUSIC_PLAY_POSITION
{
	// const auto unk3 = Pop().intVal;
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void AttachObjectLeashToObject() // 185 ATTACH_OBJECT_LEASH_TO_OBJECT
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void AttachObjectLeashToHand() // 186 ATTACH_OBJECT_LEASH_TO_HAND
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void DetachObjectLeash() // 187 DETACH_OBJECT_LEASH
{
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetCreatureOnlyDesire() // 188 SET_CREATURE_ONLY_DESIRE
{
	// const auto value = Popf();
	// const auto desire = Pop().intVal;
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetCreatureOnlyDesireOff() // 189 SET_CREATURE_ONLY_DESIRE_OFF
{
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void RestartMusic() // 190 RESTART_MUSIC
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void MusicPlayed191() // 191 MUSIC_PLAYED
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void IsOfType() // 192 IS_OF_TYPE
{
	// const auto subtype = Pop().intVal;
	// const auto type = Pop().intVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void ClearHitObject() // 193 CLEAR_HIT_OBJECT
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GameThingHit() // 194 GAME_THING_HIT
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SpellAtThing() // 195 SPELL_AT_THING
{
	// const auto curl = Popf();
	// const auto duration = Popf();
	// const auto radius = Popf();
	// const auto from = PopVec();
	// const auto target = Pop().uintVal;
	// const auto spell = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SpellAtPos() // 196 SPELL_AT_POS
{
	// const auto curl = Popf();
	// const auto duration = Popf();
	// const auto radius = Popf();
	// const auto from = PopVec();
	// const auto target = PopVec();
	// const auto spell = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void CallPlayerCreature() // 197 CALL_PLAYER_CREATURE
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void GetSlowestSpeed() // 198 GET_SLOWEST_SPEED
{
	// const auto flock = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetObjectHeld199() // 199 GET_OBJECT_HELD
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void HelpSystemOn() // 200 HELP_SYSTEM_ON
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void ShakeCamera() // 201 SHAKE_CAMERA
{
	// const auto duration = Popf();
	// const auto amplitude = Popf();
	// const auto radius = Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetAnimationModify() // 202 SET_ANIMATION_MODIFY
{
	// const auto creature = Pop().uintVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetAviSequence() // 203 SET_AVI_SEQUENCE
{
	// const auto aviSequence = Pop().intVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void PlayGesture() // 204 PLAY_GESTURE
{
	// const auto unk4 = Pop().intVal;
	// const auto unk3 = Pop().intVal;
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void DevFunction() // 205 DEV_FUNCTION
{
	// const auto func = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void HasMouseWheel() // 206 HAS_MOUSE_WHEEL
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void NumMouseButtons() // 207 NUM_MOUSE_BUTTONS
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void SetCreatureDevStage() // 208 SET_CREATURE_DEV_STAGE
{
	// const auto stage = Pop().intVal;
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetFixedCamRotation() // 209 SET_FIXED_CAM_ROTATION
{
	// const auto unk3 = Pop().intVal;
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SwapCreature() // 210 SWAP_CREATURE
{
	// const auto toCreature = Pop().uintVal;
	// const auto fromCreature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetArena() // 211 GET_ARENA
{
	// const auto unk4 = Pop().intVal;
	// const auto unk3 = Pop().intVal;
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void GetFootballPitch() // 212 GET_FOOTBALL_PITCH
{
	// const auto town = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void StopAllGames() // 213 STOP_ALL_GAMES
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void AttachToGame() // 214 ATTACH_TO_GAME
{
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void DetachFromGame() // 215 DETACH_FROM_GAME
{
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void DetachUndefinedFromGame() // 216 DETACH_UNDEFINED_FROM_GAME
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetOnlyForScripts() // 217 SET_ONLY_FOR_SCRIPTS
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StartMatchWithReferee() // 218 START_MATCH_WITH_REFEREE
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GameTeamSize() // 219 GAME_TEAM_SIZE
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GameType() // 220 GAME_TYPE
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void GameSubType() // 221 GAME_SUB_TYPE
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void IsLeashed() // 222 IS_LEASHED
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SetCreatureHome() // 223 SET_CREATURE_HOME
{
	// const auto position = PopVec();
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetHitObject() // 224 GET_HIT_OBJECT
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void GetObjectWhichHit() // 225 GET_OBJECT_WHICH_HIT
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void GetNearestTownOfPlayer() // 226 GET_NEAREST_TOWN_OF_PLAYER
{
	// const auto unk4 = Pop().intVal;
	// const auto unk3 = Pop().intVal;
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SpellAtPoint() // 227 SPELL_AT_POINT
{
	// const auto radius = Popf();
	// const auto position = PopVec();
	// const auto spell = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SetAttackOwnTown() // 228 SET_ATTACK_OWN_TOWN
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void IsFighting() // 229 IS_FIGHTING
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SetMagicRadius() // 230 SET_MAGIC_RADIUS
{
	// const auto radius = Popf();
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void TempTextWithNumber() // 231 TEMP_TEXT_WITH_NUMBER
{
	// const auto withInteraction = Pop().intVal;
	// const auto value = Popf();
	// const auto format = PopString();
	// const auto singleLine = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void RunTextWithNumber() // 232 RUN_TEXT_WITH_NUMBER
{
	// const auto withInteraction = Pop().intVal;
	// const auto number = Popf();
	// const auto string = Pop().intVal;
	// const auto singleLine = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureSpellReversion() // 233 CREATURE_SPELL_REVERSION
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetDesire() // 234 GET_DESIRE
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetEventsPerSecond() // 235 GET_EVENTS_PER_SECOND
{
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetTimeSince() // 236 GET_TIME_SINCE
{
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetTotalEvents() // 237 GET_TOTAL_EVENTS
{
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void UpdateSnapshot() // 238 UPDATE_SNAPSHOT
{
	// const auto challengeId = Pop().intVal;
	// const auto argc = Pop().intVal;
	// const auto argv = PopVarArg(argc);
	// const auto reminderScript = PopString();
	// const auto titleStrID = Pop().intVal;
	// const auto alignment = Popf();
	// const auto success = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreateReward() // 239 CREATE_REWARD
{
	// const auto fromSky = static_cast<bool>(Pop().intVal);
	// const auto position = PopVec();
	// const auto reward = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void CreateRewardInTown() // 240 CREATE_REWARD_IN_TOWN
{
	// const auto fromSky = static_cast<bool>(Pop().intVal);
	// const auto position = PopVec();
	// const auto town = Pop().uintVal;
	// const auto reward = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SetFade() // 241 SET_FADE
{
	// const auto time = Popf();
	// const auto blue = Popf();
	// const auto green = Popf();
	// const auto red = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetFadeIn() // 242 SET_FADE_IN
{
	// const auto duration = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void FadeFinished() // 243 FADE_FINISHED
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SetPlayerMagic() // 244 SET_PLAYER_MAGIC
{
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void HasPlayerMagic() // 245 HAS_PLAYER_MAGIC
{
	// const auto player = Popf();
	// const auto spell = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SpiritSpeaks() // 246 SPIRIT_SPEAKS
{
	// const auto textID = Pop().intVal;
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void BeliefForPlayer() // 247 BELIEF_FOR_PLAYER
{
	// const auto player = Popf();
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetHelp() // 248 GET_HELP
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void SetLeashWorks() // 249 SET_LEASH_WORKS
{
	// const auto creature = Pop().uintVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void LoadMyCreature() // 250 LOAD_MY_CREATURE
{
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ObjectRelativeBelief() // 251 OBJECT_RELATIVE_BELIEF
{
	// const auto belief = Popf();
	// const auto player = Popf();
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreateWithAngleAndScale() // 252 CREATE_WITH_ANGLE_AND_SCALE
{
	const auto position = PopVec();
	const auto subtype = Pop().intVal;
	const auto type = static_cast<ObjectType>(Pop().intVal);
	const auto scale = Popf();
	const auto angle = Popf();

	const entt::entity object = CreateScriptObject(type, subtype, position, 0.0f, 0.0f, angle, 0.0f, scale);

	Pusho(static_cast<uint32_t>(object));
}

void SetHelpSystem() // 253 SET_HELP_SYSTEM
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetVirtualInfluence() // 254 SET_VIRTUAL_INFLUENCE
{
	// const auto player = Popf();
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetActive() // 255 SET_ACTIVE
{
	// const auto object = Pop().uintVal;
	// const auto active = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ThingValid() // 256 THING_VALID
{
	const auto objId = Pop().uintVal;
	// TODO(Daniels118): is this the right way?
	bool valid = false;
	if (objId != 0)
	{
		auto& registry = Locator::entitiesRegistry::value();
		valid = registry.Valid(static_cast<entt::entity>(objId));
	}
	Pushb(valid);
}

void VortexFadeOut() // 257 VORTEX_FADE_OUT
{
	// const auto vortex = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void RemoveReactionOfType() // 258 REMOVE_REACTION_OF_TYPE
{
	// const auto reaction = Pop().intVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureLearnEverythingExcluding() // 259 CREATURE_LEARN_EVERYTHING_EXCLUDING
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void PlayedPercentage() // 260 PLAYED_PERCENTAGE
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void ObjectCastByObject() // 261 OBJECT_CAST_BY_OBJECT
{
	// const auto caster = Pop().uintVal;
	// const auto spellInstance = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void IsWindMagicAtPos() // 262 IS_WIND_MAGIC_AT_POS
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void CreateMist() // 263 CREATE_MIST
{
	// const auto heightRatio = Popf();
	// const auto transparency = Popf();
	// const auto b = Popf();
	// const auto g = Popf();
	// const auto r = Popf();
	// const auto scale = Popf();
	// const auto pos = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SetMistFade() // 264 SET_MIST_FADE
{
	// const auto duration = Popf();
	// const auto endTransparency = Popf();
	// const auto startTransparency = Popf();
	// const auto endScale = Popf();
	// const auto startScale = Popf();
	// const auto mist = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetObjectFade() // 265 GET_OBJECT_FADE
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void PlayHandDemo() // 266 PLAY_HAND_DEMO
{
	// const auto withoutHandModify = static_cast<bool>(Pop().intVal);
	// const auto withPause = static_cast<bool>(Pop().intVal);
	// const auto string = PopString();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void IsPlayingHandDemo() // 267 IS_PLAYING_HAND_DEMO
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void GetArsePosition() // 268 GET_ARSE_POSITION
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushv(0.0f); // x
	Pushv(0.0f); // y
	Pushv(0.0f); // z
}

void IsLeashedToObject() // 269 IS_LEASHED_TO_OBJECT
{
	// const auto target = Pop().uintVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void GetInteractionMagnitude() // 270 GET_INTERACTION_MAGNITUDE
{
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void IsCreatureAvailable() // 271 IS_CREATURE_AVAILABLE
{
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void CreateHighlight() // 272 CREATE_HIGHLIGHT
{
	// const auto challengeID = Pop().intVal;
	// const auto position = PopVec();
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void GetObjectHeld273() // 273 GET_OBJECT_HELD
{
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void GetActionCount() // 274 GET_ACTION_COUNT
{
	// const auto creature = Pop().uintVal;
	// const auto action = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetObjectLeashType() // 275 GET_OBJECT_LEASH_TYPE
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void SetFocusFollow() // 276 SET_FOCUS_FOLLOW
{
	// const auto target = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetPositionFollow() // 277 SET_POSITION_FOLLOW
{
	// const auto target = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetFocusAndPositionFollow() // 278 SET_FOCUS_AND_POSITION_FOLLOW
{
	// const auto distance = Popf();
	// const auto target = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetCameraLens() // 279 SET_CAMERA_LENS
{
	// const auto lens = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void MoveCameraLens() // 280 MOVE_CAMERA_LENS
{
	// const auto time = Popf();
	// const auto lens = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureReaction() // 281 CREATURE_REACTION
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureInDevScript() // 282 CREATURE_IN_DEV_SCRIPT
{
	// const auto creature = Pop().uintVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StoreCameraDetails() // 283 STORE_CAMERA_DETAILS
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void RestoreCameraDetails() // 284 RESTORE_CAMERA_DETAILS
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StartAngleSound285() // 285 START_ANGLE_SOUND
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetCameraPosFocLens() // 286 SET_CAMERA_POS_FOC_LENS
{
	// const auto unk6 = Pop().intVal;
	// const auto unk5 = Pop().intVal;
	// const auto unk4 = Pop().intVal;
	// const auto unk3 = Pop().intVal;
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void MoveCameraPosFocLens() // 287 MOVE_CAMERA_POS_FOC_LENS
{
	// const auto unk7 = Pop().intVal;
	// const auto unk6 = Pop().intVal;
	// const auto unk5 = Pop().intVal;
	// const auto unk4 = Pop().intVal;
	// const auto unk3 = Pop().intVal;
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GameTimeOnOff() // 288 GAME_TIME_ON_OFF
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void MoveGameTime() // 289 MOVE_GAME_TIME
{
	// const auto duration = Popf();
	// const auto hourOfTheDay = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetHighGraphicsDetail() // 290 SET_HIGH_GRAPHICS_DETAIL
{
	// const auto object = Pop().uintVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetSkeleton() // 291 SET_SKELETON
{
	// const auto object = Pop().uintVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void IsSkeleton() // 292 IS_SKELETON
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void PlayerSpellCastTime() // 293 PLAYER_SPELL_CAST_TIME
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void PlayerSpellLastCast() // 294 PLAYER_SPELL_LAST_CAST
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void GetLastSpellCastPos() // 295 GET_LAST_SPELL_CAST_POS
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushv(0.0f); // x
	Pushv(0.0f); // y
	Pushv(0.0f); // z
}

void AddSpotVisualTargetPos() // 296 ADD_SPOT_VISUAL_TARGET_POS
{
	// const auto position = PopVec();
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void AddSpotVisualTargetObject() // 297 ADD_SPOT_VISUAL_TARGET_OBJECT
{
	// const auto target = Pop().uintVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetIndestructable() // 298 SET_INDESTRUCTABLE
{
	// const auto object = Pop().uintVal;
	// const auto indestructible = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetGraphicsClipping() // 299 SET_GRAPHICS_CLIPPING
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SpiritAppear() // 300 SPIRIT_APPEAR
{
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SpiritDisappear() // 301 SPIRIT_DISAPPEAR
{
	// const auto spirit = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetFocusOnObject() // 302 SET_FOCUS_ON_OBJECT
{
	// const auto target = Pop().uintVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ReleaseObjectFocus() // 303 RELEASE_OBJECT_FOCUS
{
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ImmersionExists() // 304 IMMERSION_EXISTS
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SetDrawLeash() // 305 SET_DRAW_LEASH
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetDrawHighlight() // 306 SET_DRAW_HIGHLIGHT
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetOpenClose() // 307 SET_OPEN_CLOSE
{
	// const auto object = Pop().uintVal;
	// const auto open = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetIntroBuilding() // 308 SET_INTRO_BUILDING
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureForceFriends() // 309 CREATURE_FORCE_FRIENDS
{
	// const auto targetCreature = Pop().uintVal;
	// const auto creature = Pop().uintVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void MoveComputerPlayerPosition() // 310 MOVE_COMPUTER_PLAYER_POSITION
{
	// const auto withFixedHeight = static_cast<bool>(Pop().intVal);
	// const auto speed = Popf();
	// const auto position = PopVec();
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void EnableDisableComputerPlayer311() // 311 ENABLE_DISABLE_COMPUTER_PLAYER
{
	// const auto player = Popf();
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetComputerPlayerPosition() // 312 GET_COMPUTER_PLAYER_POSITION
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushv(0.0f); // x
	Pushv(0.0f); // y
	Pushv(0.0f); // z
}

void SetComputerPlayerPosition() // 313 SET_COMPUTER_PLAYER_POSITION
{
	// const auto withFixedHeight = static_cast<bool>(Pop().intVal);
	// const auto position = PopVec();
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetStoredCameraPosition() // 314 GET_STORED_CAMERA_POSITION
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushv(0.0f); // x
	Pushv(0.0f); // y
	Pushv(0.0f); // z
}

void GetStoredCameraFocus() // 315 GET_STORED_CAMERA_FOCUS
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushv(0.0f); // x
	Pushv(0.0f); // y
	Pushv(0.0f); // z
}

void CallNearInState() // 316 CALL_NEAR_IN_STATE
{
	// const auto excludingScripted = static_cast<bool>(Pop().intVal);
	// const auto radius = Popf();
	// const auto position = PopVec();
	// const auto state = Pop().intVal;
	// const auto subtype = Pop().intVal;
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SetCreatureSound() // 317 SET_CREATURE_SOUND
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CreatureInteractingWith() // 318 CREATURE_INTERACTING_WITH
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SetSunDraw() // 319 SET_SUN_DRAW
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ObjectInfoBits() // 320 OBJECT_INFO_BITS
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void SetHurtByFire() // 321 SET_HURT_BY_FIRE
{
	// const auto object = Pop().uintVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ConfinedObject() // 322 CONFINED_OBJECT
{
	// const auto unk4 = Pop().intVal;
	// const auto unk3 = Pop().intVal;
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ClearConfinedObject() // 323 CLEAR_CONFINED_OBJECT
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetObjectFlock() // 324 GET_OBJECT_FLOCK
{
	// const auto member = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SetPlayerBelief() // 325 SET_PLAYER_BELIEF
{
	// const auto belief = Popf();
	// const auto player = Popf();
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void PlayJcSpecial() // 326 PLAY_JC_SPECIAL
{
	// const auto feature = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void IsPlayingJcSpecial() // 327 IS_PLAYING_JC_SPECIAL
{
	// const auto feature = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void VortexParameters() // 328 VORTEX_PARAMETERS
{
	// const auto flock = Pop().uintVal;
	// const auto radius = Popf();
	// const auto distance = Popf();
	// const auto position = PopVec();
	// const auto town = Pop().uintVal;
	// const auto vortex = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void LoadCreature() // 329 LOAD_CREATURE
{
	// const auto position = PopVec();
	// const auto player = Popf();
	// const auto mindFilename = PopString();
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void IsSpellCharging() // 330 IS_SPELL_CHARGING
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void IsThatSpellCharging() // 331 IS_THAT_SPELL_CHARGING
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void OpposingCreature() // 332 OPPOSING_CREATURE
{
	// const auto god = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void FlockWithinLimits() // 333 FLOCK_WITHIN_LIMITS
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void HighlightProperties() // 334 HIGHLIGHT_PROPERTIES
{
	// const auto category = Pop().intVal;
	// const auto text = Pop().intVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void LastMusicLine() // 335 LAST_MUSIC_LINE
{
	// const auto line = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void HandDemoTrigger() // 336 HAND_DEMO_TRIGGER
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void GetBellyPosition() // 337 GET_BELLY_POSITION
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushv(0.0f); // x
	Pushv(0.0f); // y
	Pushv(0.0f); // z
}

void SetCreatureCreedProperties() // 338 SET_CREATURE_CREED_PROPERTIES
{
	// const auto time = Popf();
	// const auto power = Popf();
	// const auto scale = Popf();
	// const auto handGlow = Pop().intVal;
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GameThingCanViewCamera() // 339 GAME_THING_CAN_VIEW_CAMERA
{
	// const auto degrees = Popf();
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void GamePlaySaySoundEffect() // 340 GAME_PLAY_SAY_SOUND_EFFECT
{
	// const auto withPosition = static_cast<bool>(Pop().intVal);
	// const auto position = PopVec();
	// const auto sound = Pop().intVal;
	// const auto extra = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetTownDesireBoost() // 341 SET_TOWN_DESIRE_BOOST
{
	// const auto boost = Popf();
	// const auto desire = Pop().intVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void IsLockedInteraction() // 342 IS_LOCKED_INTERACTION
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SetCreatureName() // 343 SET_CREATURE_NAME
{
	// const auto textID = Pop().intVal;
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ComputerPlayerReady() // 344 COMPUTER_PLAYER_READY
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void EnableDisableComputerPlayer345() // 345 ENABLE_DISABLE_COMPUTER_PLAYER
{
	// const auto player = Popf();
	// const auto pause = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ClearActorMind() // 346 CLEAR_ACTOR_MIND
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void EnterExitCitadel() // 347 ENTER_EXIT_CITADEL
{
	const auto enter = static_cast<bool>(Pop().intVal);
	if (Locator::temple::has_value())
	{
		auto& temple = Locator::temple::value();
		const auto active = temple.Active();
		if (enter != active)
		{
			if (enter)
			{
				temple.Activate();
			}
			else
			{
				temple.Deactivate();
			}
		}
	}
	else
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "No temple");
	}
}

void StartAngleSound348() // 348 START_ANGLE_SOUND
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ThingJcSpecial() // 349 THING_JC_SPECIAL
{
	// const auto target = Pop().uintVal;
	// const auto feature = Pop().intVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void MusicPlayed350() // 350 MUSIC_PLAYED
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void UpdateSnapshotPicture() // 351 UPDATE_SNAPSHOT_PICTURE
{
	// const auto challengeID = Pop().intVal;
	// const auto takingPicture = static_cast<bool>(Pop().intVal);
	// const auto titleStrID = Pop().intVal;
	// const auto alignment = Popf();
	// const auto success = Popf();
	// const auto focus = PopVec();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StopScriptsInFilesExcluding() // 352 STOP_SCRIPTS_IN_FILES_EXCLUDING
{
	const auto scriptNames = PopString();
	const auto sourceFilenames = PopString();

	const auto names = GetUniqueWords(scriptNames);
	const auto filenames = GetUniqueWords(sourceFilenames);
	auto& lhvm = Locator::vm::value();
	lhvm.StopScripts([&names, &filenames](const std::string& name, const std::string& filename) -> bool {
		return filenames.contains(filename) && !names.contains(name);
	});
}

void CreateRandomVillagerOfTribe() // 353 CREATE_RANDOM_VILLAGER_OF_TRIBE
{
	// const auto position = PopVec();
	// const auto tribe = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void ToggleLeash() // 354 TOGGLE_LEASH
{
	// const auto player = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GameSetMana() // 355 GAME_SET_MANA
{
	// const auto mana = Popf();
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetMagicProperties() // 356 SET_MAGIC_PROPERTIES
{
	// const auto duration = Popf();
	// const auto magicType = Pop().intVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetGameSound() // 357 SET_GAME_SOUND
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SexIsMale() // 358 SEX_IS_MALE
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void GetFirstHelp() // 359 GET_FIRST_HELP
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetLastHelp() // 360 GET_LAST_HELP
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void IsActive() // 361 IS_ACTIVE
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SetBookmarkPosition() // 362 SET_BOOKMARK_POSITION
{
	// const auto unk3 = Pop().intVal;
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetScaffoldProperties() // 363 SET_SCAFFOLD_PROPERTIES
{
	// const auto destroy = static_cast<bool>(Pop().intVal);
	// const auto size = Popf();
	// const auto type = Pop().intVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetComputerPlayerPersonality() // 364 SET_COMPUTER_PLAYER_PERSONALITY
{
	// const auto probability = Popf();
	// const auto aspect = PopString();
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetComputerPlayerSuppression() // 365 SET_COMPUTER_PLAYER_SUPPRESSION
{
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ForceComputerPlayerAction() // 366 FORCE_COMPUTER_PLAYER_ACTION
{
	// const auto obj2 = Pop().uintVal;
	// const auto obj1 = Pop().uintVal;
	// const auto action = PopString();
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void QueueComputerPlayerAction() // 367 QUEUE_COMPUTER_PLAYER_ACTION
{
	// const auto obj2 = Pop().uintVal;
	// const auto obj1 = Pop().uintVal;
	// const auto action = PopString();
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetTownWithId() // 368 GET_TOWN_WITH_ID
{
	// const auto id = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SetDisciple() // 369 SET_DISCIPLE
{
	// const auto withSound = static_cast<bool>(Pop().intVal);
	// const auto discipleType = Pop().intVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ReleaseComputerPlayer() // 370 RELEASE_COMPUTER_PLAYER
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetComputerPlayerSpeed() // 371 SET_COMPUTER_PLAYER_SPEED
{
	// const auto speed = Popf();
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetFocusFollowComputerPlayer() // 372 SET_FOCUS_FOLLOW_COMPUTER_PLAYER
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetPositionFollowComputerPlayer() // 373 SET_POSITION_FOLLOW_COMPUTER_PLAYER
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CallComputerPlayer() // 374 CALL_COMPUTER_PLAYER
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void CallBuildingInTown() // 375 CALL_BUILDING_IN_TOWN
{
	// const auto unk3 = Pop().intVal;
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void SetCanBuildWorshipsite() // 376 SET_CAN_BUILD_WORSHIPSITE
{
	// const auto object = Pop().uintVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetFacingCameraPosition() // 377 GET_FACING_CAMERA_POSITION
{
	// const auto distance = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushv(0.0f); // x
	Pushv(0.0f); // y
	Pushv(0.0f); // z
}

void SetComputerPlayerAttitude() // 378 SET_COMPUTER_PLAYER_ATTITUDE
{
	// const auto attitude = Popf();
	// const auto player2 = Popf();
	// const auto player1 = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetComputerPlayerAttitude() // 379 GET_COMPUTER_PLAYER_ATTITUDE
{
	// const auto player2 = Popf();
	// const auto player1 = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void LoadComputerPlayerPersonality() // 380 LOAD_COMPUTER_PLAYER_PERSONALITY
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SaveComputerPlayerPersonality() // 381 SAVE_COMPUTER_PLAYER_PERSONALITY
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetPlayerAlly() // 382 SET_PLAYER_ALLY
{
	// const auto percentage = Popf();
	// const auto player2 = Popf();
	// const auto player1 = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CallFlying() // 383 CALL_FLYING
{
	// const auto excluding = static_cast<bool>(Pop().intVal);
	// const auto radius = Popf();
	// const auto position = PopVec();
	// const auto subtype = Pop().intVal;
	// const auto type = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SetObjectFadeIn() // 384 SET_OBJECT_FADE_IN
{
	// const auto time = Popf();
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void IsAffectedBySpell() // 385 IS_AFFECTED_BY_SPELL
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SetMagicInObject() // 386 SET_MAGIC_IN_OBJECT
{
	// const auto object = Pop().uintVal;
	// const auto MAGIC_TYPE = Pop().intVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void IdAdultSize() // 387 ID_ADULT_SIZE
{
	// const auto container = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void ObjectCapacity() // 388 OBJECT_CAPACITY
{
	// const auto container = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void ObjectAdultCapacity() // 389 OBJECT_ADULT_CAPACITY
{
	// const auto container = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void SetCreatureAutoFighting() // 390 SET_CREATURE_AUTO_FIGHTING
{
	// const auto creature = Pop().uintVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void IsAutoFighting() // 391 IS_AUTO_FIGHTING
{
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SetCreatureQueueFightMove() // 392 SET_CREATURE_QUEUE_FIGHT_MOVE
{
	// const auto move = Pop().intVal;
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetCreatureQueueFightSpell() // 393 SET_CREATURE_QUEUE_FIGHT_SPELL
{
	// const auto spell = Pop().intVal;
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetCreatureQueueFightStep() // 394 SET_CREATURE_QUEUE_FIGHT_STEP
{
	// const auto step = Pop().intVal;
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetCreatureFightAction() // 395 GET_CREATURE_FIGHT_ACTION
{
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void CreatureFightQueueHits() // 396 CREATURE_FIGHT_QUEUE_HITS
{
	// const auto creature = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void SquareRoot() // 397 SQUARE_ROOT
{
	const auto value = Popf();
	auto root = 0.0f;
	if (value > 0.0f)
	{
		root = std::sqrt(value);
	}
	Pushf(root);
}

void GetPlayerAlly() // 398 GET_PLAYER_ALLY
{
	// const auto player2 = Popf();
	// const auto player1 = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void SetPlayerWindResistance() // 399 SET_PLAYER_WIND_RESISTANCE
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void GetPlayerWindResistance() // 400 GET_PLAYER_WIND_RESISTANCE
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void PauseUnpauseClimateSystem() // 401 PAUSE_UNPAUSE_CLIMATE_SYSTEM
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void PauseUnpauseStormCreationInClimateSystem() // 402 PAUSE_UNPAUSE_STORM_CREATION_IN_CLIMATE_SYSTEM
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetManaForSpell() // 403 GET_MANA_FOR_SPELL
{
	// const auto spell = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void KillStormsInArea() // 404 KILL_STORMS_IN_AREA
{
	// const auto radius = Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void InsideTemple() // 405 INSIDE_TEMPLE
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void RestartObject() // 406 RESTART_OBJECT
{
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetGameTimeProperties() // 407 SET_GAME_TIME_PROPERTIES
{
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ResetGameTimeProperties() // 408 RESET_GAME_TIME_PROPERTIES
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SoundExists() // 409 SOUND_EXISTS
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void GetTownWorshipDeaths() // 410 GET_TOWN_WORSHIP_DEATHS
{
	// const auto town = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GameClearDialogue() // 411 GAME_CLEAR_DIALOGUE
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GameCloseDialogue() // 412 GAME_CLOSE_DIALOGUE
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetHandState() // 413 GET_HAND_STATE
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushi(0);
}

void SetInterfaceCitadel() // 414 SET_INTERFACE_CITADEL
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void MapScriptFunction() // 415 MAP_SCRIPT_FUNCTION
{
	// const auto command = PopString();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void WithinRotation() // 416 WITHIN_ROTATION
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void GetPlayerTownTotal() // 417 GET_PLAYER_TOWN_TOTAL
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void SpiritScreenPoint() // 418 SPIRIT_SCREEN_POINT
{
	// const auto unk2 = Pop().intVal;
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void KeyDown() // 419 KEY_DOWN
{
	// const auto key = Pop().intVal;
	// TODO(Daniels118): implement this (translate key to physical key code)
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SetFightExit() // 420 SET_FIGHT_EXIT
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetObjectClicked() // 421 GET_OBJECT_CLICKED
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void GetMana() // 422 GET_MANA
{
	// const auto worshipSite = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void ClearPlayerSpellCharging() // 423 CLEAR_PLAYER_SPELL_CHARGING
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StopSoundEffect() // 424 STOP_SOUND_EFFECT
{
	// const auto soundbank = Pop().intVal;
	// const auto sound = Pop().intVal;
	// const auto alwaysFalse = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetTotemStatue() // 425 GET_TOTEM_STATUE
{
	// const auto town = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void SetSetOnFire() // 426 SET_SET_ON_FIRE
{
	// const auto object = Pop().uintVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetLandBalance() // 427 SET_LAND_BALANCE
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetObjectBeliefScale() // 428 SET_OBJECT_BELIEF_SCALE
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StartImmersion() // 429 START_IMMERSION
{
	// const auto effect = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StopImmersion() // 430 STOP_IMMERSION
{
	// const auto effect = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void StopAllImmersion() // 431 STOP_ALL_IMMERSION
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetCreatureInTemple() // 432 SET_CREATURE_IN_TEMPLE
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GameDrawText() // 433 GAME_DRAW_TEXT
{
	// const auto fade = Popf();
	// const auto size = Popf();
	// const auto height = Popf();
	// const auto width = Popf();
	// const auto down = Popf();
	// const auto across = Popf();
	// const auto textID = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GameDrawTempText() // 434 GAME_DRAW_TEMP_TEXT
{
	// const auto fade = Popf();
	// const auto size = Popf();
	// const auto height = Popf();
	// const auto width = Popf();
	// const auto down = Popf();
	// const auto across = Popf();
	// const auto string = PopString();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void FadeAllDrawText() // 435 FADE_ALL_DRAW_TEXT
{
	// const auto time = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetDrawTextColour() // 436 SET_DRAW_TEXT_COLOUR
{
	// const auto blue = Popf();
	// const auto green = Popf();
	// const auto red = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetClippingWindow() // 437 SET_CLIPPING_WINDOW
{
	// const auto time = Popf();
	// const auto height = Popf();
	// const auto width = Popf();
	// const auto down = Popf();
	// const auto across = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void ClearClippingWindow() // 438 CLEAR_CLIPPING_WINDOW
{
	// const auto time = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SaveGameInSlot() // 439 SAVE_GAME_IN_SLOT
{
	// const auto slot = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void SetObjectCarrying() // 440 SET_OBJECT_CARRYING
{
	// const auto carriedObj = Pop().intVal;
	// const auto object = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void PosValidForCreature() // 441 POS_VALID_FOR_CREATURE
{
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void GetTimeSinceObjectAttacked() // 442 GET_TIME_SINCE_OBJECT_ATTACKED
{
	// const auto town = Pop().uintVal;
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GetTownAndVillagerHealthTotal() // 443 GET_TOWN_AND_VILLAGER_HEALTH_TOTAL
{
	// const auto town = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GameAddForBuilding() // 444 GAME_ADD_FOR_BUILDING
{
	// const auto unk1 = Pop().intVal;
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void EnableDisableAlignmentMusic() // 445 ENABLE_DISABLE_ALIGNMENT_MUSIC
{
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetDeadLiving() // 446 GET_DEAD_LIVING
{
	// const auto radius = Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void AttachSoundTag() // 447 ATTACH_SOUND_TAG
{
	// const auto target = Pop().uintVal;
	// const auto soundbank = Pop().intVal;
	// const auto sound = Pop().intVal;
	// const auto threeD = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void DetachSoundTag() // 448 DETACH_SOUND_TAG
{
	// const auto target = Pop().uintVal;
	// const auto soundbank = Pop().intVal;
	// const auto sound = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetSacrificeTotal() // 449 GET_SACRIFICE_TOTAL
{
	// const auto worshipSite = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushf(0.0f);
}

void GameSoundPlaying() // 450 GAME_SOUND_PLAYING
{
	// const auto soundbank = Pop().intVal;
	// const auto sound = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void GetTemplePosition() // 451 GET_TEMPLE_POSITION
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushv(0.0f); // x
	Pushv(0.0f); // y
	Pushv(0.0f); // z
}

void CreatureAutoscale() // 452 CREATURE_AUTOSCALE
{
	// const auto size = Popf();
	// const auto creature = Pop().uintVal;
	// const auto enable = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetSpellIconInTemple() // 453 GET_SPELL_ICON_IN_TEMPLE
{
	// const auto temple = Pop().uintVal;
	// const auto spell = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void GameClearComputerPlayerActions() // 454 GAME_CLEAR_COMPUTER_PLAYER_ACTIONS
{
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void GetFirstInContainer() // 455 GET_FIRST_IN_CONTAINER
{
	// const auto container = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void GetNextInContainer() // 456 GET_NEXT_IN_CONTAINER
{
	// const auto after = Pop().uintVal;
	// const auto container = Pop().uintVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pusho(0);
}

void GetTempleEntrancePosition() // 457 GET_TEMPLE_ENTRANCE_POSITION
{
	// const auto height = Popf();
	// const auto radius = Popf();
	// const auto player = Popf();
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushv(0.0f); // x
	Pushv(0.0f); // y
	Pushv(0.0f); // z
}

void SaySoundEffectPlaying() // 458 SAY_SOUND_EFFECT_PLAYING
{
	// const auto sound = Pop().intVal;
	// const auto alwaysFalse = static_cast<bool>(Pop().intVal);
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void SetHandDemoKeys() // 459 SET_HAND_DEMO_KEYS
{
	// const auto unk0 = Pop().intVal;
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
}

void CanSkipTutorial() // 460 CAN_SKIP_TUTORIAL
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void CanSkipCreatureTraining() // 461 CAN_SKIP_CREATURE_TRAINING
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void IsKeepingOldCreature() // 462 IS_KEEPING_OLD_CREATURE
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void CurrentProfileHasCreature() // 463 CURRENT_PROFILE_HAS_CREATURE
{
	// TODO(Daniels118): implement this
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "CHLApi Function {}() not implemented.", __func__);
	Pushb(false);
}

void CHLApi::InitFunctionsTable0()
{
	CREATE_FUNCTION_BINDING("NONE", 0, 0, None);
	CREATE_FUNCTION_BINDING("SET_CAMERA_POSITION", 3, 0, SetCameraPosition);
	CREATE_FUNCTION_BINDING("SET_CAMERA_FOCUS", 3, 0, SetCameraFocus);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_POSITION", 4, 0, MoveCameraPosition);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_FOCUS", 4, 0, MoveCameraFocus);
	CREATE_FUNCTION_BINDING("GET_CAMERA_POSITION", 0, 3, GetCameraPosition);
	CREATE_FUNCTION_BINDING("GET_CAMERA_FOCUS", 0, 3, GetCameraFocus);
	CREATE_FUNCTION_BINDING("SPIRIT_EJECT", 1, 0, SpiritEject);
	CREATE_FUNCTION_BINDING("SPIRIT_HOME", 1, 0, SpiritHome);
	CREATE_FUNCTION_BINDING("SPIRIT_POINT_POS", 5, 0, SpiritPointPos);
	CREATE_FUNCTION_BINDING("SPIRIT_POINT_GAME_THING", 3, 0, SpiritPointGameThing);
	CREATE_FUNCTION_BINDING("GAME_THING_FIELD_OF_VIEW", 1, 1, GameThingFieldOfView);
	CREATE_FUNCTION_BINDING("POS_FIELD_OF_VIEW", 3, 1, PosFieldOfView);
	CREATE_FUNCTION_BINDING("RUN_TEXT", 3, 0, RunText);
	CREATE_FUNCTION_BINDING("TEMP_TEXT", 3, 0, TempText);
	CREATE_FUNCTION_BINDING("TEXT_READ", 0, 1, TextRead);
	CREATE_FUNCTION_BINDING("GAME_THING_CLICKED", 1, 1, GameThingClicked);
	CREATE_FUNCTION_BINDING("SET_SCRIPT_STATE", 2, 0, SetScriptState);
	CREATE_FUNCTION_BINDING("SET_SCRIPT_STATE_POS", 4, 0, SetScriptStatePos);
	CREATE_FUNCTION_BINDING("SET_SCRIPT_FLOAT", 2, 0, SetScriptFloat);
	CREATE_FUNCTION_BINDING("SET_SCRIPT_ULONG", 3, 0, SetScriptUlong);
	CREATE_FUNCTION_BINDING("GET_PROPERTY", 2, 1, GetProperty);
	CREATE_FUNCTION_BINDING("SET_PROPERTY", 3, 0, SetProperty);
	CREATE_FUNCTION_BINDING("GET_POSITION", 1, 3, GetPosition);
	CREATE_FUNCTION_BINDING("SET_POSITION", 4, 0, SetPosition);
	CREATE_FUNCTION_BINDING("GET_DISTANCE", 6, 1, GetDistance);
	CREATE_FUNCTION_BINDING("CALL", 6, 1, Call);
	CREATE_FUNCTION_BINDING("CREATE", 5, 1, Create);
	CREATE_FUNCTION_BINDING("RANDOM", 2, 1, Random);
	CREATE_FUNCTION_BINDING("DLL_GETTIME", 0, 1, DllGettime);
	CREATE_FUNCTION_BINDING("START_CAMERA_CONTROL", 0, 1, StartCameraControl);
	CREATE_FUNCTION_BINDING("END_CAMERA_CONTROL", 0, 0, EndCameraControl);
	CREATE_FUNCTION_BINDING("SET_WIDESCREEN", 1, 0, SetWidescreen);
	CREATE_FUNCTION_BINDING("MOVE_GAME_THING", 5, 0, MoveGameThing);
	CREATE_FUNCTION_BINDING("SET_FOCUS", 4, 0, SetFocus);
	CREATE_FUNCTION_BINDING("HAS_CAMERA_ARRIVED", 0, 1, HasCameraArrived);
	CREATE_FUNCTION_BINDING("FLOCK_CREATE", 3, 1, FlockCreate);
	CREATE_FUNCTION_BINDING("FLOCK_ATTACH", 3, 1, FlockAttach);
	CREATE_FUNCTION_BINDING("FLOCK_DETACH", 2, 1, FlockDetach);
	CREATE_FUNCTION_BINDING("FLOCK_DISBAND", 1, 0, FlockDisband);
	CREATE_FUNCTION_BINDING("ID_SIZE", 1, 1, IdSize);
	CREATE_FUNCTION_BINDING("FLOCK_MEMBER", 2, 1, FlockMember);
	CREATE_FUNCTION_BINDING("GET_HAND_POSITION", 0, 3, GetHandPosition);
	CREATE_FUNCTION_BINDING("PLAY_SOUND_EFFECT", 6, 0, PlaySoundEffect);
	CREATE_FUNCTION_BINDING("START_MUSIC", 1, 0, StartMusic);
	CREATE_FUNCTION_BINDING("STOP_MUSIC", 0, 0, StopMusic);
	CREATE_FUNCTION_BINDING("ATTACH_MUSIC", 2, 0, AttachMusic);
	CREATE_FUNCTION_BINDING("DETACH_MUSIC", 1, 0, DetachMusic);
	CREATE_FUNCTION_BINDING("OBJECT_DELETE", 2, 0, ObjectDelete);
	CREATE_FUNCTION_BINDING("FOCUS_FOLLOW", 1, 0, FocusFollow);
	CREATE_FUNCTION_BINDING("POSITION_FOLLOW", 1, 0, PositionFollow);
	CREATE_FUNCTION_BINDING("CALL_NEAR", 7, 1, CallNear);
	CREATE_FUNCTION_BINDING("SPECIAL_EFFECT_POSITION", 5, 1, SpecialEffectPosition);
	CREATE_FUNCTION_BINDING("SPECIAL_EFFECT_OBJECT", 3, 1, SpecialEffectObject);
	CREATE_FUNCTION_BINDING("DANCE_CREATE", 6, 1, DanceCreate);
	CREATE_FUNCTION_BINDING("CALL_IN", 4, 1, CallIn);
	CREATE_FUNCTION_BINDING("CHANGE_INNER_OUTER_PROPERTIES", 4, 0, ChangeInnerOuterProperties);
	CREATE_FUNCTION_BINDING("SNAPSHOT", -1, 0, Snapshot);
	CREATE_FUNCTION_BINDING("GET_ALIGNMENT", 1, 1, GetAlignment);
	CREATE_FUNCTION_BINDING("SET_ALIGNMENT", 2, 0, SetAlignment);
	CREATE_FUNCTION_BINDING("INFLUENCE_OBJECT", 4, 1, InfluenceObject);
	CREATE_FUNCTION_BINDING("INFLUENCE_POSITION", 6, 1, InfluencePosition);
	CREATE_FUNCTION_BINDING("GET_INFLUENCE", 5, 1, GetInfluence);
	CREATE_FUNCTION_BINDING("SET_INTERFACE_INTERACTION", 1, 0, SetInterfaceInteraction);
	CREATE_FUNCTION_BINDING("PLAYED", 1, 1, Played);
	CREATE_FUNCTION_BINDING("RANDOM_ULONG", 2, 1, RandomUlong);
	CREATE_FUNCTION_BINDING("SET_GAMESPEED", 1, 0, SetGamespeed);
	CREATE_FUNCTION_BINDING("CALL_IN_NEAR", 8, 1, CallInNear);
	CREATE_FUNCTION_BINDING("OVERRIDE_STATE_ANIMATION", 2, 0, OverrideStateAnimation);
	CREATE_FUNCTION_BINDING("CREATURE_CREATE_RELATIVE_TO_CREATURE", 6, 1, CreatureCreateRelativeToCreature);
	CREATE_FUNCTION_BINDING("CREATURE_LEARN_EVERYTHING", 1, 0, CreatureLearnEverything);
	CREATE_FUNCTION_BINDING("CREATURE_SET_KNOWS_ACTION", 4, 0, CreatureSetKnowsAction);
	CREATE_FUNCTION_BINDING("CREATURE_SET_AGENDA_PRIORITY", 2, 0, CreatureSetAgendaPriority);
	CREATE_FUNCTION_BINDING("CREATURE_TURN_OFF_ALL_DESIRES", 1, 0, CreatureTurnOffAllDesires);
	CREATE_FUNCTION_BINDING("CREATURE_LEARN_DISTINCTION_ABOUT_ACTIVITY_OBJECT", 4, 0,
	                        CreatureLearnDistinctionAboutActivityObject);
	CREATE_FUNCTION_BINDING("CREATURE_DO_ACTION", 4, 0, CreatureDoAction);
	CREATE_FUNCTION_BINDING("IN_CREATURE_HAND", 2, 1, InCreatureHand);
	CREATE_FUNCTION_BINDING("CREATURE_SET_DESIRE_VALUE", 3, 0, CreatureSetDesireValue);
	CREATE_FUNCTION_BINDING("CREATURE_SET_DESIRE_ACTIVATED", 3, 0, CreatureSetDesireActivated78);
	CREATE_FUNCTION_BINDING("CREATURE_SET_DESIRE_ACTIVATED", 2, 0, CreatureSetDesireActivated79);
	CREATE_FUNCTION_BINDING("CREATURE_SET_DESIRE_MAXIMUM", 3, 0, CreatureSetDesireMaximum);
	CREATE_FUNCTION_BINDING("CONVERT_CAMERA_POSITION", 1, 3, ConvertCameraPosition);
	CREATE_FUNCTION_BINDING("CONVERT_CAMERA_FOCUS", 1, 3, ConvertCameraFocus);
	CREATE_FUNCTION_BINDING("CREATURE_SET_PLAYER", 1, 0, CreatureSetPlayer);
	CREATE_FUNCTION_BINDING("START_COUNTDOWN_TIMER", 1, 0, StartCountdownTimer);
	CREATE_FUNCTION_BINDING("CREATURE_INITIALISE_NUM_TIMES_PERFORMED_ACTION", 2, 0, CreatureInitialiseNumTimesPerformedAction);
	CREATE_FUNCTION_BINDING("CREATURE_GET_NUM_TIMES_ACTION_PERFORMED", 2, 1, CreatureGetNumTimesActionPerformed);
	CREATE_FUNCTION_BINDING("REMOVE_COUNTDOWN_TIMER", 0, 0, RemoveCountdownTimer);
	CREATE_FUNCTION_BINDING("GET_OBJECT_DROPPED", 1, 1, GetObjectDropped);
	CREATE_FUNCTION_BINDING("CLEAR_DROPPED_BY_OBJECT", 1, 0, ClearDroppedByObject);
	CREATE_FUNCTION_BINDING("CREATE_REACTION", 2, 0, CreateReaction);
	CREATE_FUNCTION_BINDING("REMOVE_REACTION", 1, 0, RemoveReaction);
	CREATE_FUNCTION_BINDING("GET_COUNTDOWN_TIMER", 0, 1, GetCountdownTimer);
	CREATE_FUNCTION_BINDING("START_DUAL_CAMERA", 2, 0, StartDualCamera);
	CREATE_FUNCTION_BINDING("UPDATE_DUAL_CAMERA", 2, 0, UpdateDualCamera);
	CREATE_FUNCTION_BINDING("RELEASE_DUAL_CAMERA", 0, 0, ReleaseDualCamera);
	CREATE_FUNCTION_BINDING("SET_CREATURE_HELP", 1, 0, SetCreatureHelp);
	CREATE_FUNCTION_BINDING("GET_TARGET_OBJECT", 1, 1, GetTargetObject);
	CREATE_FUNCTION_BINDING("CREATURE_DESIRE_IS", 2, 1, CreatureDesireIs);
	CREATE_FUNCTION_BINDING("COUNTDOWN_TIMER_EXISTS", 0, 1, CountdownTimerExists);
	CREATE_FUNCTION_BINDING("LOOK_GAME_THING", 2, 0, LookGameThing);
	CREATE_FUNCTION_BINDING("GET_OBJECT_DESTINATION", 1, 3, GetObjectDestination);
	CREATE_FUNCTION_BINDING("CREATURE_FORCE_FINISH", 1, 0, CreatureForceFinish);
	CREATE_FUNCTION_BINDING("HIDE_COUNTDOWN_TIMER", 0, 0, HideCountdownTimer);
	CREATE_FUNCTION_BINDING("GET_ACTION_TEXT_FOR_OBJECT", 1, 1, GetActionTextForObject);
	CREATE_FUNCTION_BINDING("CREATE_DUAL_CAMERA_WITH_POINT", 4, 0, CreateDualCameraWithPoint);
	CREATE_FUNCTION_BINDING("SET_CAMERA_TO_FACE_OBJECT", 2, 0, SetCameraToFaceObject);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_TO_FACE_OBJECT", 3, 0, MoveCameraToFaceObject);
}

void CHLApi::InitFunctionsTable1()
{
	CREATE_FUNCTION_BINDING("GET_MOON_PERCENTAGE", 0, 1, GetMoonPercentage);
	CREATE_FUNCTION_BINDING("POPULATE_CONTAINER", 4, 0, PopulateContainer);
	CREATE_FUNCTION_BINDING("ADD_REFERENCE", 1, 1, AddReference);
	CREATE_FUNCTION_BINDING("REMOVE_REFERENCE", 1, 1, RemoveReference);
	CREATE_FUNCTION_BINDING("SET_GAME_TIME", 1, 0, SetGameTime);
	CREATE_FUNCTION_BINDING("GET_GAME_TIME", 0, 1, GetGameTime);
	CREATE_FUNCTION_BINDING("GET_REAL_TIME", 0, 1, GetRealTime);
	CREATE_FUNCTION_BINDING("GET_REAL_DAY", 0, 1, GetRealDay115);
	CREATE_FUNCTION_BINDING("GET_REAL_DAY", 0, 1, GetRealDay116);
	CREATE_FUNCTION_BINDING("GET_REAL_MONTH", 0, 1, GetRealMonth);
	CREATE_FUNCTION_BINDING("GET_REAL_YEAR", 0, 1, GetRealYear);
	CREATE_FUNCTION_BINDING("RUN_CAMERA_PATH", 1, 0, RunCameraPath);
	CREATE_FUNCTION_BINDING("START_DIALOGUE", 0, 1, StartDialogue);
	CREATE_FUNCTION_BINDING("END_DIALOGUE", 0, 0, EndDialogue);
	CREATE_FUNCTION_BINDING("IS_DIALOGUE_READY", 0, 1, IsDialogueReady);
	CREATE_FUNCTION_BINDING("CHANGE_WEATHER_PROPERTIES", 6, 0, ChangeWeatherProperties);
	CREATE_FUNCTION_BINDING("CHANGE_LIGHTNING_PROPERTIES", 5, 0, ChangeLightningProperties);
	CREATE_FUNCTION_BINDING("CHANGE_TIME_FADE_PROPERTIES", 3, 0, ChangeTimeFadeProperties);
	CREATE_FUNCTION_BINDING("CHANGE_CLOUD_PROPERTIES", 4, 0, ChangeCloudProperties);
	CREATE_FUNCTION_BINDING("SET_HEADING_AND_SPEED", 5, 0, SetHeadingAndSpeed);
	CREATE_FUNCTION_BINDING("START_GAME_SPEED", 0, 0, StartGameSpeed);
	CREATE_FUNCTION_BINDING("END_GAME_SPEED", 0, 0, EndGameSpeed);
	CREATE_FUNCTION_BINDING("BUILD_BUILDING", 4, 0, BuildBuilding);
	CREATE_FUNCTION_BINDING("SET_AFFECTED_BY_WIND", 2, 0, SetAffectedByWind);
	CREATE_FUNCTION_BINDING("WIDESCREEN_TRANSISTION_FINISHED", 0, 1, WidescreenTransistionFinished);
	CREATE_FUNCTION_BINDING("GET_RESOURCE", 2, 1, GetResource);
	CREATE_FUNCTION_BINDING("ADD_RESOURCE", 3, 1, AddResource);
	CREATE_FUNCTION_BINDING("REMOVE_RESOURCE", 3, 1, RemoveResource);
	CREATE_FUNCTION_BINDING("GET_TARGET_RELATIVE_POS", 8, 3, GetTargetRelativePos);
	CREATE_FUNCTION_BINDING("STOP_POINTING", 1, 0, StopPointing);
	CREATE_FUNCTION_BINDING("STOP_LOOKING", 1, 0, StopLooking);
	CREATE_FUNCTION_BINDING("LOOK_AT_POSITION", 4, 0, LookAtPosition);
	CREATE_FUNCTION_BINDING("PLAY_SPIRIT_ANIM", 5, 0, PlaySpiritAnim);
	CREATE_FUNCTION_BINDING("CALL_IN_NOT_NEAR", 8, 1, CallInNotNear);
	CREATE_FUNCTION_BINDING("SET_CAMERA_ZONE", 1, 0, SetCameraZone);
	CREATE_FUNCTION_BINDING("GET_OBJECT_STATE", 1, 1, GetObjectState);
	CREATE_FUNCTION_BINDING("REVEAL_COUNTDOWN_TIMER", 0, 0, RevealCountdownTimer);
	CREATE_FUNCTION_BINDING("SET_TIMER_TIME", 2, 0, SetTimerTime);
	CREATE_FUNCTION_BINDING("CREATE_TIMER", 1, 1, CreateTimer);
	CREATE_FUNCTION_BINDING("GET_TIMER_TIME_REMAINING", 1, 1, GetTimerTimeRemaining);
	CREATE_FUNCTION_BINDING("GET_TIMER_TIME_SINCE_SET", 1, 1, GetTimerTimeSinceSet);
	CREATE_FUNCTION_BINDING("MOVE_MUSIC", 2, 0, MoveMusic);
	CREATE_FUNCTION_BINDING("GET_INCLUSION_DISTANCE", 0, 1, GetInclusionDistance);
	CREATE_FUNCTION_BINDING("GET_LAND_HEIGHT", 3, 1, GetLandHeight);
	CREATE_FUNCTION_BINDING("LOAD_MAP", 1, 0, LoadMap);
	CREATE_FUNCTION_BINDING("STOP_ALL_SCRIPTS_EXCLUDING", 1, 0, StopAllScriptsExcluding);
	CREATE_FUNCTION_BINDING("STOP_ALL_SCRIPTS_IN_FILES_EXCLUDING", 1, 0, StopAllScriptsInFilesExcluding);
	CREATE_FUNCTION_BINDING("STOP_SCRIPT", 1, 0, StopScript);
	CREATE_FUNCTION_BINDING("CLEAR_CLICKED_OBJECT", 0, 0, ClearClickedObject);
	CREATE_FUNCTION_BINDING("CLEAR_CLICKED_POSITION", 0, 0, ClearClickedPosition);
	CREATE_FUNCTION_BINDING("POSITION_CLICKED", 4, 1, PositionClicked);
	CREATE_FUNCTION_BINDING("RELEASE_FROM_SCRIPT", 1, 0, ReleaseFromScript);
	CREATE_FUNCTION_BINDING("GET_OBJECT_HAND_IS_OVER", 0, 1, GetObjectHandIsOver);
	CREATE_FUNCTION_BINDING("ID_POISONED_SIZE", 1, 1, IdPoisonedSize);
	CREATE_FUNCTION_BINDING("IS_POISONED", 1, 1, IsPoisoned);
	CREATE_FUNCTION_BINDING("CALL_POISONED_IN", 4, 1, CallPoisonedIn);
	CREATE_FUNCTION_BINDING("CALL_NOT_POISONED_IN", 4, 1, CallNotPoisonedIn);
	CREATE_FUNCTION_BINDING("SPIRIT_PLAYED", 1, 1, SpiritPlayed);
	CREATE_FUNCTION_BINDING("CLING_SPIRIT", 3, 0, ClingSpirit);
	CREATE_FUNCTION_BINDING("FLY_SPIRIT", 3, 0, FlySpirit);
	CREATE_FUNCTION_BINDING("SET_ID_MOVEABLE", 2, 0, SetIdMoveable);
	CREATE_FUNCTION_BINDING("SET_ID_PICKUPABLE", 2, 0, SetIdPickupable);
	CREATE_FUNCTION_BINDING("IS_ON_FIRE", 1, 1, IsOnFire);
	CREATE_FUNCTION_BINDING("IS_FIRE_NEAR", 4, 1, IsFireNear);
	CREATE_FUNCTION_BINDING("STOP_SCRIPTS_IN_FILES", 1, 0, StopScriptsInFiles);
	CREATE_FUNCTION_BINDING("SET_POISONED", 2, 0, SetPoisoned);
	CREATE_FUNCTION_BINDING("SET_TEMPERATURE", 2, 0, SetTemperature);
	CREATE_FUNCTION_BINDING("SET_ON_FIRE", 3, 0, SetOnFire);
	CREATE_FUNCTION_BINDING("SET_TARGET", 5, 0, SetTarget);
	CREATE_FUNCTION_BINDING("WALK_PATH", 5, 0, WalkPath);
	CREATE_FUNCTION_BINDING("FOCUS_AND_POSITION_FOLLOW", 2, 0, FocusAndPositionFollow);
	CREATE_FUNCTION_BINDING("GET_WALK_PATH_PERCENTAGE", 1, 1, GetWalkPathPercentage);
	CREATE_FUNCTION_BINDING("CAMERA_PROPERTIES", 4, 0, CameraProperties);
	CREATE_FUNCTION_BINDING("ENABLE_DISABLE_MUSIC", 2, 0, EnableDisableMusic);
	CREATE_FUNCTION_BINDING("GET_MUSIC_OBJ_DISTANCE", 1, 1, GetMusicObjDistance);
	CREATE_FUNCTION_BINDING("GET_MUSIC_ENUM_DISTANCE", 1, 1, GetMusicEnumDistance);
	CREATE_FUNCTION_BINDING("SET_MUSIC_PLAY_POSITION", 4, 0, SetMusicPlayPosition);
	CREATE_FUNCTION_BINDING("ATTACH_OBJECT_LEASH_TO_OBJECT", 2, 0, AttachObjectLeashToObject);
	CREATE_FUNCTION_BINDING("ATTACH_OBJECT_LEASH_TO_HAND", 1, 0, AttachObjectLeashToHand);
	CREATE_FUNCTION_BINDING("DETACH_OBJECT_LEASH", 1, 0, DetachObjectLeash);
	CREATE_FUNCTION_BINDING("SET_CREATURE_ONLY_DESIRE", 3, 0, SetCreatureOnlyDesire);
	CREATE_FUNCTION_BINDING("SET_CREATURE_ONLY_DESIRE_OFF", 1, 0, SetCreatureOnlyDesireOff);
	CREATE_FUNCTION_BINDING("RESTART_MUSIC", 1, 0, RestartMusic);
	CREATE_FUNCTION_BINDING("MUSIC_PLAYED", 1, 1, MusicPlayed191);
	CREATE_FUNCTION_BINDING("IS_OF_TYPE", 3, 1, IsOfType);
	CREATE_FUNCTION_BINDING("CLEAR_HIT_OBJECT", 0, 0, ClearHitObject);
	CREATE_FUNCTION_BINDING("GAME_THING_HIT", 1, 1, GameThingHit);
	CREATE_FUNCTION_BINDING("SPELL_AT_THING", 8, 1, SpellAtThing);
	CREATE_FUNCTION_BINDING("SPELL_AT_POS", 10, 1, SpellAtPos);
	CREATE_FUNCTION_BINDING("CALL_PLAYER_CREATURE", 1, 1, CallPlayerCreature);
	CREATE_FUNCTION_BINDING("GET_SLOWEST_SPEED", 1, 1, GetSlowestSpeed);
	CREATE_FUNCTION_BINDING("GET_OBJECT_HELD", 0, 1, GetObjectHeld199);
}

void CHLApi::InitFunctionsTable2()
{
	CREATE_FUNCTION_BINDING("HELP_SYSTEM_ON", 0, 1, HelpSystemOn);
	CREATE_FUNCTION_BINDING("SHAKE_CAMERA", 6, 0, ShakeCamera);
	CREATE_FUNCTION_BINDING("SET_ANIMATION_MODIFY", 2, 0, SetAnimationModify);
	CREATE_FUNCTION_BINDING("SET_AVI_SEQUENCE", 2, 0, SetAviSequence);
	CREATE_FUNCTION_BINDING("PLAY_GESTURE", 5, 0, PlayGesture);
	CREATE_FUNCTION_BINDING("DEV_FUNCTION", 1, 0, DevFunction);
	CREATE_FUNCTION_BINDING("HAS_MOUSE_WHEEL", 0, 1, HasMouseWheel);
	CREATE_FUNCTION_BINDING("NUM_MOUSE_BUTTONS", 0, 1, NumMouseButtons);
	CREATE_FUNCTION_BINDING("SET_CREATURE_DEV_STAGE", 2, 0, SetCreatureDevStage);
	CREATE_FUNCTION_BINDING("SET_FIXED_CAM_ROTATION", 4, 0, SetFixedCamRotation);
	CREATE_FUNCTION_BINDING("SWAP_CREATURE", 2, 0, SwapCreature);
	CREATE_FUNCTION_BINDING("GET_ARENA", 5, 1, GetArena);
	CREATE_FUNCTION_BINDING("GET_FOOTBALL_PITCH", 1, 1, GetFootballPitch);
	CREATE_FUNCTION_BINDING("STOP_ALL_GAMES", 1, 0, StopAllGames);
	CREATE_FUNCTION_BINDING("ATTACH_TO_GAME", 3, 0, AttachToGame);
	CREATE_FUNCTION_BINDING("DETACH_FROM_GAME", 3, 0, DetachFromGame);
	CREATE_FUNCTION_BINDING("DETACH_UNDEFINED_FROM_GAME", 2, 0, DetachUndefinedFromGame);
	CREATE_FUNCTION_BINDING("SET_ONLY_FOR_SCRIPTS", 2, 0, SetOnlyForScripts);
	CREATE_FUNCTION_BINDING("START_MATCH_WITH_REFEREE", 2, 0, StartMatchWithReferee);
	CREATE_FUNCTION_BINDING("GAME_TEAM_SIZE", 2, 0, GameTeamSize);
	CREATE_FUNCTION_BINDING("GAME_TYPE", 1, 1, GameType);
	CREATE_FUNCTION_BINDING("GAME_SUB_TYPE", 1, 1, GameSubType);
	CREATE_FUNCTION_BINDING("IS_LEASHED", 1, 1, IsLeashed);
	CREATE_FUNCTION_BINDING("SET_CREATURE_HOME", 4, 0, SetCreatureHome);
	CREATE_FUNCTION_BINDING("GET_HIT_OBJECT", 0, 1, GetHitObject);
	CREATE_FUNCTION_BINDING("GET_OBJECT_WHICH_HIT", 0, 1, GetObjectWhichHit);
	CREATE_FUNCTION_BINDING("GET_NEAREST_TOWN_OF_PLAYER", 5, 1, GetNearestTownOfPlayer);
	CREATE_FUNCTION_BINDING("SPELL_AT_POINT", 5, 1, SpellAtPoint);
	CREATE_FUNCTION_BINDING("SET_ATTACK_OWN_TOWN", 2, 0, SetAttackOwnTown);
	CREATE_FUNCTION_BINDING("IS_FIGHTING", 1, 1, IsFighting);
	CREATE_FUNCTION_BINDING("SET_MAGIC_RADIUS", 2, 0, SetMagicRadius);
	CREATE_FUNCTION_BINDING("TEMP_TEXT_WITH_NUMBER", 4, 0, TempTextWithNumber);
	CREATE_FUNCTION_BINDING("RUN_TEXT_WITH_NUMBER", 4, 0, RunTextWithNumber);
	CREATE_FUNCTION_BINDING("CREATURE_SPELL_REVERSION", 2, 0, CreatureSpellReversion);
	CREATE_FUNCTION_BINDING("GET_DESIRE", 2, 1, GetDesire);
	CREATE_FUNCTION_BINDING("GET_EVENTS_PER_SECOND", 1, 1, GetEventsPerSecond);
	CREATE_FUNCTION_BINDING("GET_TIME_SINCE", 1, 1, GetTimeSince);
	CREATE_FUNCTION_BINDING("GET_TOTAL_EVENTS", 1, 1, GetTotalEvents);
	CREATE_FUNCTION_BINDING("UPDATE_SNAPSHOT", -1, 0, UpdateSnapshot);
	CREATE_FUNCTION_BINDING("CREATE_REWARD", 5, 1, CreateReward);
	CREATE_FUNCTION_BINDING("CREATE_REWARD_IN_TOWN", 6, 1, CreateRewardInTown);
	CREATE_FUNCTION_BINDING("SET_FADE", 4, 0, SetFade);
	CREATE_FUNCTION_BINDING("SET_FADE_IN", 1, 0, SetFadeIn);
	CREATE_FUNCTION_BINDING("FADE_FINISHED", 0, 1, FadeFinished);
	CREATE_FUNCTION_BINDING("SET_PLAYER_MAGIC", 3, 0, SetPlayerMagic);
	CREATE_FUNCTION_BINDING("HAS_PLAYER_MAGIC", 2, 1, HasPlayerMagic);
	CREATE_FUNCTION_BINDING("SPIRIT_SPEAKS", 2, 1, SpiritSpeaks);
	CREATE_FUNCTION_BINDING("BELIEF_FOR_PLAYER", 2, 1, BeliefForPlayer);
	CREATE_FUNCTION_BINDING("GET_HELP", 1, 1, GetHelp);
	CREATE_FUNCTION_BINDING("SET_LEASH_WORKS", 2, 0, SetLeashWorks);
	CREATE_FUNCTION_BINDING("LOAD_MY_CREATURE", 3, 0, LoadMyCreature);
	CREATE_FUNCTION_BINDING("OBJECT_RELATIVE_BELIEF", 3, 0, ObjectRelativeBelief);
	CREATE_FUNCTION_BINDING("CREATE_WITH_ANGLE_AND_SCALE", 7, 1, CreateWithAngleAndScale);
	CREATE_FUNCTION_BINDING("SET_HELP_SYSTEM", 1, 0, SetHelpSystem);
	CREATE_FUNCTION_BINDING("SET_VIRTUAL_INFLUENCE", 2, 0, SetVirtualInfluence);
	CREATE_FUNCTION_BINDING("SET_ACTIVE", 2, 0, SetActive);
	CREATE_FUNCTION_BINDING("THING_VALID", 1, 1, ThingValid);
	CREATE_FUNCTION_BINDING("VORTEX_FADE_OUT", 1, 0, VortexFadeOut);
	CREATE_FUNCTION_BINDING("REMOVE_REACTION_OF_TYPE", 2, 0, RemoveReactionOfType);
	CREATE_FUNCTION_BINDING("CREATURE_LEARN_EVERYTHING_EXCLUDING", 2, 0, CreatureLearnEverythingExcluding);
	CREATE_FUNCTION_BINDING("PLAYED_PERCENTAGE", 1, 1, PlayedPercentage);
	CREATE_FUNCTION_BINDING("OBJECT_CAST_BY_OBJECT", 2, 1, ObjectCastByObject);
	CREATE_FUNCTION_BINDING("IS_WIND_MAGIC_AT_POS", 1, 1, IsWindMagicAtPos);
	CREATE_FUNCTION_BINDING("CREATE_MIST", 9, 1, CreateMist);
	CREATE_FUNCTION_BINDING("SET_MIST_FADE", 6, 0, SetMistFade);
	CREATE_FUNCTION_BINDING("GET_OBJECT_FADE", 1, 1, GetObjectFade);
	CREATE_FUNCTION_BINDING("PLAY_HAND_DEMO", 3, 0, PlayHandDemo);
	CREATE_FUNCTION_BINDING("IS_PLAYING_HAND_DEMO", 0, 1, IsPlayingHandDemo);
	CREATE_FUNCTION_BINDING("GET_ARSE_POSITION", 1, 3, GetArsePosition);
	CREATE_FUNCTION_BINDING("IS_LEASHED_TO_OBJECT", 2, 1, IsLeashedToObject);
	CREATE_FUNCTION_BINDING("GET_INTERACTION_MAGNITUDE", 1, 1, GetInteractionMagnitude);
	CREATE_FUNCTION_BINDING("IS_CREATURE_AVAILABLE", 1, 1, IsCreatureAvailable);
	CREATE_FUNCTION_BINDING("CREATE_HIGHLIGHT", 5, 1, CreateHighlight);
	CREATE_FUNCTION_BINDING("GET_OBJECT_HELD", 1, 1, GetObjectHeld273);
	CREATE_FUNCTION_BINDING("GET_ACTION_COUNT", 2, 1, GetActionCount);
	CREATE_FUNCTION_BINDING("GET_OBJECT_LEASH_TYPE", 1, 1, GetObjectLeashType);
	CREATE_FUNCTION_BINDING("SET_FOCUS_FOLLOW", 1, 0, SetFocusFollow);
	CREATE_FUNCTION_BINDING("SET_POSITION_FOLLOW", 1, 0, SetPositionFollow);
	CREATE_FUNCTION_BINDING("SET_FOCUS_AND_POSITION_FOLLOW", 2, 0, SetFocusAndPositionFollow);
	CREATE_FUNCTION_BINDING("SET_CAMERA_LENS", 1, 0, SetCameraLens);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_LENS", 2, 0, MoveCameraLens);
	CREATE_FUNCTION_BINDING("CREATURE_REACTION", 2, 0, CreatureReaction);
	CREATE_FUNCTION_BINDING("CREATURE_IN_DEV_SCRIPT", 2, 0, CreatureInDevScript);
	CREATE_FUNCTION_BINDING("STORE_CAMERA_DETAILS", 0, 0, StoreCameraDetails);
	CREATE_FUNCTION_BINDING("RESTORE_CAMERA_DETAILS", 0, 0, RestoreCameraDetails);
	CREATE_FUNCTION_BINDING("START_ANGLE_SOUND", 1, 0, StartAngleSound285);
	CREATE_FUNCTION_BINDING("SET_CAMERA_POS_FOC_LENS", 7, 0, SetCameraPosFocLens);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_POS_FOC_LENS", 8, 0, MoveCameraPosFocLens);
	CREATE_FUNCTION_BINDING("GAME_TIME_ON_OFF", 1, 0, GameTimeOnOff);
	CREATE_FUNCTION_BINDING("MOVE_GAME_TIME", 2, 0, MoveGameTime);
	CREATE_FUNCTION_BINDING("SET_HIGH_GRAPHICS_DETAIL", 2, 0, SetHighGraphicsDetail);
	CREATE_FUNCTION_BINDING("SET_SKELETON", 2, 0, SetSkeleton);
	CREATE_FUNCTION_BINDING("IS_SKELETON", 1, 1, IsSkeleton);
	CREATE_FUNCTION_BINDING("PLAYER_SPELL_CAST_TIME", 1, 1, PlayerSpellCastTime);
	CREATE_FUNCTION_BINDING("PLAYER_SPELL_LAST_CAST", 1, 1, PlayerSpellLastCast);
	CREATE_FUNCTION_BINDING("GET_LAST_SPELL_CAST_POS", 1, 3, GetLastSpellCastPos);
	CREATE_FUNCTION_BINDING("ADD_SPOT_VISUAL_TARGET_POS", 4, 0, AddSpotVisualTargetPos);
	CREATE_FUNCTION_BINDING("ADD_SPOT_VISUAL_TARGET_OBJECT", 2, 0, AddSpotVisualTargetObject);
	CREATE_FUNCTION_BINDING("SET_INDESTRUCTABLE", 2, 0, SetIndestructable);
	CREATE_FUNCTION_BINDING("SET_GRAPHICS_CLIPPING", 2, 0, SetGraphicsClipping);
}

void CHLApi::InitFunctionsTable3()
{
	CREATE_FUNCTION_BINDING("SPIRIT_APPEAR", 1, 0, SpiritAppear);
	CREATE_FUNCTION_BINDING("SPIRIT_DISAPPEAR", 1, 0, SpiritDisappear);
	CREATE_FUNCTION_BINDING("SET_FOCUS_ON_OBJECT", 2, 0, SetFocusOnObject);
	CREATE_FUNCTION_BINDING("RELEASE_OBJECT_FOCUS", 1, 0, ReleaseObjectFocus);
	CREATE_FUNCTION_BINDING("IMMERSION_EXISTS", 0, 1, ImmersionExists);
	CREATE_FUNCTION_BINDING("SET_DRAW_LEASH", 1, 0, SetDrawLeash);
	CREATE_FUNCTION_BINDING("SET_DRAW_HIGHLIGHT", 1, 0, SetDrawHighlight);
	CREATE_FUNCTION_BINDING("SET_OPEN_CLOSE", 2, 0, SetOpenClose);
	CREATE_FUNCTION_BINDING("SET_INTRO_BUILDING", 1, 0, SetIntroBuilding);
	CREATE_FUNCTION_BINDING("CREATURE_FORCE_FRIENDS", 3, 0, CreatureForceFriends);
	CREATE_FUNCTION_BINDING("MOVE_COMPUTER_PLAYER_POSITION", 6, 0, MoveComputerPlayerPosition);
	CREATE_FUNCTION_BINDING("ENABLE_DISABLE_COMPUTER_PLAYER", 2, 0, EnableDisableComputerPlayer311);
	CREATE_FUNCTION_BINDING("GET_COMPUTER_PLAYER_POSITION", 1, 3, GetComputerPlayerPosition);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_POSITION", 5, 0, SetComputerPlayerPosition);
	CREATE_FUNCTION_BINDING("GET_STORED_CAMERA_POSITION", 0, 3, GetStoredCameraPosition);
	CREATE_FUNCTION_BINDING("GET_STORED_CAMERA_FOCUS", 0, 3, GetStoredCameraFocus);
	CREATE_FUNCTION_BINDING("CALL_NEAR_IN_STATE", 8, 1, CallNearInState);
	CREATE_FUNCTION_BINDING("SET_CREATURE_SOUND", 1, 0, SetCreatureSound);
	CREATE_FUNCTION_BINDING("CREATURE_INTERACTING_WITH", 2, 1, CreatureInteractingWith);
	CREATE_FUNCTION_BINDING("SET_SUN_DRAW", 1, 0, SetSunDraw);
	CREATE_FUNCTION_BINDING("OBJECT_INFO_BITS", 1, 1, ObjectInfoBits);
	CREATE_FUNCTION_BINDING("SET_HURT_BY_FIRE", 2, 0, SetHurtByFire);
	CREATE_FUNCTION_BINDING("CONFINED_OBJECT", 5, 0, ConfinedObject);
	CREATE_FUNCTION_BINDING("CLEAR_CONFINED_OBJECT", 1, 0, ClearConfinedObject);
	CREATE_FUNCTION_BINDING("GET_OBJECT_FLOCK", 1, 1, GetObjectFlock);
	CREATE_FUNCTION_BINDING("SET_PLAYER_BELIEF", 3, 0, SetPlayerBelief);
	CREATE_FUNCTION_BINDING("PLAY_JC_SPECIAL", 1, 0, PlayJcSpecial);
	CREATE_FUNCTION_BINDING("IS_PLAYING_JC_SPECIAL", 1, 1, IsPlayingJcSpecial);
	CREATE_FUNCTION_BINDING("VORTEX_PARAMETERS", 8, 0, VortexParameters);
	CREATE_FUNCTION_BINDING("LOAD_CREATURE", 6, 0, LoadCreature);
	CREATE_FUNCTION_BINDING("IS_SPELL_CHARGING", 1, 1, IsSpellCharging);
	CREATE_FUNCTION_BINDING("IS_THAT_SPELL_CHARGING", 2, 1, IsThatSpellCharging);
	CREATE_FUNCTION_BINDING("OPPOSING_CREATURE", 1, 1, OpposingCreature);
	CREATE_FUNCTION_BINDING("FLOCK_WITHIN_LIMITS", 1, 1, FlockWithinLimits);
	CREATE_FUNCTION_BINDING("HIGHLIGHT_PROPERTIES", 3, 0, HighlightProperties);
	CREATE_FUNCTION_BINDING("LAST_MUSIC_LINE", 1, 1, LastMusicLine);
	CREATE_FUNCTION_BINDING("HAND_DEMO_TRIGGER", 0, 1, HandDemoTrigger);
	CREATE_FUNCTION_BINDING("GET_BELLY_POSITION", 1, 3, GetBellyPosition);
	CREATE_FUNCTION_BINDING("SET_CREATURE_CREED_PROPERTIES", 5, 0, SetCreatureCreedProperties);
	CREATE_FUNCTION_BINDING("GAME_THING_CAN_VIEW_CAMERA", 2, 1, GameThingCanViewCamera);
	CREATE_FUNCTION_BINDING("GAME_PLAY_SAY_SOUND_EFFECT", 6, 0, GamePlaySaySoundEffect);
	CREATE_FUNCTION_BINDING("SET_TOWN_DESIRE_BOOST", 3, 0, SetTownDesireBoost);
	CREATE_FUNCTION_BINDING("IS_LOCKED_INTERACTION", 1, 1, IsLockedInteraction);
	CREATE_FUNCTION_BINDING("SET_CREATURE_NAME", 2, 0, SetCreatureName);
	CREATE_FUNCTION_BINDING("COMPUTER_PLAYER_READY", 1, 1, ComputerPlayerReady);
	CREATE_FUNCTION_BINDING("ENABLE_DISABLE_COMPUTER_PLAYER", 2, 0, EnableDisableComputerPlayer345);
	CREATE_FUNCTION_BINDING("CLEAR_ACTOR_MIND", 1, 0, ClearActorMind);
	CREATE_FUNCTION_BINDING("ENTER_EXIT_CITADEL", 1, 0, EnterExitCitadel);
	CREATE_FUNCTION_BINDING("START_ANGLE_SOUND", 1, 0, StartAngleSound348);
	CREATE_FUNCTION_BINDING("THING_JC_SPECIAL", 3, 0, ThingJcSpecial);
	CREATE_FUNCTION_BINDING("MUSIC_PLAYED", 1, 1, MusicPlayed350);
	CREATE_FUNCTION_BINDING("UPDATE_SNAPSHOT_PICTURE", 11, 0, UpdateSnapshotPicture);
	CREATE_FUNCTION_BINDING("STOP_SCRIPTS_IN_FILES_EXCLUDING", 2, 0, StopScriptsInFilesExcluding);
	CREATE_FUNCTION_BINDING("CREATE_RANDOM_VILLAGER_OF_TRIBE", 4, 1, CreateRandomVillagerOfTribe);
	CREATE_FUNCTION_BINDING("TOGGLE_LEASH", 1, 0, ToggleLeash);
	CREATE_FUNCTION_BINDING("GAME_SET_MANA", 2, 0, GameSetMana);
	CREATE_FUNCTION_BINDING("SET_MAGIC_PROPERTIES", 3, 0, SetMagicProperties);
	CREATE_FUNCTION_BINDING("SET_GAME_SOUND", 1, 0, SetGameSound);
	CREATE_FUNCTION_BINDING("SEX_IS_MALE", 1, 1, SexIsMale);
	CREATE_FUNCTION_BINDING("GET_FIRST_HELP", 1, 1, GetFirstHelp);
	CREATE_FUNCTION_BINDING("GET_LAST_HELP", 1, 1, GetLastHelp);
	CREATE_FUNCTION_BINDING("IS_ACTIVE", 1, 1, IsActive);
	CREATE_FUNCTION_BINDING("SET_BOOKMARK_POSITION", 4, 0, SetBookmarkPosition);
	CREATE_FUNCTION_BINDING("SET_SCAFFOLD_PROPERTIES", 4, 0, SetScaffoldProperties);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_PERSONALITY", 3, 0, SetComputerPlayerPersonality);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_SUPPRESSION", 3, 0, SetComputerPlayerSuppression);
	CREATE_FUNCTION_BINDING("FORCE_COMPUTER_PLAYER_ACTION", 4, 0, ForceComputerPlayerAction);
	CREATE_FUNCTION_BINDING("QUEUE_COMPUTER_PLAYER_ACTION", 4, 0, QueueComputerPlayerAction);
	CREATE_FUNCTION_BINDING("GET_TOWN_WITH_ID", 1, 1, GetTownWithId);
	CREATE_FUNCTION_BINDING("SET_DISCIPLE", 3, 0, SetDisciple);
	CREATE_FUNCTION_BINDING("RELEASE_COMPUTER_PLAYER", 1, 0, ReleaseComputerPlayer);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_SPEED", 2, 0, SetComputerPlayerSpeed);
	CREATE_FUNCTION_BINDING("SET_FOCUS_FOLLOW_COMPUTER_PLAYER", 1, 0, SetFocusFollowComputerPlayer);
	CREATE_FUNCTION_BINDING("SET_POSITION_FOLLOW_COMPUTER_PLAYER", 1, 0, SetPositionFollowComputerPlayer);
	CREATE_FUNCTION_BINDING("CALL_COMPUTER_PLAYER", 1, 1, CallComputerPlayer);
	CREATE_FUNCTION_BINDING("CALL_BUILDING_IN_TOWN", 4, 1, CallBuildingInTown);
	CREATE_FUNCTION_BINDING("SET_CAN_BUILD_WORSHIPSITE", 2, 0, SetCanBuildWorshipsite);
	CREATE_FUNCTION_BINDING("GET_FACING_CAMERA_POSITION", 1, 3, GetFacingCameraPosition);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_ATTITUDE", 3, 0, SetComputerPlayerAttitude);
	CREATE_FUNCTION_BINDING("GET_COMPUTER_PLAYER_ATTITUDE", 2, 1, GetComputerPlayerAttitude);
	CREATE_FUNCTION_BINDING("LOAD_COMPUTER_PLAYER_PERSONALITY", 2, 0, LoadComputerPlayerPersonality);
	CREATE_FUNCTION_BINDING("SAVE_COMPUTER_PLAYER_PERSONALITY", 2, 0, SaveComputerPlayerPersonality);
	CREATE_FUNCTION_BINDING("SET_PLAYER_ALLY", 3, 0, SetPlayerAlly);
	CREATE_FUNCTION_BINDING("CALL_FLYING", 7, 1, CallFlying);
	CREATE_FUNCTION_BINDING("SET_OBJECT_FADE_IN", 2, 0, SetObjectFadeIn);
	CREATE_FUNCTION_BINDING("IS_AFFECTED_BY_SPELL", 1, 1, IsAffectedBySpell);
	CREATE_FUNCTION_BINDING("SET_MAGIC_IN_OBJECT", 3, 0, SetMagicInObject);
	CREATE_FUNCTION_BINDING("ID_ADULT_SIZE", 1, 1, IdAdultSize);
	CREATE_FUNCTION_BINDING("OBJECT_CAPACITY", 1, 1, ObjectCapacity);
	CREATE_FUNCTION_BINDING("OBJECT_ADULT_CAPACITY", 1, 1, ObjectAdultCapacity);
	CREATE_FUNCTION_BINDING("SET_CREATURE_AUTO_FIGHTING", 2, 0, SetCreatureAutoFighting);
	CREATE_FUNCTION_BINDING("IS_AUTO_FIGHTING", 1, 1, IsAutoFighting);
	CREATE_FUNCTION_BINDING("SET_CREATURE_QUEUE_FIGHT_MOVE", 2, 0, SetCreatureQueueFightMove);
	CREATE_FUNCTION_BINDING("SET_CREATURE_QUEUE_FIGHT_SPELL", 2, 0, SetCreatureQueueFightSpell);
	CREATE_FUNCTION_BINDING("SET_CREATURE_QUEUE_FIGHT_STEP", 2, 0, SetCreatureQueueFightStep);
	CREATE_FUNCTION_BINDING("GET_CREATURE_FIGHT_ACTION", 1, 1, GetCreatureFightAction);
	CREATE_FUNCTION_BINDING("CREATURE_FIGHT_QUEUE_HITS", 1, 1, CreatureFightQueueHits);
	CREATE_FUNCTION_BINDING("SQUARE_ROOT", 1, 1, SquareRoot);
	CREATE_FUNCTION_BINDING("GET_PLAYER_ALLY", 2, 1, GetPlayerAlly);
	CREATE_FUNCTION_BINDING("SET_PLAYER_WIND_RESISTANCE", 2, 1, SetPlayerWindResistance);
}

void CHLApi::InitFunctionsTable4()
{
	CREATE_FUNCTION_BINDING("GET_PLAYER_WIND_RESISTANCE", 2, 1, GetPlayerWindResistance);
	CREATE_FUNCTION_BINDING("PAUSE_UNPAUSE_CLIMATE_SYSTEM", 1, 0, PauseUnpauseClimateSystem);
	CREATE_FUNCTION_BINDING("PAUSE_UNPAUSE_STORM_CREATION_IN_CLIMATE_SYSTEM", 1, 0, PauseUnpauseStormCreationInClimateSystem);
	CREATE_FUNCTION_BINDING("GET_MANA_FOR_SPELL", 1, 1, GetManaForSpell);
	CREATE_FUNCTION_BINDING("KILL_STORMS_IN_AREA", 4, 0, KillStormsInArea);
	CREATE_FUNCTION_BINDING("INSIDE_TEMPLE", 0, 1, InsideTemple);
	CREATE_FUNCTION_BINDING("RESTART_OBJECT", 1, 0, RestartObject);
	CREATE_FUNCTION_BINDING("SET_GAME_TIME_PROPERTIES", 3, 0, SetGameTimeProperties);
	CREATE_FUNCTION_BINDING("RESET_GAME_TIME_PROPERTIES", 0, 0, ResetGameTimeProperties);
	CREATE_FUNCTION_BINDING("SOUND_EXISTS", 0, 1, SoundExists);
	CREATE_FUNCTION_BINDING("GET_TOWN_WORSHIP_DEATHS", 1, 1, GetTownWorshipDeaths);
	CREATE_FUNCTION_BINDING("GAME_CLEAR_DIALOGUE", 0, 0, GameClearDialogue);
	CREATE_FUNCTION_BINDING("GAME_CLOSE_DIALOGUE", 0, 0, GameCloseDialogue);
	CREATE_FUNCTION_BINDING("GET_HAND_STATE", 0, 1, GetHandState);
	CREATE_FUNCTION_BINDING("SET_INTERFACE_CITADEL", 1, 0, SetInterfaceCitadel);
	CREATE_FUNCTION_BINDING("MAP_SCRIPT_FUNCTION", 1, 0, MapScriptFunction);
	CREATE_FUNCTION_BINDING("WITHIN_ROTATION", 0, 1, WithinRotation);
	CREATE_FUNCTION_BINDING("GET_PLAYER_TOWN_TOTAL", 1, 1, GetPlayerTownTotal);
	CREATE_FUNCTION_BINDING("SPIRIT_SCREEN_POINT", 3, 0, SpiritScreenPoint);
	CREATE_FUNCTION_BINDING("KEY_DOWN", 1, 1, KeyDown);
	CREATE_FUNCTION_BINDING("SET_FIGHT_EXIT", 1, 0, SetFightExit);
	CREATE_FUNCTION_BINDING("GET_OBJECT_CLICKED", 0, 1, GetObjectClicked);
	CREATE_FUNCTION_BINDING("GET_MANA", 1, 1, GetMana);
	CREATE_FUNCTION_BINDING("CLEAR_PLAYER_SPELL_CHARGING", 1, 0, ClearPlayerSpellCharging);
	CREATE_FUNCTION_BINDING("STOP_SOUND_EFFECT", 3, 0, StopSoundEffect);
	CREATE_FUNCTION_BINDING("GET_TOTEM_STATUE", 1, 1, GetTotemStatue);
	CREATE_FUNCTION_BINDING("SET_SET_ON_FIRE", 2, 0, SetSetOnFire);
	CREATE_FUNCTION_BINDING("SET_LAND_BALANCE", 2, 0, SetLandBalance);
	CREATE_FUNCTION_BINDING("SET_OBJECT_BELIEF_SCALE", 2, 0, SetObjectBeliefScale);
	CREATE_FUNCTION_BINDING("START_IMMERSION", 1, 0, StartImmersion);
	CREATE_FUNCTION_BINDING("STOP_IMMERSION", 1, 0, StopImmersion);
	CREATE_FUNCTION_BINDING("STOP_ALL_IMMERSION", 0, 0, StopAllImmersion);
	CREATE_FUNCTION_BINDING("SET_CREATURE_IN_TEMPLE", 1, 0, SetCreatureInTemple);
	CREATE_FUNCTION_BINDING("GAME_DRAW_TEXT", 7, 0, GameDrawText);
	CREATE_FUNCTION_BINDING("GAME_DRAW_TEMP_TEXT", 7, 0, GameDrawTempText);
	CREATE_FUNCTION_BINDING("FADE_ALL_DRAW_TEXT", 1, 0, FadeAllDrawText);
	CREATE_FUNCTION_BINDING("SET_DRAW_TEXT_COLOUR", 3, 0, SetDrawTextColour);
	CREATE_FUNCTION_BINDING("SET_CLIPPING_WINDOW", 5, 0, SetClippingWindow);
	CREATE_FUNCTION_BINDING("CLEAR_CLIPPING_WINDOW", 1, 0, ClearClippingWindow);
	CREATE_FUNCTION_BINDING("SAVE_GAME_IN_SLOT", 1, 0, SaveGameInSlot);
	CREATE_FUNCTION_BINDING("SET_OBJECT_CARRYING", 2, 0, SetObjectCarrying);
	CREATE_FUNCTION_BINDING("POS_VALID_FOR_CREATURE", 3, 1, PosValidForCreature);
	CREATE_FUNCTION_BINDING("GET_TIME_SINCE_OBJECT_ATTACKED", 2, 1, GetTimeSinceObjectAttacked);
	CREATE_FUNCTION_BINDING("GET_TOWN_AND_VILLAGER_HEALTH_TOTAL", 1, 1, GetTownAndVillagerHealthTotal);
	CREATE_FUNCTION_BINDING("GAME_ADD_FOR_BUILDING", 2, 0, GameAddForBuilding);
	CREATE_FUNCTION_BINDING("ENABLE_DISABLE_ALIGNMENT_MUSIC", 1, 0, EnableDisableAlignmentMusic);
	CREATE_FUNCTION_BINDING("GET_DEAD_LIVING", 4, 1, GetDeadLiving);
	CREATE_FUNCTION_BINDING("ATTACH_SOUND_TAG", 4, 0, AttachSoundTag);
	CREATE_FUNCTION_BINDING("DETACH_SOUND_TAG", 3, 0, DetachSoundTag);
	CREATE_FUNCTION_BINDING("GET_SACRIFICE_TOTAL", 1, 1, GetSacrificeTotal);
	CREATE_FUNCTION_BINDING("GAME_SOUND_PLAYING", 2, 1, GameSoundPlaying);
	CREATE_FUNCTION_BINDING("GET_TEMPLE_POSITION", 1, 3, GetTemplePosition);
	CREATE_FUNCTION_BINDING("CREATURE_AUTOSCALE", 3, 0, CreatureAutoscale);
	CREATE_FUNCTION_BINDING("GET_SPELL_ICON_IN_TEMPLE", 2, 1, GetSpellIconInTemple);
	CREATE_FUNCTION_BINDING("GAME_CLEAR_COMPUTER_PLAYER_ACTIONS", 1, 0, GameClearComputerPlayerActions);
	CREATE_FUNCTION_BINDING("GET_FIRST_IN_CONTAINER", 1, 1, GetFirstInContainer);
	CREATE_FUNCTION_BINDING("GET_NEXT_IN_CONTAINER", 2, 1, GetNextInContainer);
	CREATE_FUNCTION_BINDING("GET_TEMPLE_ENTRANCE_POSITION", 3, 3, GetTempleEntrancePosition);
	CREATE_FUNCTION_BINDING("SAY_SOUND_EFFECT_PLAYING", 2, 1, SaySoundEffectPlaying);
	CREATE_FUNCTION_BINDING("SET_HAND_DEMO_KEYS", 1, 0, SetHandDemoKeys);
	CREATE_FUNCTION_BINDING("CAN_SKIP_TUTORIAL", 0, 1, CanSkipTutorial);
	CREATE_FUNCTION_BINDING("CAN_SKIP_CREATURE_TRAINING", 0, 1, CanSkipCreatureTraining);
	CREATE_FUNCTION_BINDING("IS_KEEPING_OLD_CREATURE", 0, 1, IsKeepingOldCreature);
	CREATE_FUNCTION_BINDING("CURRENT_PROFILE_HAS_CREATURE", 0, 1, CurrentProfileHasCreature);
}

} // namespace openblack::chlapi
