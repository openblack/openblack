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
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "ECS/PhysicsClasses.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "Particles/ParticleEffect.h"
#include "Physics/BodyShapes.h"
#include "Physics/TurnRules.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
class btDiscreteDynamicsWorld;
struct btDbvtBroadphase;
class btSequentialImpulseConstraintSolver;

namespace openblack
{
class LandIslandInterface;
namespace ecs
{
class PhysicsGround;
namespace components
{
struct Transform;
}
} // namespace ecs
} // namespace openblack

namespace openblack::ecs::systems
{

class DynamicsSystem final: public DynamicsSystemInterface
{
public:
	DynamicsSystem();
	~DynamicsSystem() override;

	void Reset() override;
	void Update(std::chrono::microseconds& dt) override;
	void AddRigidBody(btRigidBody* object) override;
	void RemoveRigidBody(btRigidBody* object) override;
	void RegisterRigidBodies() override;
	void RegisterIslandRigidBodies(LandIslandInterface& island) override;
	void UpdatePhysicsTransforms() override;
	[[nodiscard]] std::optional<std::pair<ecs::components::Transform, RigidBodyDetails>>
	RayCastClosestHit(const glm::vec3& origin, const glm::vec3& direction, float tMax) const override;
	[[nodiscard]] std::optional<glm::vec3> RayCastLand(const glm::vec3& origin, const glm::vec3& direction,
	                                                   float tMax) const override;

	void ResetSimulation() override;
	void SetClassHooks(std::unique_ptr<PhysicsClassHooks> hooks) override;
	void ProcessTurn() override;
	void UpdateFrame(float turnFraction, float gameSeconds) override;
	void CollectDrawFrame(particles::draw::Frame& frame) const override;

	PhysicsStarted InitialisePhysics(entt::entity object, const PhysicsStart& start) override;
	PhysicsStarted StartPhysicsAsObject(entt::entity object, const PhysicsStart& start) override;
	PhysicsEntry* AddObject(entt::entity object, const PhysicsStart& start) override;
	[[nodiscard]] PhysicsEntry* Find(entt::entity object) override;
	[[nodiscard]] bool IsFlying(entt::entity object) const override;
	entt::entity RemoveObject(entt::entity object, bool insert, bool endPhysics) override;
	entt::entity EndPhysicsAsObject(entt::entity object, bool insert, bool hasBody) override;
	void RaiseClearOfWhatIsUnder(PhysicsEntry& entry) override;
	void SettleOnLand(PhysicsEntry& entry, bool noPullDown, bool alignToSlope) override;
	FromHandResult LetGoFromHand(entt::entity object, const FromHand& release) override;
	[[nodiscard]] std::optional<std::pair<glm::mat3, glm::vec3>> ReleasePose(entt::entity object, bool alignToSlope) override;
	float PushObject(entt::entity object) override;
	[[nodiscard]] const physics::Ground* GetGround() const override;

	void RecordHit(entt::entity hit, entt::entity hitter) override;
	[[nodiscard]] entt::entity GetHitObject() const override;
	[[nodiscard]] entt::entity GetObjectWhichHit() const override;

	void ForEachEntry(const std::function<void(const PhysicsEntry&)>& visit) const override;

private:
	/// Any creature's leash tied to an object lets go, as the object starts to move
	static void LetGoOfLeashesTiedTo(entt::entity object);
	/// A toy a player's hand let go may set the player's creature thinking of playing with it, whether it landed or flew
	void ConsiderToyPlay(entt::entity object, const FromHand& release);
	/// The kind of an object as the physics sees it now
	[[nodiscard]] physics_classes::ClassFacts FactsOf(entt::entity object) const;
	/// A body for an object at its place, moving or resting; none for an object without a shape
	[[nodiscard]] std::unique_ptr<physics::Body> MakeBody(entt::entity object, const physics_classes::ClassFacts& facts);
	/// The creature's body: an obstacle of its bones' ellipsoids, as it stands posed now
	[[nodiscard]] std::unique_ptr<physics::Body> MakeCreatureBody(entt::entity creature, const physics::Material& material);
	/// The creature's bones as the ellipsoids a body hits, posed as they are now
	[[nodiscard]] std::vector<physics::Ellipsoid> CreatureSkeleton(entt::entity creature);
	/// The list grows by a few slots when it is full, before a body is added to it
	void MakeRoomForOne();
	/// A resting obstacle for an object a moving body may hit
	PhysicsEntry* AddProxy(entt::entity object);
	/// Takes an entry out of the list, the last moving into its place
	void RemoveAt(size_t index);
	[[nodiscard]] std::optional<size_t> IndexOf(entt::entity object) const;
	/// The land as it is now, none without one
	[[nodiscard]] const PhysicsGround* Land();

	/// Where an object goes as it follows its body: to the body's object origin, to its centre (a villager coming to
	/// rest), or it stays where it is and only turns
	enum class Place : uint8_t
	{
		Origin,
		Centre,
		Kept,
	};
	/// The object takes its body's angles, and its place
	void SyncObject(const PhysicsEntry& entry, Place place);
	/// The entry learns what its object's kind does in the physics
	static void TakeKind(PhysicsEntry& entry, const physics_classes::ClassFacts& facts);

	void BeginTurn();
	void WakeNearMovingBodies();
	void Step(const physics::Ground& ground);
	void EndTurn();
	/// What breaks buildings, come to rest or taken out, is no longer passed through by the buildings it struck
	void ForgetAsBuildingHitter(entt::entity object);
	/// The sound and the dust of a knock
	void AttemptCollisionSound(PhysicsEntry& entry);
	void AddLandingDust(glm::vec3 centre, float radius, uint32_t argb);

	[[nodiscard]] PhysicsClassHooks& Hooks();

	/// collision configuration contains default setup for memory, collision setup
	std::unique_ptr<btDefaultCollisionConfiguration> _configuration;
	/// use the default collision dispatcher. For parallel processing you can use
	/// a different dispatcher (see Extras/BulletMultiThreaded)
	std::unique_ptr<btCollisionDispatcher> _dispatcher;
	std::unique_ptr<btDbvtBroadphase> _broadphase;
	/// the default constraint solver. For parallel processing you can use a
	/// different solver (see Extras/BulletMultiThreaded)
	std::unique_ptr<btSequentialImpulseConstraintSolver> _solver;
	std::unique_ptr<btDiscreteDynamicsWorld> _world;

	std::unique_ptr<PhysicsClassHooks> _hooks;
	std::unique_ptr<PhysicsGround> _ground;
	const LandIslandInterface* _groundLand {nullptr};
	/// The bodies, in the order the physics goes through them
	std::vector<std::unique_ptr<PhysicsEntry>> _entries;
	/// The slots the list has room for: it grows only when a body is added to a full list, never while the moving
	/// bodies wake what is near them
	size_t _capacity {0};
	/// While the physics runs its turn, bodies aren't taken out of it from outside
	bool _inTurnUpdate {false};
	physics::turn::SoundPairs _soundPairs;
	std::vector<physics::turn::DustPuff> _dust;
	/// Each creature model's bones' boxes, by the model, worked out once
	std::unordered_map<entt::id_type, std::vector<physics::shapes::BoneBox>> _creatureBoxes;
	/// A collision sound that follows the thing that made it while it plays
	struct FollowingSound
	{
		entt::entity emitter {entt::null};
		entt::entity owner {entt::null};
	};
	std::vector<FollowingSound> _followingSounds;
	/// The system clock's milliseconds, which pick the fly-by whoosh
	std::function<uint64_t()> _ticks {[]() {
		return static_cast<uint64_t>(
		    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
	}};
	/// What the dust's puffs are drawn as
	particles::Creator _dustCreator;
	entt::entity _hitObject {entt::null};
	entt::entity _objectWhichHit {entt::null};
};
} // namespace openblack::ecs::systems
