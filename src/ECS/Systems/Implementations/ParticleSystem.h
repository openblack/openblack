/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <entt/entity/entity.hpp>

#include "ECS/Systems/ParticleSystemInterface.h"
#include "Particles/ParticleClassRegistry.h"
#include "Particles/ParticleCreators.h"
#include "Particles/ParticleEffect.h"
#include "Particles/ParticleMaths.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

/// The particle effects' world: the land's height, the players' colours, the camera and the objects miracles act on
class GameParticleWorld final: public particles::ParticleWorldInterface
{
public:
	[[nodiscard]] float LandHeight(glm::vec2 xz) const override;
	[[nodiscard]] uint32_t PlayerColour(int player) const override;
	[[nodiscard]] glm::vec3 CameraRight() const override;
	[[nodiscard]] glm::vec3 CameraUp() const override;
	[[nodiscard]] std::optional<TargetInfo> Target(entt::entity target, bool centre) const override;
	[[nodiscard]] bool IsTargetHeld(entt::entity target) const override;
	[[nodiscard]] bool IsTargetClaimed(entt::entity target) const override { return _claimed.contains(target); }
	void ClaimTarget(entt::entity target, bool claimed) override;
	void StartSound(const particles::Effect& effect, const std::shared_ptr<particles::ParticleSoundLink>& sound) override;
	[[nodiscard]] bool IsWater(glm::vec3 point) const override;
	[[nodiscard]] glm::vec3 LandNormal(glm::vec2 xz) const override;
	[[nodiscard]] bool IsRainingAt(glm::vec3 point) const override;
	[[nodiscard]] glm::vec3 WindAt(glm::vec3 point) const override;
	[[nodiscard]] std::vector<particles::StrikeCandidate> StrikeCandidates(glm::vec3 centre, size_t cells) const override;
	[[nodiscard]] bool LandBlocks(glm::vec3 from, glm::vec3 to) const override;
	void AddShield(const std::shared_ptr<particles::ShieldSphere>& shield) override;
	[[nodiscard]] std::shared_ptr<particles::ShieldSphere> FindShield(glm::vec3 point, float margin) const override;
	[[nodiscard]] std::shared_ptr<particles::ShieldSphere> ShieldOf(const particles::Effect& effect) const override;

	/// Once a game turn: the sounds follow their particles, loops play on, and the sounds let go stop
	void ProcessSounds();
	[[nodiscard]] size_t SoundCount() const { return _sounds.size(); }
	void Reset();

private:
	/// A sound a particle started, and what plays it
	struct PlayingSound
	{
		std::shared_ptr<particles::ParticleSoundLink> link;
		entt::entity emitter {entt::null};
		bool letGo {false};
	};
	void Play(PlayingSound& sound) const;

	std::set<entt::entity> _claimed;
	std::vector<PlayingSound> _sounds;
	mutable std::vector<std::weak_ptr<particles::ShieldSphere>> _shields;
	/// The game's sound actions by name, read when first needed
	mutable std::optional<std::map<std::string, int32_t, std::less<>>> _soundActions;
};

/// The models and light maps of the particle creators, found by their names and loaded once through the resource caches
class GameCreatorResources final: public particles::CreatorResourcesInterface
{
public:
	[[nodiscard]] std::optional<entt::id_type> MeshByName(std::string_view name) override;
	[[nodiscard]] std::optional<entt::id_type> MeshByFile(std::string_view path) override;
	[[nodiscard]] std::optional<entt::id_type> LightMap(std::string_view path, int pitch, int channels, int framesInFile,
	                                                    int framesInUse) override;

private:
	/// The game's list of models by name, read when first needed
	std::optional<std::map<std::string, int32_t, std::less<>>> _meshNames;
};

class ParticleSystem final: public ParticleSystemInterface
{
public:
	ParticleSystem();
	~ParticleSystem() override;
	ParticleSystem(const ParticleSystem&) = delete;
	ParticleSystem& operator=(const ParticleSystem&) = delete;
	ParticleSystem(ParticleSystem&&) = delete;
	ParticleSystem& operator=(ParticleSystem&&) = delete;

	EffectId Start(std::string_view file, glm::vec3 origin, float magnitude, bool synced) override;
	EffectId Start(ParticleType type, glm::vec3 origin, float magnitude, bool synced) override;
	EffectId StartForSpell(ParticleType type, glm::vec3 origin, glm::vec3 direction, float magnitude,
	                       particles::SpellSink& sink, bool synced = true) override;
	bool ProcessForSpell(EffectId id, const particles::ProcessInfo& info, float seconds) override;
	EffectId StartSpotVisual(SpotVisualType type, glm::vec3 position, std::optional<int> turns, entt::entity owner,
	                         float magnitude) override;

	void SetOrigin(EffectId id, glm::vec3 origin) override;
	void SetPlayer(EffectId id, int player) override;
	void SetDrawPath(EffectId id, particles::draw::DrawPath path) override;
	void SetDrawOffset(EffectId id, glm::vec3 offset) override;
	[[nodiscard]] std::shared_ptr<particles::ShieldSphere> FindShield(glm::vec3 point, float margin) const override;
	[[nodiscard]] size_t GetSoundCount() const override { return _world.SoundCount(); }
	void AddTarget(EffectId id, entt::entity target) override;
	void CloseDown(EffectId id) override;
	void Delete(EffectId id) override;
	[[nodiscard]] bool IsRunning(EffectId id) const override;
	[[nodiscard]] particles::Effect* Find(EffectId id) override;

	void ProcessTurn() override;
	void Reset() override;

	void CollectDrawFrame(float turnFraction, particles::draw::Frame& frame) const override;
	[[nodiscard]] DrawStats GetDrawStats() const override { return _drawStats; }
	[[nodiscard]] std::vector<EffectInfo> GetEffects() const override;
	[[nodiscard]] std::vector<std::string> GetFileNames() const override;
	void SetPaused(bool paused) override { _paused = paused; }
	[[nodiscard]] bool IsPaused() const override { return _paused; }

private:
	struct Running
	{
		EffectId id;
		std::string file;
		std::unique_ptr<particles::Effect> effect;
		/// Stepped by its miracle, not by ProcessTurn
		bool ownedBySpell {false};
		/// A spot visual's turns left, negative for ever
		std::optional<int> turnsLeft;
		/// An object it follows and ends with
		entt::entity owner {entt::null};
		particles::draw::DrawPath path {particles::draw::DrawPath::Sorted};
		/// Everything it draws is moved by this, as the miracle in the hand follows the hand between turns
		glm::vec3 drawOffset {0.0f};
	};

	std::deque<Running>::iterator FindRunning(EffectId id);
	[[nodiscard]] std::deque<Running>::const_iterator FindRunning(EffectId id) const;
	/// Steps an effect; true once it has ended
	bool StepEffect(particles::Effect& effect, float seconds);
	/// The sheets its sprites are drawn from, looked up whatever case the files spell their names in
	void ResolveTextures(const particles::Effect& effect);

	GameCreatorResources _resources;
	particles::ParticleClassRegistry _classes;
	GameParticleWorld _world;
	particles::maths::ValueNoise _noise;
	/// Newest first, as they are stepped and drawn
	std::deque<Running> _effects;
	EffectId _nextId {1};
	bool _paused {false};
	/// The textures by the name a creator spells them with: the sheet and its alpha
	std::map<std::string, std::pair<entt::id_type, entt::id_type>, std::less<>> _textures;
	/// Every texture's name in lower case, to its spelling on disk
	std::map<std::string, std::string, std::less<>> _textureStems;
	/// The particle classes already reported as not run yet
	std::set<std::string, std::less<>> _reportedUnported;
	/// What the last collected frame drew, and its walk, kept for its room
	mutable DrawStats _drawStats {};
	mutable particles::Effect::DrawWalk _walk;
};

} // namespace openblack::ecs::systems
