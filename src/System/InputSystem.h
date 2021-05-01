/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#ifdef HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif // HAS_FILESYSTEM
#include <map>
#include <array>
#include <string>

#include <glm/glm.hpp>
#include <SDL_keycode.h>


namespace openblack
{

enum class InputType
{
	CameraForward,
	CameraBackward,
	CameraLeft,
	CameraRight,
	CameraDown,
	CameraUp,
	CameraFreeLook,
	CameraGrab
};


class InputSystem
{
public:
	InputSystem();

	virtual ~InputSystem() = default;	

	void LoadKeyMap(const std::string& path);

	void ProcessSDLEvent(const SDL_Event& e);
	void ClearState();


	glm::vec2 GetMouseMouvements() const;
	bool IsInputDown(InputType type) const;

private:
	glm::vec2 _mouseMouvements;

	std::map<std::string, InputType> _nameMap;
	std::map<InputType, SDL_Keycode> _keyMap;
	std::map<InputType, bool> _inputTypeState;

	void InitKeyType(InputType inputType, std::string keyMapName, SDL_Keycode defaultKeyCode);

	void ProcessSDLKeyboardEvent(const SDL_Event& e);
	void ProcessSDLMouseEvent(const SDL_Event& e);

	InputType GetInputTypeFromName(std::string name);
};

}
