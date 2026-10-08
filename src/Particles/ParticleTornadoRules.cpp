/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The tornado's rule. Its foot wanders on the land under the storm's clouds, which it stands up to; each step it hurts
// what is at its foot, and every few turns it picks up one thing it reaches: a villager, a tree, a pot (a pile gives up
// some of what it holds as a pot), while a creature is caught helpless instead. What it picks up orbits the funnel, is
// pulled to its wall and rises, and is flung out near the top to fall. Two spinning shells make the funnel, dust is
// thrown up at its foot and bushes and chickens it pretends to have picked up whirl about it. It fades in, and out once
// the storm closes down.

#include <cmath>

#include <algorithm>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include <ParticleFile.h>

#include "ParticleClassRegistry.h"
#include "ParticleSounds.h"
#include "TornadoMaths.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// What flies round the funnel
enum class Flying : uint8_t
{
	/// Something picked up: a game object it carries
	Carried = 0,
	/// A bush or a chicken it only pretends to have picked up
	Pretend = 1,
	/// A puff of cloud on the wall, which fades with the tornado
	Puff = 2,
};
/// The height drawn to grows by this share a second
constexpr float k_TargetRise = 0.1f;
/// Things are flung out once above both of these shares of the height
constexpr float k_FlingShare = 0.7f;
constexpr float k_FlingShareHigh = 0.9f;
/// A puff goes once the tornado has faded below this
constexpr float k_PuffGone = 0.0001f;
/// The pretend objects are thrown only while the tornado is this opaque
constexpr uint8_t k_PretendAlpha = 250;
/// The pretend objects pick their model by a random number against these
constexpr float k_SmallShare = 0.33f;
constexpr float k_MediumShare = 0.66f;
/// Launch speeds are between these shares of the full speed
constexpr float k_LeastLaunch = 0.33f;
constexpr float k_MostLaunch = 0.66f;
/// What is picked up starts drawn to a random height up to this share, and something's orbit factor is 0.5..1.5
constexpr float k_CarriedTarget = 0.7f;
constexpr float k_OrbitBase = 0.5f;
/// The dust stops at this many atoms
constexpr size_t k_MostDebris = 50;
/// The shells spin at speeds this much apart
constexpr float k_ShellSpread = 0.13f;
/// It looks for something to pick up every this many turns
constexpr uint32_t k_SearchEvery = 3;
/// The pot made from a pile is between these of its size
constexpr float k_PotSizeLeast = 0.7f;
constexpr float k_PotSizeMost = 1.2f;

/// What the rule keeps for its collection
struct TornadoState
{
	bool first {true};
	bool closing {false};
	glm::vec3 base {0.0f};
	glm::vec3 top {0.0f};
	glm::vec3 baseVelocity {0.0f};
	glm::vec3 topVelocity {0.0f};
	float closeFade {1.0f};
	uint8_t alpha {0};
	float pretendMade {0.0f};
	float pretendDue {0.0f};
	float debrisMade {0.0f};
	float debrisDue {0.0f};
};

/// What it keeps for each flying atom
struct FlyingData
{
	Flying type {Flying::Puff};
	/// The share of the height it is drawn to
	float target {0.0f};
	float orbitFactor {1.0f};
};

class Tornado final: public Modifier
{
public:
	explicit Tornado(const ParticleObject& object)
	    : topHeight(object.Float("TopHeight", 100.0f))
	    , fadeOutTime(object.Float("FadeOutTime", 3.0f))
	    , fadeInTime(object.Float("FadeInTime", 8.0f))
	    , meshSpin(object.Float("MeshThetaDot", 6.0f))
	    , pretendRise(0.3f)
	    , rise(0.1f)
	    , topMoveFrequency(object.Float("TopMoveFreq", 0.25f))
	    , topMoveAmplitude(object.Float("TopMoveAmp", 30.0f))
	    , delayBeforeMove(object.Float("DelayBeforeMove", 5.0f))
	    , damping(object.Float("Damping", 0.1f))
	    , velocityField(object.Bool("ShowVelocityField", true))
	    , groupFlying(object.Int("GroupFlying", -1))
	    , groupDebris(object.Int("GroupDebris", -1))
	    , groupMesh(object.Int("GroupMesh", -1))
	    , groupOnCloseDown(object.Int("GroupToMoveToOnCloseDown", -1))
	    , groupOnceDone(object.Int("GroupToMoveToOnceDone", -1))
	    , puffs(object.Int("NumAtomsToCreate", 0))
	    , creator(object.String("PCreator"))
	    , puffCreator(object.String("SpriteCreator"))
	    , debrisCreator(object.String("DebrisSpriteCreator"))
	    , debrisRate(object.Float("DebrisEmitRate", 10.0f))
	    , debrisHeight(object.Float("DebrisHeight", 10.0f))
	    , debrisRadius(object.Float("DebrisRadius", 10.0f))
	    , debrisSpeed(object.Float("DebrisSpeed", 20.0f))
	    , debrisSpread(object.Float("DebrisSpreadAngle", 1.03673f))
	    , pretendRate(object.Float("PretendEmitRate", 2.0f))
	    , pretendGravity(object.Float("PretendGravity", 10.0f))
	    , pretendBlend(object.Float("PretendBlendTime", 4.0f))
	    , pretendRadius(object.Float("PretendRadius", 10.0f))
	    , pretendSpeed(object.Float("PretendSpeed", 100.0f))
	    , pretendSpread(object.Float("PretendSpreadAngle", 1.03673f))
	    , pretendSmall(object.String("PretendObjectCreatorSmall"))
	    , pretendMedium(object.String("PretendObjectCreatorMedium"))
	    , pretendLarge(object.String("PretendObjectCreatorLarge"))
	    , resourceLeast(object.Float("ResourceAmountRemoveMin", 150.0f))
	    , resourceMost(object.Float("ResourceAmountRemoveMax", 650.0f))
	    , scaleProvider(object.String("TornadoScaleFloatProvider"))
	    , sound({.action = object.Sound("SoundTornado")})
	{
		funnel.baseRadius = object.Float("BaseRadius", funnel.baseRadius);
		funnel.topRadius = object.Float("TopRadius", funnel.topRadius);
		funnel.baseScale = object.Float("BaseScale", funnel.baseScale);
		funnel.topScale = object.Float("TopScale", funnel.topScale);
		funnel.baseSpin = object.Float("BaseThetaDot", funnel.baseSpin);
		funnel.topSpin = object.Float("TopThetaDot", funnel.topSpin);
		funnel.spinBias = object.Float("ThetaBias", funnel.spinBias);
		funnel.bend = object.Float("FunnelBendParameter", funnel.bend);
		funnel.wiggleCount = object.Int("WiggleCount", funnel.wiggleCount);
		funnel.wiggleAmplitude = object.Float("WiggleAmplitude", funnel.wiggleAmplitude);
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		if (!slot.data.has_value())
		{
			slot.data = TornadoState {};
		}
		auto& state = *std::any_cast<TornadoState>(&slot.data);
		tornado::Funnel shape = funnel;
		// Its size from the storm's, 1 without one
		shape.scale = scaleProvider.empty() ? 1.0f : effect.FloatProvider(scaleProvider, 1.0f);
		UpdateFootAndTop(effect, collection, state, shape);
		if (state.first)
		{
			const auto* rootCreator = effect.FindCreator(creator);
			if (rootCreator == nullptr)
			{
				return true;
			}
			// The tornado's own atom carries the flying things, the dust and the shells, and its roar
			const std::vector<int> groups {groupFlying, groupDebris, groupMesh};
			auto& root = effect.NewAtom(collection, rootCreator, groups);
			StartAtomSound(effect, root, sound);
			root.position = state.base;
		}
		if (collection.parent != nullptr)
		{
			collection.parent->position = effect.GlobalToLocal(*collection.parent->collection, state.base);
		}
		if (!collection.atoms.empty())
		{
			auto& root = *collection.atoms.front();
			root.position = state.base;
			for (size_t s = 0; s < root.subCollections.size(); ++s)
			{
				auto& sub = *root.subCollections[s];
				if (sub.group == groupFlying)
				{
					UpdateFlying(effect, collection, sub, state, shape);
				}
				else if (sub.group == groupMesh)
				{
					UpdateMesh(effect, sub, state, shape);
				}
				else if (sub.group == groupDebris)
				{
					UpdateDebris(effect, sub, state, shape);
				}
			}
		}
		state.first = false;
		return true;
	}

private:
	/// The top follows the clouds once it may move, the foot wanders under it on the land, and while open the foot strikes
	/// what is there
	void UpdateFootAndTop(Effect& effect, const Collection& collection, TornadoState& state, const tornado::Funnel& shape) const
	{
		state.closing = state.closing || effect.Closing();
		state.closeFade = state.closing ? tornado::CloseFade(effect.GetAge() - effect.GetCloseAge(), fadeOutTime) : 1.0f;
		const float age = effect.CollectionAge(collection);
		state.alpha = tornado::FadeAlpha(age, fadeInTime, state.closeFade);
		if (state.first)
		{
			state.top = effect.GetOrigin();
			state.base = state.top;
		}
		const auto oldBase = state.base;
		const auto oldTop = state.top;
		if (age >= delayBeforeMove && collection.parent != nullptr)
		{
			state.top = effect.GlobalPosition(*collection.parent);
		}
		state.base = state.top;
		const auto& noise = effect.Services().noise;
		const auto times = tornado::FootWanderTimes(age, topMoveFrequency);
		const auto wander = tornado::FootWander(noise.Smooth(times.a), noise.Smooth(times.b), noise.Smooth(times.c),
		                                        noise.Smooth(times.d), topMoveAmplitude, shape.scale);
		state.base.x += wander.x;
		state.base.z += wander.y;
		auto& world = effect.Services().world;
		state.base.y = world.LandHeight({state.base.x, state.base.z});
		effect.SetOrigin(state.base);
		state.top.y = shape.scale * topHeight + state.base.y;
		const float dt = effect.GetDt();
		if (state.first || !(dt > 0.0f))
		{
			state.baseVelocity = glm::vec3(0.0f);
			state.topVelocity = glm::vec3(0.0f);
		}
		else
		{
			state.baseVelocity = (state.base - oldBase) / dt;
			state.topVelocity = (state.top - oldTop) / dt;
		}
		if (!state.closing)
		{
			effect.SendSpellEvent({.type = SpellEventInfo::Type::Point,
			                       .position = state.base,
			                       .velocity = state.baseVelocity,
			                       .strength = 1.0f,
			                       .checkShields = false,
			                       .target = entt::null});
		}
	}

	/// What it keeps for a flying atom: a puff until told otherwise
	[[nodiscard]] FlyingData Read(const Atom& atom) const
	{
		const auto found = atom.data.find(this);
		if (found == atom.data.end() || !found->second.started)
		{
			return {};
		}
		const auto& a = found->second.a;
		return {.type = static_cast<Flying>(static_cast<int>(a.x)), .target = a.y, .orbitFactor = a.z};
	}
	void Write(Atom& atom, const FlyingData& data) const
	{
		auto& kept = atom.data[this];
		kept.started = true;
		kept.a = {static_cast<float>(data.type), data.target, data.orbitFactor, 0.0f};
	}

	void UpdateFlying(Effect& effect, const Collection& tornado, Collection& flying, TornadoState& state,
	                  const tornado::Funnel& shape) const
	{
		flying.alpha = state.alpha;
		if (state.first)
		{
			MakePuffs(effect, flying, state, shape);
		}
		EmitPretend(effect, flying, state, shape);
		PickUp(effect, tornado, flying, state, shape);

		const float dt = effect.GetDt();
		if (!(dt > 0.0f))
		{
			return;
		}
		std::vector<std::pair<Atom*, int>> leaving;
		for (auto& owned : flying.atoms)
		{
			auto& atom = *owned;
			auto data = Read(atom);
			const float h = tornado::HeightShare(atom.position.y, state.base.y, state.top.y);
			if (data.type != Flying::Puff)
			{
				if (state.closing)
				{
					leaving.emplace_back(&atom, groupOnCloseDown);
					continue;
				}
				data.target = std::clamp(data.target + dt * k_TargetRise, 0.0f, 1.0f);
				Write(atom, data);
				if (h > k_FlingShare && h > k_FlingShareHigh)
				{
					leaving.emplace_back(&atom, groupOnceDone);
					continue;
				}
			}
			Orbit(effect, atom, data, state, shape, h, dt);
			if (data.type == Flying::Puff)
			{
				if (state.closeFade < k_PuffGone)
				{
					leaving.emplace_back(&atom, -1);
					continue;
				}
				atom.ruleScale = tornado::WallScale(shape, h) * state.closeFade;
			}
		}
		for (const auto& [atom, destination] : leaving)
		{
			if (destination >= 0)
			{
				effect.MoveToGroup(*atom, destination);
			}
			else
			{
				// Apple's clang can't capture a structured binding: take a plain copy of it
				const auto* gone = atom;
				std::erase_if(flying.atoms, [gone](const auto& owned) { return owned.get() == gone; });
			}
		}
	}

	/// One step round the funnel: pulled to the wall, spun round the axis, risen towards its height, and carried with the
	/// axis as it moves
	void Orbit(Effect& effect, Atom& atom, const FlyingData& data, const TornadoState& state, const tornado::Funnel& shape,
	           float h, float dt) const
	{
		const auto axis = tornado::AxisCentre(shape, state.base, state.top, h);
		const auto offset = atom.position - axis;
		const float distance = std::sqrt(offset.x * offset.x + offset.z * offset.z);
		const float wall = tornado::WallRadius(shape, h);
		const float spin = tornado::SpinRate(shape, h, data.orbitFactor);
		const float pulled = tornado::RadialRate(wall, distance, dt) * dt + distance;
		const float riseSpeed = tornado::RiseSpeed(atom.position.y, state.base.y, state.top.y, data.target,
		                                           data.type == Flying::Carried ? rise : pretendRise);
		const float angle = std::atan2(offset.z, offset.x) + tornado::SpinOutside(spin, wall, pulled) * dt;
		const auto axisVelocity = state.baseVelocity + (state.topVelocity - state.baseVelocity) * h;
		const float invDt = 1.0f / dt;
		const glm::vec3 wanted {
		    (axis.x + std::cos(angle) * pulled - atom.position.x) * invDt + axisVelocity.x,
		    ((riseSpeed * dt + offset.y + axis.y) - atom.position.y) * invDt + axisVelocity.y,
		    (axis.z + std::sin(angle) * pulled - atom.position.z) * invDt + axisVelocity.z,
		};
		if (data.type == Flying::Pretend)
		{
			// Thrown up by gravity at first, then more and more by the funnel
			const glm::vec3 gravity {0.0f, -pretendGravity, 0.0f};
			const auto toOrbit = (wanted - atom.velocity) * invDt;
			const float blend = pretendBlend > 0.0f ? std::clamp(effect.AtomAge(atom) / pretendBlend, 0.0f, 1.0f) : 1.0f;
			atom.velocity += (gravity + (toOrbit - gravity) * blend) * dt;
		}
		else if (velocityField)
		{
			atom.velocity = wanted;
		}
		else
		{
			atom.velocity += (wanted - atom.velocity) * damping * dt;
		}
		atom.position += atom.velocity * dt;
	}

	/// Puffs of cloud up the wall at the start, from the top down
	void MakePuffs(Effect& effect, Collection& flying, const TornadoState& state, const tornado::Funnel& shape) const
	{
		const auto* puff = effect.FindCreator(puffCreator);
		if (puff == nullptr || puffs <= 0)
		{
			return;
		}
		for (int k = 0; k < puffs; ++k)
		{
			auto& atom = effect.NewAtom(flying, puff, {});
			FlyingData data {.type = Flying::Puff};
			data.orbitFactor = effect.Random(1.0f) + k_OrbitBase;
			data.target = static_cast<float>(puffs - k) / static_cast<float>(puffs);
			Write(atom, data);
			const auto axis = tornado::AxisCentre(shape, state.base, state.top, data.target);
			const float angle = effect.Random(k_TwoPi);
			const float wall = tornado::WallRadius(shape, data.target);
			atom.position = axis + glm::vec3(std::cos(angle) * wall, 0.0f, std::sin(angle) * wall);
		}
	}

	/// Bushes and chickens thrown up from the foot while it is on dry land
	void EmitPretend(Effect& effect, Collection& flying, TornadoState& state, const tornado::Funnel& shape) const
	{
		const auto* small = effect.FindCreator(pretendSmall);
		const auto* medium = effect.FindCreator(pretendMedium);
		const auto* large = effect.FindCreator(pretendLarge);
		if (small == nullptr || medium == nullptr || large == nullptr || state.closing || state.alpha <= k_PretendAlpha)
		{
			return;
		}
		auto& world = effect.Services().world;
		const float rate = world.IsDryLand(state.base) ? shape.scale * pretendRate : 0.0f;
		state.pretendDue += rate * effect.GetDt();
		while (state.pretendMade < state.pretendDue)
		{
			state.pretendMade += 1.0f;
			const float pick = effect.Random(1.0f);
			const auto* chosen = pick < k_SmallShare ? small : (pick < k_MediumShare ? medium : large);
			auto& atom = effect.NewAtom(flying, chosen, {});
			const float z = effect.Random(-pretendRadius, pretendRadius);
			const float x = effect.Random(-pretendRadius, pretendRadius);
			if (shape.scale < 1.0f)
			{
				atom.baseScale *= shape.scale;
			}
			atom.position = {x * shape.scale + state.base.x, state.base.y, z * shape.scale + state.base.z};
			const float fromVertical = effect.Random(pretendSpread);
			const float heading = effect.Random(k_TwoPi);
			const float share = effect.Random(k_LeastLaunch, k_MostLaunch);
			atom.velocity = tornado::Launch(fromVertical, heading, share, pretendSpeed, shape.scale, state.baseVelocity);
			const float orbitFactor = effect.Random(1.0f) + k_OrbitBase;
			Write(atom, {.type = Flying::Pretend, .target = 1.0f, .orbitFactor = orbitFactor});
		}
	}

	/// Every few turns once faded in, the first thing in reach it can pick up: the living, trees and things lying about
	/// that fit the funnel and the miracle may destroy, or a pot of a pile's food or wood; a creature in reach is caught
	/// helpless
	void PickUp(Effect& effect, const Collection& tornado, Collection& flying, const TornadoState& state,
	            const tornado::Funnel& shape) const
	{
		auto& world = effect.Services().world;
		if (effect.CollectionAge(tornado) < fadeInTime || state.closing || world.GameTurn() % k_SearchEvery != 0)
		{
			return;
		}
		const float tribalPower = effect.GetSink() != nullptr ? effect.GetSink()->TribalPower() : 1.0f;
		const auto takePot = [&](entt::entity pile) {
			// So much of it, rounded to the nearest, a half to the even number
			const auto amount =
			    static_cast<uint32_t>(std::nearbyint(tornado::PileTake(shape.scale, resourceLeast, resourceMost)));
			const float size = tornado::PotScale(shape.scale, effect.Random(k_PotSizeLeast, k_PotSizeMost));
			const auto pot = world.TakeFromPile(pile, amount, size);
			return pot != entt::null && Lift(effect, flying, pot);
		};
		for (const auto& candidate : world.TornadoCandidates(state.base, tornado::Reach(shape, tribalPower)))
		{
			switch (candidate.kind)
			{
			case TornadoCandidate::Kind::Liftable:
				if (tornado::Fits(shape, candidate.radius))
				{
					// Turned down by the miracle, it is passed over
					if (effect.SendSpellEvent({.type = SpellEventInfo::Type::Capture,
					                           .position = candidate.position,
					                           .velocity = glm::vec3(0.0f),
					                           .strength = 1.0f,
					                           .checkShields = false,
					                           .target = candidate.object}) &&
					    Lift(effect, flying, candidate.object))
					{
						return;
					}
				}
				else if (candidate.pile && takePot(candidate.object))
				{
					return;
				}
				break;
			case TornadoCandidate::Kind::Creature:
				world.CatchCreature(candidate.object);
				break;
			case TornadoCandidate::Kind::Pile:
				if (takePot(candidate.object))
				{
					return;
				}
				break;
			}
		}
	}

	/// An atom of the flying group carries the object from where it stands
	bool Lift(Effect& effect, Collection& flying, entt::entity object) const
	{
		auto& world = effect.Services().world;
		const auto where = world.Target(object, false);
		auto carried = std::make_shared<CarriedObject>(CarriedObject {.object = object});
		const auto rotation = world.Carry(carried);
		if (!rotation.has_value())
		{
			return false;
		}
		auto& atom = effect.NewAtom(flying, nullptr, {});
		atom.position = where.has_value() ? where->position : effect.GetOrigin();
		atom.rotation = *rotation;
		atom.carried = carried;
		carried->atom = &atom;
		carried->position = atom.position;
		carried->rotation = atom.rotation;
		const float target = effect.Random(k_CarriedTarget);
		const float orbitFactor = effect.Random(1.0f) + k_OrbitBase;
		Write(atom, {.type = Flying::Carried, .target = target, .orbitFactor = orbitFactor});
		return true;
	}

	/// The two shells at the foot, as big as the tornado, each spinning a little faster than the last
	void UpdateMesh(Effect& effect, Collection& mesh, const TornadoState& state, const tornado::Funnel& shape) const
	{
		mesh.alpha = state.alpha;
		const float spin = meshSpin * shape.spinMultiplier;
		float index = 0.0f;
		for (auto& atom : mesh.atoms)
		{
			atom->rotation = maths::TurnAboutAxis(atom->rotation, 1, (index * k_ShellSpread + 1.0f) * effect.GetDt() * spin);
			atom->position = state.base;
			atom->ruleScale = shape.scale;
			index += 1.0f;
		}
	}

	/// Dust thrown up round the foot, the colour of the land there, no more than so many at once
	void UpdateDebris(Effect& effect, Collection& debris, TornadoState& state, const tornado::Funnel& shape) const
	{
		const auto* dust = effect.FindCreator(debrisCreator);
		if (dust == nullptr)
		{
			return;
		}
		debris.alpha = state.alpha;
		state.debrisDue += effect.GetDt() * debrisRate;
		if (state.closing)
		{
			return;
		}
		const auto land = effect.Services().world.LandColour({state.base.x, state.base.z});
		while (state.debrisMade < state.debrisDue)
		{
			if (debris.atoms.size() >= k_MostDebris)
			{
				return;
			}
			state.debrisMade += 1.0f;
			auto& atom = effect.NewAtom(debris, dust, {});
			for (size_t c = 0; c < 3; ++c)
			{
				const auto shade = static_cast<uint32_t>(land[static_cast<glm::length_t>(c)]);
				atom.rgba.at(c) = static_cast<uint8_t>((static_cast<uint32_t>(atom.rgba.at(c)) * shade) >> 8u);
			}
			const float z = effect.Random(-debrisRadius, debrisRadius);
			const float y = effect.Random(debrisHeight);
			const float x = effect.Random(-debrisRadius, debrisRadius);
			atom.position = {x * shape.scale + state.base.x, shape.scale * y + state.base.y, shape.scale * z + state.base.z};
			atom.baseScale *= shape.scale;
			const float fromVertical = effect.Random(debrisSpread);
			const float heading = effect.Random(k_TwoPi);
			const float share = effect.Random(k_LeastLaunch, k_MostLaunch);
			atom.velocity = tornado::Launch(fromVertical, heading, share, debrisSpeed, shape.scale, state.baseVelocity);
		}
	}

	tornado::Funnel funnel;
	float topHeight;
	float fadeOutTime;
	float fadeInTime;
	float meshSpin;
	float pretendRise;
	float rise;
	float topMoveFrequency;
	float topMoveAmplitude;
	float delayBeforeMove;
	float damping;
	bool velocityField;
	int groupFlying;
	int groupDebris;
	int groupMesh;
	int groupOnCloseDown;
	int groupOnceDone;
	int puffs;
	std::string creator;
	std::string puffCreator;
	std::string debrisCreator;
	float debrisRate;
	float debrisHeight;
	float debrisRadius;
	float debrisSpeed;
	float debrisSpread;
	float pretendRate;
	float pretendGravity;
	float pretendBlend;
	float pretendRadius;
	float pretendSpeed;
	float pretendSpread;
	std::string pretendSmall;
	std::string pretendMedium;
	std::string pretendLarge;
	float resourceLeast;
	float resourceMost;
	std::string scaleProvider;
	ParticleSound sound;
};
} // namespace

void openblack::particles::RegisterTornadoRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_Tornado", ParticleClassRegistry::Make<Tornado>);
}
