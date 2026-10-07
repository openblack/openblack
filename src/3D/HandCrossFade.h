/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace openblack
{

/// The hand's move from one way of being held to another, such as from hovering to gripping the land.
///
/// The game fades the hand from where it was last drawn to where its new state puts it, at an even pace over 0.13
/// seconds, the same fade it gives the hand's pose. The place faded from stays put while the new one moves on.
class HandCrossFade
{
public:
	static constexpr float k_Seconds = 0.13f;

	/// Starts a fade from where the hand was last drawn
	void Start(glm::vec3 from)
	{
		_from = from;
		_elapsed = 0.0f;
	}

	/// Moves the fade on by a frame, before it is applied to the frame's place
	void Update(float deltaSeconds)
	{
		if (!_from.has_value())
		{
			return;
		}
		_elapsed += deltaSeconds;
		if (_elapsed >= k_Seconds)
		{
			_from.reset();
		}
	}

	/// Where the hand is drawn, given where its state puts it
	[[nodiscard]] glm::vec3 Apply(glm::vec3 to) const
	{
		return _from.has_value() ? glm::mix(*_from, to, _elapsed / k_Seconds) : to;
	}

	[[nodiscard]] bool IsActive() const { return _from.has_value(); }

private:
	std::optional<glm::vec3> _from;
	float _elapsed {0.0f};
};

} // namespace openblack
