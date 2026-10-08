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
#include <unordered_map>
#include <utility>
#include <vector>

#include <entt/entity/entity.hpp>

#include "ECS/Systems/ParticleSystemInterface.h"
#include "ParticleObjectEffects.h"
#include "Particles/ParticleClassRegistry.h"
#include "Particles/ParticleCreators.h"
#include "Particles/ParticleEffect.h"
#include "Particles/ParticleMaths.h"
#include "Particles/ParticleSoundRelease.h"

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
	[[nodiscard]] glm::vec3 CameraPosition() const override;
	void PlayListenerSound(uint32_t inGameSample) override;
	[[nodiscard]] std::optional<TargetInfo> Target(entt::entity target, bool centre) const override;
	[[nodiscard]] bool IsTargetHeld(entt::entity target) const override;
	[[nodiscard]] bool IsTargetClaimed(entt::entity target) const override { return _claimed.contains(target); }
	void ClaimTarget(entt::entity target, bool claimed) override;
	void SetTargetGlow(entt::entity target, glm::u8vec3 rgb) override;
	void StartSound(const particles::Effect& effect, const std::shared_ptr<particles::ParticleSoundLink>& sound) override;
	[[nodiscard]] bool IsWater(glm::vec3 point) const override;
	[[nodiscard]] int SurfaceAt(glm::vec3 point) const override;
	[[nodiscard]] int SoundAlignment(int player) const override;
	[[nodiscard]] glm::vec3 LandNormal(glm::vec2 xz) const override;
	[[nodiscard]] bool IsRainingAt(glm::vec3 point) const override;
	[[nodiscard]] glm::vec3 WindAt(glm::vec3 point) const override;
	[[nodiscard]] glm::vec3 SmoothWindAt(glm::vec3 point) const override;
	[[nodiscard]] uint32_t AddRainStorm(const particles::storm::RainStorm& storm) override;
	bool MoveRainStorm(uint32_t storm, glm::vec3 centre) override;
	[[nodiscard]] bool IsDryLand(glm::vec3 point) const override;
	void RemoveRainStorm(uint32_t storm) override;
	[[nodiscard]] std::vector<particles::StrikeCandidate> StrikeCandidates(glm::vec3 centre, size_t cells) const override;
	[[nodiscard]] bool LandBlocks(glm::vec3 from, glm::vec3 to) const override;
	void QueueArcs(entt::entity object) override;
	void StrikeWithoutMiracle(glm::vec3 point) override;
	[[nodiscard]] std::optional<particles::BeliefSprite> TakeBeliefSprite() override;
	/// A symbol of belief waits to rise; whether there was room
	bool QueueBeliefSprite(const particles::BeliefSprite& sprite);
	[[nodiscard]] std::optional<entt::entity> TakeArcs() override;
	[[nodiscard]] std::shared_ptr<const void> HoldArc() const override;
	[[nodiscard]] std::vector<particles::SurfacePoint>
	SurfacePoints(entt::entity object, size_t count, const std::function<int32_t(int32_t)>& random) const override;
	void AddBolt(const std::shared_ptr<particles::BoltShare>& bolt) override;
	[[nodiscard]] std::vector<std::shared_ptr<particles::BoltShare>> Bolts() const override;
	[[nodiscard]] uint64_t NextBoltOrder() override { return ++_boltOrder; }
	/// Whether an object has been struck for arcs since last asked, so that the effect the arcs crawl in is to run
	[[nodiscard]] bool TakeArcsWanted();
	[[nodiscard]] std::shared_ptr<particles::GestureTrail> TakeGestureTrail() override;
	void AddLightSheet(const std::shared_ptr<particles::LightSheet>& sheet) override;
	void SetHandGlow(uint32_t rgb) override;
	void QueueGestureTrail(std::shared_ptr<particles::GestureTrail> trail);
	/// The sheets of light still standing; those gone are forgotten
	[[nodiscard]] std::vector<std::shared_ptr<particles::LightSheet>> LightSheets() const;
	void AddShield(const std::shared_ptr<particles::ShieldSphere>& shield) override;
	[[nodiscard]] std::shared_ptr<particles::ShieldSphere> FindShield(glm::vec3 point, float margin) const override;
	[[nodiscard]] std::shared_ptr<particles::ShieldSphere> ShieldOf(const particles::Effect& effect) const override;
	// What a tornado asks of the world, through the tornado system
	[[nodiscard]] uint32_t GameTurn() const override;
	[[nodiscard]] glm::u8vec3 LandColour(glm::vec2 xz) const override;
	[[nodiscard]] std::vector<particles::TornadoCandidate> TornadoCandidates(glm::vec3 foot, float reach) const override;
	void CatchCreature(entt::entity creature) override;
	entt::entity TakeFromPile(entt::entity pile, uint32_t amount, float sizeShare) override;
	std::optional<glm::mat3> Carry(const std::shared_ptr<particles::CarriedObject>& carried) override;
	[[nodiscard]] std::vector<glm::vec3> TargetExtraPoints(entt::entity target) const override;
	[[nodiscard]] std::optional<glm::vec3> ObjectPosition(entt::entity object) const override;
	[[nodiscard]] SurfacePoint RandomSurfacePoint(entt::entity object, particles::Effect& effect) const override;
	[[nodiscard]] float PlayerAlignment(int player) const override;
	[[nodiscard]] std::optional<particles::CreatureSpellBody> CreatureBody(entt::entity creature) const override;
	[[nodiscard]] uint32_t TargetPointCount(entt::entity object) const override;
	[[nodiscard]] std::optional<glm::vec3> TargetPoint(entt::entity object, uint32_t index) const override;
	[[nodiscard]] float TargetScale(entt::entity object) const override;
	void FollowCameraPath(std::string_view file, std::string_view animation, const glm::mat4& placement, float pauseSeconds,
	                      float speedUp, entt::entity spell) override;
	[[nodiscard]] particles::ObjectEffectsInterface* ObjectEffects() override { return &_objectEffects; }
	[[nodiscard]] particles::FragmentSourceInterface* FragmentSource() override { return &_fragments; }

	/// Once a game turn: the sounds follow their particles, loops play on, and the sounds let go stop
	void ProcessSounds();
	/// Once a game turn: the glows nothing lights any more go
	void ProcessGlows();
	[[nodiscard]] size_t SoundCount() const { return _sounds.size(); }
	void Reset();

private:
	/// A sound a particle started, and what plays it
	struct PlayingSound
	{
		std::shared_ptr<particles::ParticleSoundLink> link;
		entt::entity emitter {entt::null};
		/// What the sound is played for: its own, so that a second effect playing the same sound doesn't find this one
		/// playing and give up
		entt::entity owner {entt::null};
		/// Let go of by its particle, and how it is fading out
		bool letGo {false};
		particles::SoundLetGo fade;
		/// Seconds before it is heard, while it travels to the listener
		float wait {0.0f};
	};
	void Play(PlayingSound& sound) const;
	/// The particle let go of the sound: stopped at once, or, when its effect file soft-releases it, its loop released
	/// and its first step quieter taken
	static void LetGo(PlayingSound& sound);
	/// A let-go sound one step quieter, stopped once silent
	static void Fade(PlayingSound& sound);
	/// The sound has gone: its owner goes with it
	static void Forget(PlayingSound& sound);

	GameObjectEffects _objectEffects;
	GameFragmentSource _fragments;
	std::set<entt::entity> _claimed;
	/// What lightning struck, waiting for its arcs
	std::vector<entt::entity> _arcsWaiting;
	/// Held by every arc crawling over an object, so that its count is how many there are
	std::shared_ptr<char> _liveArcs {std::make_shared<char>()};
	std::vector<particles::BeliefSprite> _beliefSprites;
	/// The recognised gestures' trails waiting to be shown, and the sheets of light the trails raised
	std::vector<std::shared_ptr<particles::GestureTrail>> _gestureTrails;
	mutable std::vector<std::weak_ptr<particles::LightSheet>> _lightSheets;
	bool _arcsWanted {false};
	/// The bolts from hands, and how many there have been
	std::vector<std::weak_ptr<particles::BoltShare>> _bolts;
	uint64_t _boltOrder {0};
	std::vector<PlayingSound> _sounds;
	/// The storms the effects laid over the land, by the numbers the effects know them by
	std::unordered_map<uint32_t, entt::entity> _rainStorms;
	uint32_t _nextRainStorm {1};
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
	                       particles::SpellSink& sink, bool synced) override;
	bool ProcessForSpell(EffectId id, const particles::ProcessInfo& info, float seconds) override;
	bool ProcessByFrame(EffectId id, float seconds) override;
	EffectId StartSpotVisual(SpotVisualType type, glm::vec3 position, std::optional<int> turns, entt::entity owner,
	                         float magnitude) override;

	void SetOrigin(EffectId id, glm::vec3 origin) override;
	void SetPlayer(EffectId id, int player) override;
	void SetDrawPath(EffectId id, particles::draw::DrawPath path) override;
	void SetDrawOffset(EffectId id, glm::vec3 offset) override;
	[[nodiscard]] std::shared_ptr<particles::ShieldSphere> FindShield(glm::vec3 point, float margin) const override;
	[[nodiscard]] size_t GetSoundCount() const override { return _world.SoundCount(); }
	void AddTarget(EffectId id, entt::entity target) override;
	void AddTargetPosition(EffectId id, glm::vec3 position) override;
	void AddBeliefSprite(const particles::BeliefSprite& sprite) override;
	void AddGestureTrail(std::shared_ptr<particles::GestureTrail> trail) override;
	void UpdateFrame(float gameSeconds, const HandFrame& hand) override;
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
	/// The effect that throws the pieces of what the blasts break
	EffectId _explodeObject {k_NoEffect};
	/// The effect every recognised gesture's trail shows in, and the chain behind the hand while it gestures; both run
	/// all the time, started again whenever they end
	EffectId _gestureTrails {k_NoEffect};
	EffectId _gestureChain {k_NoEffect};
	struct Running
	{
		EffectId id;
		std::string file;
		std::unique_ptr<particles::Effect> effect;
		/// Stepped by its miracle, or as it is drawn, not by ProcessTurn
		bool ownedBySpell {false};
		/// Stepped every frame, not by ProcessTurn
		bool everyFrame {false};
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
	/// The gesture trails' effect, started when it isn't running
	void KeepGestureTrails();
	/// The trails' sheets of light, each in its place among what blends
	void AddLightSheets(particles::draw::Frame& frame) const;

	GameCreatorResources _resources;
	particles::ParticleClassRegistry _classes;
	GameParticleWorld _world;
	particles::maths::ValueNoise _noise;
	/// Newest first, as they are stepped and drawn
	std::deque<Running> _effects;
	EffectId _nextId {1};
	bool _paused {false};
	/// A symbol of belief waits for its effect
	bool _beliefWanted {false};
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
