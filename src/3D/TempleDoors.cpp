/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TempleDoors.h"

#include <cmath>

#include <algorithm>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Audio/Sound.h"

using namespace openblack;

TempleDoors::TempleDoors(PlaySound playSound)
    : _playSound(std::move(playSound))
{
	_joints.fill(glm::mat4(1.0f));
}

std::optional<uint32_t> TempleDoors::LeafOf(TempleRoom room)
{
	// The main room's mesh numbers the leaves round from the doorway out, skipping the wall of the scrolls
	switch (room)
	{
	case TempleRoom::Options:
		return 3;
	case TempleRoom::CreatureCave:
		return 5;
	case TempleRoom::Challenge:
		return 7;
	case TempleRoom::SaveGame:
		return 9;
	case TempleRoom::Credits:
		return 11;
	case TempleRoom::Multi:
		return 13;
	case TempleRoom::Main:
	case TempleRoom::Unknown:
		break;
	}
	return std::nullopt;
}

void TempleDoors::Open(std::optional<uint32_t> leaf, float from, float rate)
{
	if (_swing > 1.0f || leaf != _leaf)
	{
		_swing = from;
		if (leaf.has_value())
		{
			_leaf = leaf;
		}
	}
	_rate = rate;
}

void TempleDoors::Close(float rate)
{
	if (!_leaf.has_value() || _swing > 1.0f)
	{
		return;
	}
	_playSound(static_cast<entt::id_type>(audio::SoundId::G_CitadelDoorClose_02));
	_swing = 2.0f - _swing;
	_rate = rate;
	if (_swing <= 1.0f)
	{
		_swing += 0.01f;
	}
}

void TempleDoors::FastClose()
{
	if (!_leaf.has_value())
	{
		return;
	}
	_playSound(static_cast<entt::id_type>(audio::SoundId::G_CitadelDoorClose_02));
	_swing = 2.0f;
	_rate = 2.0f;
}

void TempleDoors::Update(float dt)
{
	if (!_leaf.has_value())
	{
		_swing = 0.0f;
	}
	else if (_swing < 1.0f)
	{
		const bool waiting = _swing <= 0.0f;
		_swing += _rate * dt;
		if (waiting && _swing > 0.0f)
		{
			_playSound(static_cast<entt::id_type>(audio::SoundId::G_CitadelDoorOpen_01));
		}
		_swing = std::min(_swing, 1.0f);
	}
	else if (_swing > 1.0f)
	{
		_swing += _rate * dt;
		if (_swing > 2.0f)
		{
			_swing = 0.0f;
			_leaf.reset();
		}
	}
	LayOutJoints();
}

void TempleDoors::LayOutJoints()
{
	// The leaves ease open and back shut
	float turn = 0.0f;
	if (_swing > 0.0f && _swing < 2.0f)
	{
		turn = (1.0f - std::cos(_swing * glm::pi<float>())) * k_HalfSwing;
	}
	_joints.fill(glm::mat4(1.0f));
	if (!_leaf.has_value())
	{
		return;
	}
	// The pair turn opposite ways, so both open outwards
	const glm::vec3 up {0.0f, 1.0f, 0.0f};
	if (*_leaf < k_JointCount)
	{
		_joints[*_leaf] = glm::rotate(glm::mat4(1.0f), turn, up);
	}
	if (*_leaf + 1 < k_JointCount)
	{
		_joints[*_leaf + 1] = glm::rotate(glm::mat4(1.0f), -turn, up);
	}
}
