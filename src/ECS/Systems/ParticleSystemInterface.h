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

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <entt/core/fwd.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "Particles/ParticleDrawFrame.h"
#include "Particles/ParticleSpellLink.h"

namespace openblack::particles
{
class Effect;
struct GestureTrail;
struct ShieldSphere;
} // namespace openblack::particles

namespace openblack::ecs::systems
{

/// The running particle effects: those miracles own and step themselves, the spot visuals scripts and the game place for
/// a time, and any other effect, stepped once a game turn. What they draw is gathered between the turns.
class ParticleSystemInterface
{
public:
	/// A running effect, by a number that is never reused
	using EffectId = uint32_t;
	static constexpr EffectId k_NoEffect = 0;

	/// How many things the last collected frame drew, for the debug window
	struct DrawStats
	{
		size_t sprites;
		size_t chains;
		size_t meshes;
		size_t mists;
		size_t lightStamps;
		size_t effects;
	};

	/// What the debug window shows of an effect
	struct EffectInfo
	{
		EffectId id;
		std::string file;
		glm::vec3 origin;
		float age;
		size_t atoms;
		size_t collections;
		bool closing;
		bool ownedBySpell;
		particles::draw::DrawPath path;
		/// Objects given to it that its rules haven't taken yet
		size_t targets;
		/// Seconds left of a spot visual's life, none for one that lasts until it is closed
		std::optional<float> secondsLeft;
		std::vector<std::string> unportedClasses;
	};

	virtual ~ParticleSystemInterface() = default;

	/// An effect from a particle file by its name (such as "SF_Smoke"), at a point and a magnitude; k_NoEffect if there
	/// is no such file. Its random numbers are the shared ones when synced.
	virtual EffectId Start(std::string_view file, glm::vec3 origin, float magnitude, bool synced = false) = 0;
	/// The effect of a particle type; k_NoEffect for a type without a file
	virtual EffectId Start(ParticleType type, glm::vec3 origin, float magnitude, bool synced = false) = 0;
	/// An effect a miracle owns and steps itself with ProcessForSpell; it is told the effect started. Its random numbers
	/// are the shared ones unless it is only this computer's, such as the one shown in the hand
	virtual EffectId StartForSpell(ParticleType type, glm::vec3 origin, glm::vec3 direction, float magnitude,
	                               particles::SpellSink& sink, bool synced = true) = 0;
	/// One step of a miracle's effect; false once it has ended, and then it is gone
	virtual bool ProcessForSpell(EffectId id, const particles::ProcessInfo& info, float seconds) = 0;
	/// Steps an effect that is stepped as it is drawn, by the frame's game time, rather than each turn; false once it has
	/// gone
	virtual bool ProcessByFrame(EffectId /*id*/, float /*seconds*/) { return false; }
	/// A spot visual: the info table's particle type at a point for some turns (its own life when not given, for ever
	/// when negative), staying where it was made, and ending when its owner object (if any) goes
	virtual EffectId StartSpotVisual(SpotVisualType type, glm::vec3 position, std::optional<int> turns, entt::entity owner,
	                                 float magnitude = 1.0f) = 0;

	virtual void SetOrigin(EffectId id, glm::vec3 origin) = 0;
	virtual void SetPlayer(EffectId id, int player) = 0;
	/// How the effect is drawn: sorted with everything else that blends unless told otherwise
	virtual void SetDrawPath(EffectId id, particles::draw::DrawPath path) = 0;
	/// Everything the effect draws is moved by this from where its last step left it, as the miracle in the hand follows
	/// the hand between turns
	virtual void SetDrawOffset(EffectId id, glm::vec3 offset) = 0;
	/// The live shield holding a point, its sphere grown by a margin, if any
	[[nodiscard]] virtual std::shared_ptr<particles::ShieldSphere> FindShield(glm::vec3 point, float margin) const = 0;
	/// How many particle sounds are playing or dying away, for the debug window
	[[nodiscard]] virtual size_t GetSoundCount() const = 0;
	/// An object for the effect's rules to act on, such as a person for the heal miracle's chakra
	virtual void AddTarget(EffectId id, entt::entity target) = 0;
	/// A point of the world for the effect's rules to act on, such as where a spot visual's beam ends
	virtual void AddTargetPosition(EffectId /*id*/, glm::vec3 /*position*/) {}
	/// A symbol of belief rises from something that gained it, in the effect every symbol rises in, kept running once
	/// wanted; no more than a few hundred wait
	virtual void AddBeliefSprite(const particles::BeliefSprite& /*sprite*/) {}

	/// This computer's hand in a frame, for the chain that follows it while it gestures
	struct HandFrame
	{
		glm::vec3 position {0.0f};
		/// How big the hand is drawn
		float size {1.0f};
		glm::vec3 cameraPosition {0.0f};
		/// It is drawing a gesture that shows the chain: one that powers up the miracle in it, or the circle that sizes
		/// it
		bool gesturing {false};
	};
	/// A recognised gesture's trail to show on the land, in the effect every trail shows in, which runs all the time
	virtual void AddGestureTrail(std::shared_ptr<particles::GestureTrail> /*trail*/) {}
	/// Once a frame, by the frame's game time (none while the game is paused): the trails' sheets of light move on, and
	/// the chain behind the hand steps
	virtual void UpdateFrame(float /*gameSeconds*/, const HandFrame& /*hand*/) {}
	/// The effect stops making particles and fades out as its file has it, or goes at once
	virtual void CloseDown(EffectId id) = 0;
	virtual void Delete(EffectId id) = 0;
	[[nodiscard]] virtual bool IsRunning(EffectId id) const = 0;
	[[nodiscard]] virtual particles::Effect* Find(EffectId id) = 0;

	/// Once a game turn: every effect not owned by a miracle steps, newest first, and the spot visuals count down
	virtual void ProcessTurn() = 0;
	/// A new land: every effect goes
	virtual void Reset() = 0;

	/// Everything the effects draw this frame, t of the way through the game turn, into a frame that is cleared first.
	/// The frame doesn't depend on the camera: each pass orders it for its own (particles::draw::Order).
	virtual void CollectDrawFrame(float turnFraction, particles::draw::Frame& frame) const = 0;
	[[nodiscard]] virtual DrawStats GetDrawStats() const = 0;
	[[nodiscard]] virtual std::vector<EffectInfo> GetEffects() const = 0;
	/// The particle files' names, for the debug window
	[[nodiscard]] virtual std::vector<std::string> GetFileNames() const = 0;
	/// Stops stepping effects, for looking at them in the debug window
	virtual void SetPaused(bool paused) = 0;
	[[nodiscard]] virtual bool IsPaused() const = 0;
};

} // namespace openblack::ecs::systems
