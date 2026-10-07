/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CameraHelp.h"

#include <cmath>

#include <array>

namespace openblack::camera_help
{

namespace
{
/// The tutorials' self-tilting camera: its pitch, in radians, and its height over the land
constexpr float k_TutorialPitch = 0.448798954f;
constexpr float k_TutorialHeight = 15.0f;

/// What each interface level allows, and whether the camera tilts itself; the levels the game knows, from 0
struct Level
{
	bool known;
	uint32_t features;
	bool autoPitch;
	bool cameraKeys;
	bool placeKeys;
	/// The hand's reach it sets, none leaving it as it was
	std::optional<float> handReach;
};
constexpr float k_TutorialReach = 75.0f;
constexpr std::array<Level, 16> k_Levels {{
    {.known = true,
     .features = camera_drag::k_DefaultFeatures,
     .autoPitch = false,
     .cameraKeys = true,
     .placeKeys = true,
     .handReach = k_DefaultHandReach},
    {.known = true, .features = 0x08, .autoPitch = true, .cameraKeys = true, .placeKeys = false, .handReach = k_TutorialReach},
    {.known = true, .features = 0x08, .autoPitch = true, .cameraKeys = true, .placeKeys = false, .handReach = std::nullopt},
    {.known = true,
     .features = 0x02,
     .autoPitch = false,
     .cameraKeys = true,
     .placeKeys = false,
     .handReach = k_DefaultHandReach},
    {.known = true,
     .features = 0x18,
     .autoPitch = false,
     .cameraKeys = true,
     .placeKeys = false,
     .handReach = k_DefaultHandReach},
    {.known = true, .features = 0x24, .autoPitch = false, .cameraKeys = true, .placeKeys = false, .handReach = std::nullopt},
    {.known = true,
     .features = 0x02,
     .autoPitch = false,
     .cameraKeys = true,
     .placeKeys = false,
     .handReach = k_DefaultHandReach},
    {.known = true,
     .features = 0x26,
     .autoPitch = false,
     .cameraKeys = true,
     .placeKeys = false,
     .handReach = k_DefaultHandReach},
    {.known = true, .features = 0x00, .autoPitch = false, .cameraKeys = false, .placeKeys = false, .handReach = std::nullopt},
    {.known = false, .features = 0, .autoPitch = false, .cameraKeys = false, .placeKeys = false, .handReach = std::nullopt},
    {.known = true,
     .features = 0x0A,
     .autoPitch = true,
     .cameraKeys = true,
     .placeKeys = false,
     .handReach = k_DefaultHandReach},
    {.known = true, .features = 0x01, .autoPitch = false, .cameraKeys = true, .placeKeys = false, .handReach = std::nullopt},
    {.known = true, .features = 0x00, .autoPitch = false, .cameraKeys = false, .placeKeys = false, .handReach = std::nullopt},
    {.known = true,
     .features = 0x1A,
     .autoPitch = false,
     .cameraKeys = true,
     .placeKeys = false,
     .handReach = k_DefaultHandReach},
    {.known = true,
     .features = 0x1B,
     .autoPitch = false,
     .cameraKeys = true,
     .placeKeys = false,
     .handReach = k_DefaultHandReach},
    {.known = true, .features = 0x3F, .autoPitch = false, .cameraKeys = true, .placeKeys = false, .handReach = std::nullopt},
}};

/// Nothing tilts within this of the way there; the tilt is a tenth of the way, at most the frame's seconds, and
/// becomes pitch input at this rate
constexpr float k_CloseEnough = 0.01f;
constexpr float k_ShareOfTheWay = 0.1f;
constexpr float k_InputPerTilt = -150.0f;
} // namespace

void CameraHelp::SetAutoPitch(float pitch, float height, bool on)
{
	Enable(on ? feature::k_AutoPitch : 0u, feature::k_AutoPitch);
	autoPitch = pitch;
	autoPitchHeight = height;
}

void CameraHelp::ResetForNewLand()
{
	features = camera_drag::k_DefaultFeatures;
	autoPitchHeight = k_DefaultAutoPitchHeight;
}

bool CameraHelp::SetInterfaceLevel(int32_t level)
{
	if (level < 0 || level >= static_cast<int32_t>(k_Levels.size()) || !k_Levels.at(static_cast<size_t>(level)).known)
	{
		return false;
	}
	const auto& allowed = k_Levels.at(static_cast<size_t>(level));
	Enable(allowed.features, ~0u);
	cameraKeys = allowed.cameraKeys;
	placeKeys = allowed.placeKeys;
	if (allowed.handReach.has_value())
	{
		handReach = *allowed.handReach;
	}
	if (allowed.autoPitch)
	{
		SetAutoPitch(k_TutorialPitch, k_TutorialHeight, true);
	}
	return true;
}

input::BindableActionMap CameraHelp::BlockedActions() const
{
	using input::BindableActionMap;
	constexpr auto k_Places = static_cast<uint64_t>(BindableActionMap::ZOOM_TO_TEMPLE) |
	                          static_cast<uint64_t>(BindableActionMap::ZOOM_TO_CREATURE) |
	                          static_cast<uint64_t>(BindableActionMap::ZOOM_TO_REALM);
	// Every camera action from zooming out to flying to the realm, but talking
	constexpr auto k_Camera =
	    (static_cast<uint64_t>(BindableActionMap::ZOOM_OUT) | static_cast<uint64_t>(BindableActionMap::ZOOM_IN) |
	     static_cast<uint64_t>(BindableActionMap::ZOOM_ON) | static_cast<uint64_t>(BindableActionMap::MOVE_LEFT) |
	     static_cast<uint64_t>(BindableActionMap::MOVE_RIGHT) | static_cast<uint64_t>(BindableActionMap::MOVE_FORWARDS) |
	     static_cast<uint64_t>(BindableActionMap::MOVE_BACKWARDS) | static_cast<uint64_t>(BindableActionMap::TILT_UP) |
	     static_cast<uint64_t>(BindableActionMap::TILT_DOWN) | static_cast<uint64_t>(BindableActionMap::ROTATE_LEFT) |
	     static_cast<uint64_t>(BindableActionMap::ROTATE_RIGHT) | static_cast<uint64_t>(BindableActionMap::ROTATE_ON) |
	     static_cast<uint64_t>(BindableActionMap::ROTATE_AROUND_MOUSE_ON)) |
	    k_Places;
	if (!cameraKeys)
	{
		return static_cast<BindableActionMap>(k_Camera);
	}
	return placeKeys ? BindableActionMap::NONE : static_cast<BindableActionMap>(k_Places);
}

std::optional<float> AutoPitchInput(float targetPitch, float pitch, float deltaSeconds)
{
	const auto tilt = (targetPitch - pitch) * k_ShareOfTheWay;
	if (std::abs(tilt) <= k_CloseEnough)
	{
		return std::nullopt;
	}
	auto limited = tilt;
	if (limited <= -deltaSeconds)
	{
		limited = -deltaSeconds;
	}
	else if (deltaSeconds <= limited)
	{
		limited = deltaSeconds;
	}
	return limited * k_InputPerTilt;
}

} // namespace openblack::camera_help
