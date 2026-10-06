/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <numbers>
#include <optional>

#include <glm/vec3.hpp>

namespace openblack::zoom_to
{

/// Where the camera is and what it looks at
struct CameraView
{
	glm::vec3 origin;
	glm::vec3 focus;
};

/// The temple key flies to the temple from this far, looking down this steeply
inline constexpr float k_TempleDistance = 130.0f;
inline constexpr float k_TemplePitch = std::numbers::pi_v<float> / 4.0f;
/// The realm key flies over the middle of the island from this far, looking down this steeply
inline constexpr float k_RealmDistance = 2200.0f;
inline constexpr float k_RealmPitch = std::numbers::pi_v<float> / 8.0f;
/// A single tap of the temple key puts the camera this far from what it looks at, this steeply
inline constexpr float k_TapDistance = 50.0f;
inline constexpr float k_TapPitch = std::numbers::pi_v<float> / 6.0f;
/// The camera looks at a point this far above the ground
inline constexpr float k_FocusAboveGround = 3.0f;
/// A second tap of the temple key within this long is a double tap
inline constexpr std::chrono::milliseconds k_DoubleTapTime {500};
/// While the camera still looks within this far of where it was flown to, the key flies it back
inline constexpr float k_ReturnRadius = 100.0f;

/// The heading of a view: its angle around the vertical, from the origin towards the focus
[[nodiscard]] float HeadingOf(const CameraView& view) noexcept;
/// Where a camera looking at the focus with the heading, from the distance and pitch (downwards), is
[[nodiscard]] glm::vec3 OriginAround(glm::vec3 focus, float heading, float distance, float pitch) noexcept;

/// The temple and realm keys. A single tap of the temple key puts the view back to a pleasing one over what the camera
/// looks at; a double tap flies to the temple, and the realm key flies over the middle of the island. Pressed again
/// while still looking there, either key flies back to where the camera was.
class ZoomToPlaces
{
public:
	/// The temple key pressed at a time. The temple's and the looked at point's ground positions have the land's height.
	/// Without a temple the key flies over the realm.
	[[nodiscard]] std::optional<CameraView> PressTemple(std::chrono::milliseconds now, const CameraView& current,
	                                                    glm::vec3 lookedAtGround, std::optional<glm::vec3> templeGround,
	                                                    glm::vec3 realmGround) noexcept;
	/// The realm key pressed
	[[nodiscard]] std::optional<CameraView> PressRealm(const CameraView& current, glm::vec3 realmGround) noexcept;

	/// Whether the camera has been flown to a place it can be flown back from
	[[nodiscard]] bool IsZoomedTo() const noexcept { return _returnTo.has_value(); }

private:
	[[nodiscard]] CameraView ZoomTo(const CameraView& current, glm::vec3 targetGround, float distance, float pitch) noexcept;

	std::optional<std::chrono::milliseconds> _lastTap;
	/// The view to fly back to, while flown to a place
	std::optional<CameraView> _returnTo;
};

} // namespace openblack::zoom_to
