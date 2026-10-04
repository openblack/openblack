/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <memory>
#include <vector>

#include <entt/entity/fwd.hpp>
#include <glm/vec3.hpp>

namespace openblack
{
namespace graphics
{
struct BeamMesh;
}

/// The effects of the creature's room that CreatureRoom::InitEngine sets up and CreatureRoom::Draw and DrawAdditional
/// move on each frame the room is drawn: the flames of its fire and the smoke over it, and the spray and mist at the
/// foot of its waterfall. The flames and smoke are sprites and the mist domes of mist.l3d, which the renderer draws.
class CreatureCaveEffects
{
public:
	/// LH3DSmoke's particles each: they rise from the smoke's place over 900 steps of age, 255 a second
	static constexpr size_t k_SmokeParticles = 10;
	static constexpr int32_t k_SmokeLife = 900;

	/// A smoke's particles and how they move (LH3DSmoke)
	struct Smoke
	{
		struct Particle
		{
			glm::vec3 position {0.0f};
			glm::vec3 velocity {0.0f};
			int32_t age {0};
			float angle {0.0f};
			bool spinsBack {false};
			bool hidden {true};
			entt::entity sprite;
		};
		glm::vec3 place {0.0f};
		/// Which way the particles drift as they rise
		glm::vec3 drift {0.0f, 1.0f, 0.0f};
		/// RGB of the particles, which fade by age
		glm::vec3 colour {1.0f};
		float size {1.0f};
		/// Rising smoke is thrown up from its place and falls back, fading in as it starts
		bool rising {false};
		std::array<Particle, k_SmokeParticles> particles;
	};

	/// The flames on the fire, the place CreatureRoom::DrawAdditional plays its sound at
	explicit CreatureCaveEffects(glm::vec3 fire);
	~CreatureCaveEffects();
	CreatureCaveEffects(const CreatureCaveEffects&) = delete;
	CreatureCaveEffects& operator=(const CreatureCaveEffects&) = delete;

	/// A frame of the room drawn, of milliseconds, and the time in milliseconds the flames take their frames from
	void Update(uint32_t milliseconds, uint32_t tickCount);

	/// LH3DSmoke's step of its particles over milliseconds (fn_007F8E00), without drawing them
	static void StepSmoke(Smoke& smoke, uint32_t milliseconds);
	/// The alpha, out of 255, of a smoke particle of an age
	[[nodiscard]] static uint8_t SmokeAlpha(int32_t age, bool rising);
	/// The frame of the flames' animation of a flame at a time, 0 to 31
	[[nodiscard]] static uint32_t FlameFrame(uint32_t tickCount, uint32_t flame);

private:
	void ShowSmoke(const Smoke& smoke) const;

	std::vector<entt::entity> _flames;
	std::vector<Smoke> _fireSmoke;
	std::vector<Smoke> _spray;
	struct Mist
	{
		entt::entity entity;
		int32_t counter;
	};
	std::vector<Mist> _mists;
	std::shared_ptr<graphics::BeamMesh> _mistDome;
};

} // namespace openblack
