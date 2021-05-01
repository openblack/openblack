/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <SDL.h>
#include <fstream>
#include "InputSystem.h"

using namespace openblack;


InputSystem::InputSystem()
{
	_mouseMouvements = glm::vec2(0, 0);

	InitKeyType(InputType::CameraForward, "CameraForward", SDLK_w);
	InitKeyType(InputType::CameraBackward, "CameraBackward", SDLK_s);
	InitKeyType(InputType::CameraLeft, "CameraLeft", SDLK_a);
	InitKeyType(InputType::CameraRight, "CameraRight", SDLK_d);
	InitKeyType(InputType::CameraDown, "CameraDown", SDLK_LCTRL);
	InitKeyType(InputType::CameraUp, "CameraUp", SDLK_SPACE);
	InitKeyType(InputType::CameraFreeLook, "CameraFreeLook", SDLK_UNKNOWN);
	InitKeyType(InputType::CameraGrab, "CameraGrab", SDLK_UNKNOWN);
}

void InputSystem::LoadKeyMap(const std::string& path)
{
	std::ifstream keyMapFile(path);
	if (!keyMapFile.good())
		throw std::invalid_argument("Invalid path to keymap file");

	std::string line;
	while (std::getline(keyMapFile, line))
	{
		std::istringstream is_line(line);
		std::string key;
		if (!std::getline(is_line, key, '='))
			continue;

		std::string value;
		if (!std::getline(is_line, value))
			continue;

		auto inputType = GetInputTypeFromName(key);
		auto sdlKeyCode = SDL_GetKeyFromName(value.c_str());
		_keyMap[inputType] = sdlKeyCode;
	}
	keyMapFile.close();
}

void InputSystem::ProcessSDLEvent(const SDL_Event& e)
{
	ProcessSDLKeyboardEvent(e);
	ProcessSDLMouseEvent(e);
}

void InputSystem::ProcessSDLKeyboardEvent(const SDL_Event& e)
{
	if (e.type != SDL_KEYDOWN && e.type != SDL_KEYUP)
		return;

	// ignore all repeated keys
	if (e.key.repeat > 0)
		return;

	// Iterate over each registered input type and update the state map
	for (auto inputTypeMapping : _keyMap)
	{
		if (e.key.keysym.sym != inputTypeMapping.second)
			continue;

		_inputTypeState[inputTypeMapping.first] = e.type == SDL_KEYDOWN;
	}
}

void InputSystem::ProcessSDLMouseEvent(const SDL_Event& e) {
	_inputTypeState[InputType::CameraFreeLook] = false;
	_inputTypeState[InputType::CameraGrab] = false;

	//Hardcode freelook and grab for now
	if (e.type != SDL_MOUSEMOTION)
		return;

	_mouseMouvements.x += e.motion.xrel;
	_mouseMouvements.y += e.motion.yrel;

	_inputTypeState[InputType::CameraFreeLook] = (bool)(e.motion.state & SDL_BUTTON(SDL_BUTTON_MIDDLE));
	_inputTypeState[InputType::CameraGrab] = (bool)(e.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT));
}

void InputSystem::ClearState()
{
	_mouseMouvements = glm::vec2(0, 0);
}

glm::vec2 InputSystem::GetMouseMouvements() const
{
	return _mouseMouvements;
}

bool InputSystem::IsInputDown(InputType type) const
{
	auto it = _inputTypeState.find(type);
	if (it == _inputTypeState.end())
		return false;

	return it->second;
}

InputType InputSystem::GetInputTypeFromName(std::string name)
{
	auto it = _nameMap.find(name);
	if (it == _nameMap.end())
		throw std::invalid_argument("Invalid InputType Name");

	return it->second;
}

void InputSystem::InitKeyType(InputType inputType, std::string keyMapName, SDL_Keycode defaultKeyCode)
{
	_nameMap[keyMapName] = inputType;
	_keyMap[inputType] = defaultKeyCode;
}
