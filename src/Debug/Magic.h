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

#include <string>
#include <vector>

#include <glm/vec3.hpp>

#include "Enums.h"
#include "Magic/SpellChants.h"
#include "Magic/WorshipBattery.h"
#include "Particles/ParticleDrawPath.h"
#include "Window.h"

namespace openblack::debug::gui
{

/// Shows the miracle tables (costs, timers and seeds of every magic type) and a prayer power sandbox: a worship site
/// with a number of dancers feeding a miracle cast from it, stepped turn by turn
class Magic final: public Window
{
public:
	Magic() noexcept;

protected:
	void Draw() noexcept override;
	void Update() noexcept override;
	void UpdateAlways() noexcept override;
	void ProcessEventOpen(const SDL_Event& event) noexcept override;
	void ProcessEventAlways(const SDL_Event& event) noexcept override;

private:
	void DrawSelected() noexcept;
	void DrawTable() noexcept;
	void DrawSandbox() noexcept;
	void DrawParticles() noexcept;
	void DrawRunningEffects() noexcept;
	/// The running miracles, the dispensers and the creatures' spells, and tools to cast and put them down
	void DrawMiracles() noexcept;
	/// The spawned effects given a time close down when it is up
	void UpdateTimedEffects(float seconds) noexcept;
	/// Where a spawned effect goes: the hand, or what the camera looks at, on the land
	[[nodiscard]] glm::vec3 SpawnPoint() const noexcept;
	/// What the particles drew in the last frame, by kind
	void DrawParticleStats() const noexcept;
	/// Gives an effect the creatures and villagers nearest its origin to act on
	void GiveNearestTargets(uint32_t effect, const glm::vec3& origin) const noexcept;

	void ResetSandbox() noexcept;
	void CastInSandbox() noexcept;
	void StepSandbox() noexcept;

	MagicType _selected {MagicType::Fireball};
	/// The miracle the miracles' tab casts and puts down
	MagicType _miracle {MagicType::Fireball};

	// The sandbox
	Tribe _tribe {Tribe::NORSE};
	int _dancers {10};
	float _tribalPower {1.0f};
	magic::WorshipBattery _site;
	magic::SpellChants _spell;
	MagicType _spellType {MagicType::None};
	bool _spellRunning {false};
	float _spellAge {0.0f};
	float _spellStrength {0.0f};
	uint32_t _turns {0};
	bool _running {false};
	float _sinceTurn {0.0f};

	// The particles' tab
	enum class SpawnAt : uint8_t
	{
		CameraFocus,
		Hand,
	};
	/// By particle type, or by any file
	bool _byFile {false};
	ParticleType _particleType {ParticleType::Smoke};
	int _particleFile {0};
	SpawnAt _spawnAt {SpawnAt::CameraFocus};
	float _spawnHeight {0.0f};
	float _magnitude {1.0f};
	int _player {0};
	/// Seconds before it closes down, none to run until closed
	float _closeAfter {0.0f};
	bool _synced {false};
	particles::draw::DrawPath _drawPath {particles::draw::DrawPath::Sorted};
	/// The creatures and villagers nearest a spawned effect it is given to act on, as the heal miracle is given people
	int _targets {0};
	std::vector<std::string> _particleFiles;
	struct TimedEffect
	{
		uint32_t id;
		float secondsLeft;
	};
	std::vector<TimedEffect> _timedEffects;
};

} // namespace openblack::debug::gui
