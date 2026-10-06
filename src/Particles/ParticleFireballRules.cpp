/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The fireball's rules: the throw from the hand (or a lob from another caster), the flight under gravity bouncing off
// the land and off shields, the event it sends its miracle every step, the spin, the fiery trail, its light fading with
// height, the steam where it meets rain or water, and the flags of a ball a shield turned away

#include <cmath>

#include <algorithm>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>

#include "ParticleClassRegistry.h"
#include "ParticleMiracleMaths.h"
#include "ParticleShields.h"
#include "ParticleSounds.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
/// The gravity a lob is worked out for when the group has no gravity rule
constexpr float k_DefaultLobGravity = 30.0f;
/// A throw's share of this speed sizes its sound
constexpr float k_FastestThrowForSound = 200.0f;
/// The wind's pull is scaled down so
constexpr float k_WindScale = 0.1f;

/// A point of the world in an atom's collection's frame
void SetGlobal(const Effect& effect, Atom& atom, const glm::vec3& global)
{
	atom.position = atom.collection != nullptr ? effect.GlobalToLocal(*atom.collection, global) : global;
}

/// The atom, or one of the atoms its collections hang from, was turned away
bool DeflectedInHierarchy(const Atom* atom)
{
	for (; atom != nullptr; atom = atom->collection != nullptr ? atom->collection->parent : nullptr)
	{
		if (atom->deflected)
		{
			return true;
		}
	}
	return false;
}

/// Flight under gravity, pulled by the wind or slowed by damping, bouncing where it strikes the land with a sound sized
/// by how hard it struck, and turned away by the shields it crosses into when asked
class GravityWithFloor final: public Modifier
{
public:
	explicit GravityWithFloor(const ParticleObject& object)
	    : gravity(object.Float("Gravity", 10.0f))
	    , maxSpeed(object.Float("MaxSpeed", 100.0f))
	    , damping(object.Float("Damping", 0.0f))
	    , windMagnification(object.Float("WindMagnification", 100.0f))
	    , useDamping(object.Bool("UseDamping", false))
	    , useWind(object.Bool("UseWind", true))
	    , disableWindForNonHuman(object.Bool("DisableWindForNonHuman", false))
	    , disableDampingForNonHuman(object.Bool("DisableDampingForNonHuman", false))
	    , horizontalBounce(object.Float("DampingHorozontalBounce", 0.5f))
	    , verticalBounce(object.Float("DampingVerticalBounce", 0.5f))
	    , groundDrag(object.Float("GroundDrag", 0.0f))
	    , impactSound({.action = object.Sound("ImpactSound")})
	    , impactSoundCondition(object.String("ImpactSoundCondition"))
	    , impactSmall(object.Float("ImpactSpeedSmall", 5.0f))
	    , impactMedium(object.Float("ImpactSpeedMedium", 20.0f))
	    , impactLarge(object.Float("ImpactSpeedLarge", 40.0f))
	    , minAlpha(object.Int("MinAlphaForImpactSoundOrRipple", 60))
	    , checkShields(object.Bool("CheckShieldDeflections", false))
	{
	}

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		auto& world = effect.Services().world;
		const bool human = effect.IsHumanPlayerCasting();
		const bool damped = useDamping && (!disableDampingForNonHuman || human);
		const bool windy = useWind && (!disableWindForNonHuman || human);
		const float dt = effect.GetDt();
		for (auto& pointer : collection.atoms)
		{
			auto& atom = *pointer;
			auto v = atom.velocity;
			const auto before = effect.GlobalPosition(atom);
			if (windy)
			{
				v += (world.WindAt(before) * windMagnification * k_WindScale - v) * damping * dt;
			}
			else if (damped)
			{
				v *= 1.0f - dt * damping;
			}
			atom.position += v * dt;
			auto p = effect.GlobalPosition(atom);
			const float land = world.LandHeight({p.x, p.z});
			if (p.y >= land)
			{
				// Gravity pulls less once it falls at the greatest speed
				v.y -= std::clamp(v.y + maxSpeed, 0.0f, 1.0f) * gravity * atom.gravity * dt;
			}
			else
			{
				p.y = land;
				SetGlobal(effect, atom, p);
				const auto normal = world.LandNormal({p.x, p.z});
				const float into = glm::dot(v, normal);
				if (into < 0.0f)
				{
					ImpactSound(effect, atom, std::abs(into));
					v = maths::BounceOffSlope(v, normal, groundDrag, dt, horizontalBounce, verticalBounce);
				}
			}
			atom.velocity = v;
			if (checkShields)
			{
				DeflectOffShields(effect, atom, before);
			}
		}
		return true;
	}

	void ImpactSound(Effect& effect, Atom& atom, float speed) const
	{
		if (static_cast<int>(atom.rgba[3]) < minAlpha || impactSound.Silent() || speed < impactSmall ||
		    FindAtomSound(atom, impactSound.action.sound) != nullptr)
		{
			return;
		}
		if (!impactSoundCondition.empty() && !effect.ConditionForAtom(impactSoundCondition, atom))
		{
			return;
		}
		auto sound = impactSound;
		sound.size = SoundSizeFromImpactSpeed(speed, impactMedium, impactLarge);
		StartAtomSound(effect, atom, sound);
	}

	float gravity;
	float maxSpeed;
	float damping;
	float windMagnification;
	bool useDamping;
	bool useWind;
	bool disableWindForNonHuman;
	bool disableDampingForNonHuman;
	float horizontalBounce;
	float verticalBounce;
	float groundDrag;
	ParticleSound impactSound;
	std::string impactSoundCondition;
	float impactSmall;
	float impactMedium;
	float impactLarge;
	int minAlpha;
	bool checkShields;
};

/// The throw: once, the atoms leave the hand. A human player throws them with the hand's movement; any other caster lobs
/// them at the effect's origin under the group's gravity. More than one atom spread round the first on a ring, wider
/// for a faster throw, when asked.
class CreateWithInitialDirection final: public Modifier
{
public:
	explicit CreateWithInitialDirection(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , numAtoms(object.Int("NumAtoms", 1))
	    , sound({.action = object.Sound("SoundOfCreate")})
	    , predictFraction(object.Float("PredictFraction", 0.0f))
	    , elevate(object.Bool("Elevate", false))
	    , predictStartPosition(object.Bool("PredictStartPos", false))
	    , verticalScatterMin(object.Float("VerticalScatterAtMinSpeed", 0.0f))
	    , verticalScatterMax(object.Float("VerticalScatterAtMaxSpeed", 0.0f))
	    , horizontalScatterMin(object.Float("HorozScatterAtMinSpeed", 0.0f))
	    , horizontalScatterMax(object.Float("HorozScatterAtMaxSpeed", 0.0f))
	    , speedRandomFrac(object.Float("SpeedRandomFrac", 0.0f))
	    , initScale(object.String("InitScaleFP"))
	    , lift(object.Float("Elevation", maths::k_ThrowLift))
	    , speeds({.hand = {object.Float("V_In_0", 0.0f), object.Float("V_In_1", 50.0f), object.Float("V_In_2", 450.0f)},
	              .thrown = {object.Float("V_Out_0", 0.0f), object.Float("V_Out_1", 50.0f), object.Float("V_Out_2", 250.0f)}})
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		const auto* atomCreator = effect.FindCreator(creator);
		if (atomCreator == nullptr)
		{
			return false;
		}
		const auto hand = effect.GetProcessInfo().handPosition;
		maths::Launch launch;
		if (effect.IsHumanPlayerCasting())
		{
			launch = maths::HandThrow(effect.GetDirection(), lift, speeds);
		}
		else
		{
			launch = maths::Lob(hand, effect.GetOrigin(), GroupGravity(collection));
		}
		const float share = std::clamp(launch.speed / k_FastestThrowForSound, 0.0f, 1.0f);
		auto played = sound;
		played.size = SoundSizeFromThrow(share);
		auto start = hand;
		if (predictStartPosition)
		{
			start += effect.GetDirection() * effect.GetDt() * predictFraction;
		}
		for (int i = 0; i < numAtoms; ++i)
		{
			auto& atom = effect.NewAtom(collection, atomCreator, nextGroups);
			atom.baseScale *= effect.FloatProvider(initScale, 1.0f);
			auto direction = launch.direction;
			float speed = launch.speed;
			if (elevate && i >= 1)
			{
				float yaw = std::atan2(direction.z, direction.x);
				float pitch = std::atan2(direction.y, std::sqrt(direction.x * direction.x + direction.z * direction.z));
				const float around =
				    static_cast<float>(i - 1) * 2.0f * std::numbers::pi_v<float> / static_cast<float>(numAtoms - 1);
				pitch += std::sin(around) * ((verticalScatterMax - verticalScatterMin) * share + verticalScatterMin);
				yaw += std::cos(around) * ((horizontalScatterMax - horizontalScatterMin) * share + horizontalScatterMin);
				if (speedRandomFrac != 0.0f)
				{
					speed = (1.0f - effect.Random(speedRandomFrac)) * launch.speed;
				}
				pitch = std::clamp(pitch, -std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> / 2.0f);
				direction = {std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch)};
			}
			atom.position = start;
			atom.velocity = direction * speed;
			StartAtomSound(effect, atom, played);
		}
		return false;
	}

	/// The gravity of the group's flight rule
	[[nodiscard]] static float GroupGravity(const Collection& collection)
	{
		for (const auto& slot : collection.modifiers)
		{
			if (const auto* flight = dynamic_cast<const GravityWithFloor*>(slot.modifier))
			{
				return flight->gravity;
			}
		}
		return k_DefaultLobGravity;
	}

	std::string creator;
	std::vector<int> nextGroups;
	int numAtoms;
	ParticleSound sound;
	float predictFraction;
	bool elevate;
	bool predictStartPosition;
	float verticalScatterMin;
	float verticalScatterMax;
	float horizontalScatterMin;
	float horizontalScatterMax;
	float speedRandomFrac;
	std::string initScale;
	float lift;
	maths::ThrowSpeeds speeds;
};

/// Every step, the atom tells its miracle where it is and how it moved, so the miracle can act there
class EventAlways final: public Modifier
{
public:
	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		effect.SendSpellEvent({.type = SpellEventInfo::Type::Point,
		                       .position = effect.GlobalPosition(atom),
		                       .velocity = atom.velocity * effect.GetDt(),
		                       .strength = 1.0f,
		                       .checkShields = false,
		                       .target = entt::null});
		return true;
	}
};

/// The atom is flagged as turned away
class SetAtomHasBeenDeflected final: public Modifier
{
public:
	bool ModifyAtom(Effect& /*effect*/, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		atom.deflected = true;
		return true;
	}
};

/// The ball carries the game object that burns what it passes. The miracle burns round each point the ball tells it
/// of instead (see the fireball miracle), so this rule only marks where that object would be: nothing for it to do.
class AttachFireBallToAtom final: public Modifier
{
};

/// The ball curves as it flies, turning about the vertical by the caster's spin, which fades over a time. Until the
/// hand gives a spin, it flies straight.
class SideSpin final: public Modifier
{
public:
	explicit SideSpin(const ParticleObject& object)
	    : scale(object.Float("ScaleAngularVelocity", 0.0f))
	    , maximum(object.Float("MaxAngularVelocity", 1.0f))
	    , timeToFade(object.Float("TimeToFade", 1.0f))
	{
	}

	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		auto& data = atom.data[this];
		if (!data.started)
		{
			if (effect.GetPlayer() < 0)
			{
				return true;
			}
			const float spin = std::clamp(effect.GetProcessInfo().spin * scale, -maximum, maximum);
			const float share = maximum != 0.0f ? spin / maximum : 0.0f;
			data.a.x = share * share * spin;
			data.started = true;
		}
		const float fade = std::clamp(effect.AtomAge(atom) / timeToFade, 0.0f, 1.0f);
		const float angle = data.a.x * (1.0f - fade) * effect.GetDt();
		const float c = std::cos(angle);
		const float s = std::sin(angle);
		const auto v = atom.velocity;
		atom.velocity = {c * v.x - s * v.z, v.y, c * v.z + s * v.x};
		return true;
	}

	float scale;
	float maximum;
	float timeToFade;
};

/// The alpha goes from one value on the land to another at a height above it and beyond
class FadeAlphaWithHeight final: public Modifier
{
public:
	explicit FadeAlphaWithHeight(const ParticleObject& object)
	    : alphaAtZero(static_cast<float>(object.Int("AlphaAtZero", 255)))
	    , alphaAtReference(static_cast<float>(object.Int("AlphaAtRefHeight", 255)))
	    , referenceHeight(object.Float("RefHeight", 1.0f))
	{
	}

	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		const auto p = effect.GlobalPosition(atom);
		const float height = p.y - effect.Services().world.LandHeight({p.x, p.z});
		float alpha = alphaAtReference;
		if (height < 0.0f)
		{
			alpha = alphaAtZero;
		}
		else if (height <= referenceHeight)
		{
			alpha = (alphaAtReference - alphaAtZero) * height / referenceHeight + alphaAtZero;
		}
		atom.rgba[3] = static_cast<uint8_t>(std::clamp(alpha, 0.0f, 255.0f));
		return true;
	}

	float alphaAtZero;
	float alphaAtReference;
	float referenceHeight;
};

/// Once for each atom, the groups' collections are made under it
class AddSubCollectionsToAtom final: public Modifier
{
public:
	explicit AddSubCollectionsToAtom(const ParticleObject& object)
	    : nextGroups(object.IntArray("NextGroups"))
	{
	}

	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		auto& data = atom.data[this];
		if (!data.started)
		{
			data.started = true;
			effect.AddSubCollections(atom, nextGroups);
		}
		return true;
	}

	std::vector<int> nextGroups;
};

/// A trail of atoms behind the parent atom along where it was over the last turns, no longer than a length, spaced
/// closer towards the head or evenly, fading and shrinking towards the tail. The first and last atoms are only points
/// when the head or tail group is unset.
class Trail final: public Modifier
{
public:
	explicit Trail(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , maxTrailLength(object.Float("MaxTrailLength", 10.0f))
	    , numAtoms(object.Int("NumAtoms", 5))
	    , turns(object.Int("NumGameTurnsToSpreadOver", 10))
	    , headGroup(object.Int("HeadGroup", -1))
	    , trailGroup(object.Int("TrailGroup", -1))
	    , nonLinear(object.Bool("UseNonLinearSpacing", true))
	    , useAlpha(object.Bool("UseAlpha", true))
	    , useScaling(object.Bool("UseScaling", true))
	    , combinedParentScale(object.Bool("UseCombinedParentScale", false))
	    , useParentAlpha(object.Bool("UseParentAlpha", false))
	    , modifyAlpha(object.Bool("ModifyAlpha", true))
	    , modifyScaling(object.Bool("ModifyScaling", true))
	    , initUsingVelocity(object.Bool("InitTrailUsingVelocity", false))
	    , scaleLengthWithParent(object.Bool("ScaleMaxTrailLengthWithParent", false))
	    , fadeTailAlpha(object.Float("FadeTailAlpha", 1.0f))
	    , fadeTailScale(object.Float("FadeTailScale", 1.0f))
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	struct Sample
	{
		glm::vec3 position {0.0f};
		float scale {1.0f};
		glm::mat3 rotation {1.0f};
	};
	/// Where the parent was over the last turns, newest at the head of the ring
	struct History
	{
		std::vector<Sample> samples;
		int count {0};
		int head {-1};

		/// The i-th newest
		[[nodiscard]] const Sample& At(int i) const
		{
			const int capacity = static_cast<int>(samples.size());
			return samples[static_cast<size_t>(((head - i) % capacity + capacity) % capacity)];
		}
		void Push(const Sample& sample)
		{
			const int capacity = static_cast<int>(samples.size());
			head = (head + 1) % capacity;
			count = std::min(count + 1, capacity);
			samples[static_cast<size_t>(head)] = sample;
		}
	};

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		if (slot.first)
		{
			slot.first = false;
			History history;
			history.samples.assign(static_cast<size_t>(std::max(turns, 1)), Sample {});
			slot.data = history;
			const auto* atomCreator = effect.FindCreator(creator);
			for (int i = 0; i < numAtoms; ++i)
			{
				const bool point = i == 0 ? headGroup < 0 : (i == numAtoms - 1 && trailGroup < 0);
				effect.NewAtom(collection, point ? nullptr : atomCreator,
				               point ? std::span<const int> {} : std::span<const int>(nextGroups));
			}
			return true;
		}
		auto* history = std::any_cast<History>(&slot.data);
		const Atom* parent = collection.parent;
		if (history == nullptr || parent == nullptr)
		{
			return true;
		}
		const float parentScale = parent->ruleScale * (combinedParentScale ? parent->baseScale : 1.0f);
		const Sample sample {.position = parent->position, .scale = parentScale, .rotation = parent->rotation};
		if (initUsingVelocity && history->count == 0)
		{
			const auto back = parent->velocity * effect.GetDt();
			for (int i = static_cast<int>(history->samples.size()) - 1; i >= 0; --i)
			{
				history->Push({sample.position - back * static_cast<float>(i), sample.scale, sample.rotation});
			}
		}
		else
		{
			history->Push(sample);
		}
		Lay(collection, *history, parentScale, *parent);
		return true;
	}

	void Lay(Collection& collection, const History& history, float parentScale, const Atom& parent) const
	{
		// How much of the history the length covers
		const float maximum = scaleLengthWithParent ? parentScale * maxTrailLength : maxTrailLength;
		float coverage = 1.0f;
		float covered = 0.0f;
		for (int i = 0; i + 1 < history.count; ++i)
		{
			const float segment = glm::length(history.At(i).position - history.At(i + 1).position);
			if (segment + covered > maximum)
			{
				coverage = ((maximum - covered) / segment + static_cast<float>(i)) / static_cast<float>(history.count);
				break;
			}
			covered += segment;
		}
		const int n = static_cast<int>(collection.atoms.size());
		const float half = 0.5f * static_cast<float>(n - 1) * static_cast<float>(n - 1);
		const float span = static_cast<float>(history.count - 1);
		for (int k = 0; k < n; ++k)
		{
			auto& atom = *collection.atoms[static_cast<size_t>(k)];
			const float t = nonLinear
			                    ? (half > 0.0f ? (half - 0.5f * static_cast<float>(k * k)) * coverage / half * span : 0.0f)
			                    : static_cast<float>(n - k) * coverage / static_cast<float>(n) * span;
			const int index = static_cast<int>(std::floor(t));
			const float fraction = t - static_cast<float>(index);
			Sample at {.position = glm::vec3(0.0f), .scale = 0.0f, .rotation = glm::mat3(1.0f)};
			if (index >= 0 && index < history.count)
			{
				const auto& a = history.At(index);
				at = a;
				if (index < history.count - 1)
				{
					const auto& b = history.At(index + 1);
					at.position = a.position + (b.position - a.position) * fraction;
					at.scale = a.scale + (b.scale - a.scale) * fraction;
				}
			}
			atom.position = at.position;
			atom.ruleScale = at.scale;
			atom.rotation = at.rotation;
			if (useParentAlpha)
			{
				const float collectionAlpha = parent.collection != nullptr ? parent.collection->alpha : 255.0f;
				atom.rgba[3] =
				    static_cast<uint8_t>((static_cast<int>(parent.rgba[3]) * static_cast<int>(collectionAlpha)) >> 8);
			}
			else if (modifyAlpha)
			{
				atom.rgba[3] = 255;
			}
			// Faded and shrunk towards the tail (k = 0), the head as the fade-tail values have it
			const float share = n > 1 ? static_cast<float>(k) / static_cast<float>(n - 1) : 1.0f;
			if (useScaling)
			{
				atom.ruleScale = share * fadeTailScale;
			}
			else if (modifyScaling)
			{
				atom.ruleScale *= fadeTailScale * share;
			}
			const float headAlpha = k == n - 1 ? 255.0f : fadeTailAlpha * 255.0f;
			if (useAlpha)
			{
				atom.rgba[3] = static_cast<uint8_t>(std::clamp(headAlpha * share, 0.0f, 255.0f));
			}
			else if (modifyAlpha)
			{
				atom.rgba[3] = static_cast<uint8_t>(
				    std::clamp(headAlpha * static_cast<float>(atom.rgba[3]) * share / 255.0f, 0.0f, 255.0f));
			}
		}
	}

	std::string creator;
	std::vector<int> nextGroups;
	float maxTrailLength;
	int numAtoms;
	int turns;
	int headGroup;
	int trailGroup;
	bool nonLinear;
	bool useAlpha;
	bool useScaling;
	bool combinedParentScale;
	bool useParentAlpha;
	bool modifyAlpha;
	bool modifyScaling;
	bool initUsingVelocity;
	bool scaleLengthWithParent;
	float fadeTailAlpha;
	float fadeTailScale;
};
} // namespace

void openblack::particles::RegisterFireballRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UpdateRuleGravityWithFloor", ParticleClassRegistry::Make<GravityWithFloor>);
	registry.AddModifier("CreateWithInitialDirection", ParticleClassRegistry::Make<CreateWithInitialDirection>);
	registry.AddModifier("AttatchFireBallToAtom", ParticleClassRegistry::Make<AttachFireBallToAtom>);
	registry.AddModifier("EventAlways", ParticleClassRegistry::Make<EventAlways>);
	registry.AddModifier("SetAtomHasBeenDeflected", ParticleClassRegistry::Make<SetAtomHasBeenDeflected>);
	registry.AddModifier("UR_SideSpin", ParticleClassRegistry::Make<SideSpin>);
	registry.AddModifier("AR_FadeAlphaWithHeightAboveLandscape", ParticleClassRegistry::Make<FadeAlphaWithHeight>);
	registry.AddModifier("AddSubCollectionsToAtom", ParticleClassRegistry::Make<AddSubCollectionsToAtom>);
	registry.AddModifier("UR_Trail", ParticleClassRegistry::Make<Trail>);

	registry.AddAtomCondition("EventConditionAtomHasBeenDeflected",
	                          [](const Effect&, const ParticleObject&, const Atom& atom) { return atom.deflected; });
	registry.AddAtomCondition("EC_DeflectionInAtomsHierarchy", [](const Effect&, const ParticleObject&, const Atom& atom) {
		return DeflectedInHierarchy(&atom);
	});
	registry.AddCollectionCondition("EC_DeflectionInCollectionsHierarchy",
	                                [](const Effect&, const ParticleObject&, const Collection& collection) {
		                                return DeflectedInHierarchy(collection.parent);
	                                });
	// Near the water: not turned away, below a height above the land, over the sea
	registry.AddAtomCondition(
	    "EventConditionAtomCloseWater", [](const Effect& effect, const ParticleObject& object, const Atom& atom) {
		    if (atom.deflected)
		    {
			    return false;
		    }
		    const auto p = effect.GlobalPosition(atom);
		    const auto& world = effect.Services().world;
		    return p.y - world.LandHeight({p.x, p.z}) < object.Float("CutOffHeight", 0.0f) && world.IsWater(p);
	    });
	// In the rain: not turned away, where it rains or snows
	registry.AddAtomCondition("EventConditionFireBallSteam", [](const Effect& effect, const ParticleObject&, const Atom& atom) {
		return !atom.deflected && effect.Services().world.IsRainingAt(effect.GlobalPosition(atom));
	});
}
