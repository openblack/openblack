/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The storm miracle's rules. Five cores start at the point it is cast, each with a collection of clouds gathering round
// it: the clouds are born far out, circle in and fade away, each living a few seconds before it is born again elsewhere.
// The first core's clouds lay the storm's rain and wind over the land and, for the tornado, raise the funnel. The cores
// drift with the wind, and the miracle with them. Powered up, the clouds' older members let fly bolts of lightning now
// and then, flashing and thundering. Where the storm is cast, a swirl of dark sprites tightens and drifts off the way it
// was thrown, and in the hand flashes of lightning crackle about the cloud.

#include <cmath>

#include <algorithm>
#include <limits>
#include <memory>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>

#include "ParticleClassRegistry.h"
#include "ParticleShields.h"
#include "ParticleSounds.h"
#include "StormMaths.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// A cloud past this share of its life may let fly a bolt
constexpr float k_StrikingShare = 0.5f;
/// The land under the clouds is looked at this share of their radius either side of the middle
constexpr float k_GroundSampleSpacing = 0.33f;
/// The tornado's power-up level, whose clouds stand at the height of the funnel's foot
constexpr int k_TornadoLevel = 1;
/// A float provider the file doesn't name gives this
constexpr float k_Missing = std::numeric_limits<float>::quiet_NaN();

/// The way the storm is pushed: the way the camera looks across the ground when it is cast, as long as it is; none when
/// the camera looks straight down
glm::vec3 Heading(const Effect& effect)
{
	glm::vec3 heading = effect.GetProcessInfo().cameraForward;
	heading.y = 0.0f;
	return heading;
}

/// The weather a storm lays over the land, taken away when its clouds let go of it
class RainStormLease
{
public:
	RainStormLease(ParticleWorldInterface& world, uint32_t storm)
	    : _world(world)
	    , _storm(storm)
	{
	}
	RainStormLease(const RainStormLease&) = delete;
	RainStormLease& operator=(const RainStormLease&) = delete;
	RainStormLease(RainStormLease&&) = delete;
	RainStormLease& operator=(RainStormLease&&) = delete;
	~RainStormLease() { _world.RemoveRainStorm(_storm); }

	/// False once the weather has ended the storm
	[[nodiscard]] bool Move(glm::vec3 centre) const { return _world.MoveRainStorm(_storm, centre); }

private:
	ParticleWorldInterface& _world;
	uint32_t _storm;
};

/// What one gathering of clouds keeps between steps
struct Gathering
{
	/// Clouds a second, and how many are due and made
	float rate {0.0f};
	float due {0.0f};
	int made {0};
	/// The way they circle, 1 or -1
	float direction {1.0f};
	bool lightning {false};
	/// Its age when the next bolt flies, and when the flying one ends
	float nextStrike {0.0f};
	float strikeEnds {0.0f};
	/// The cloud carrying the bolt, and the bolt's collection while no cloud carries it
	const Atom* boltCloud {nullptr};
	Collection* bolt {nullptr};
	std::unique_ptr<Collection> spareBolt;
	/// The first core's clouds own the storm's weather
	bool ownsStorm {false};
	std::shared_ptr<RainStormLease> storm;
};

/// A cloud's own: how far out it was born, where round it is, how much higher it floats, and when a bolt lit it
struct CloudData
{
	float radius;
	float angle;
	float heightVary;
	bool lit;
	float litAge;
};

CloudData ReadCloud(const AtomRuleData& data)
{
	return {.radius = data.a.x, .angle = data.a.y, .heightVary = data.a.z, .lit = data.b.x != 0.0f, .litAge = data.b.y};
}

void WriteCloud(AtomRuleData& data, const CloudData& cloud)
{
	data.a = {cloud.radius, cloud.angle, cloud.heightVary, 0.0f};
	data.b = {cloud.lit ? 1.0f : 0.0f, cloud.litAge, 0.0f, 0.0f};
}

class CloudGather final: public Modifier
{
public:
	explicit CloudGather(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , numAtoms(object.Int("NumAtoms", 10))
	    , timeToForm(std::max(object.Float("TimeToForm", 8.0f), 0.001f))
	    , maxAngularSpeed(object.Float("MaxAngularSpeed", 0.2f))
	    , lightningGroup(object.Int("LightningGroup", -1))
	    , specLife(object.Float("SpecLife", 0.5f))
	    , lightningLife(object.Float("LightningLife", 0.2f))
	    , lightningDelay(object.Float("LightningDelay", 8.0f))
	    , switchLife(object.Float("SwitchLife", 3.0f))
	    , heightVary(object.Float("HeightVaryAmount", 4.0f))
	    , ratioMaxCollection(object.Float("CloudRatioMaxCollection", 1.0f))
	    , radiusInitialScale(object.Float("CollectionRadiusInitialScale", 2.0f))
	    , tornadoGroup(object.Int("TornadoGroup", 10))
	    , wind({.minSpeed = object.Float("WindMinSpeed", 40.0f),
	            .maxSpeed = object.Float("WindMaxSpeed", 100.0f),
	            .magnitudeForMinSpeed = object.Float("MagnitudeForWindMinSpeed", 20.0f),
	            .magnitudeForMaxSpeed = object.Float("MagnitudeForWindMaxSpeed", 100.0f)})
	    , shades({.maxColour = object.Int("MaxColor", 120),
	              .minColour = object.Int("MinColor", 90),
	              .minAlpha = object.Int("MinAlpha", 0),
	              .maxAlpha = object.Int("MaxAlpha", 180),
	              .minScale = object.Float("MinScaleFactor", 0.0f),
	              .maxScale = object.Float("MaxScaleFactor", 1.0f),
	              .maxRatio = object.Float("MaxCloudRatio", 10.0f),
	              .minRatio = object.Float("MinCloudRatio", 1.0f),
	              .fractionToMaxSize = object.Float("FracToMaxSize", 0.413717f)})
	    , radiusProvider(object.String("RadiusFloatProvider"))
	    , scaleProvider(object.String("ScaleFloatProvider"))
	    , heightProvider(object.String("CloudHeight"))
	    , thunder({.action = object.Sound("SoundLightning"), .size = 2, .travelsAtSoundSpeed = true, .onLand = true})
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		const float radius = effect.FloatProvider(radiusProvider, k_Missing);
		const float scale = effect.FloatProvider(scaleProvider, k_Missing);
		const float cloudHeight = effect.FloatProvider(heightProvider, k_Missing);
		// It needs all three of its sizes
		if (std::isnan(radius) || std::isnan(scale) || std::isnan(cloudHeight))
		{
			return true;
		}
		if (!slot.data.has_value())
		{
			slot.data = std::make_shared<Gathering>();
		}
		auto& gathering = **std::any_cast<std::shared_ptr<Gathering>>(&slot.data);
		if (effect.Closing())
		{
			// The storm's weather goes at once
			gathering.storm.reset();
			gathering.ownsStorm = false;
		}
		const glm::vec3 middle = collection.parent != nullptr ? effect.GlobalPosition(*collection.parent) : effect.GetOrigin();
		const int level = effect.PowerUpLevel();
		const float collectionAge = effect.CollectionAge(collection);
		if (slot.first)
		{
			slot.first = false;
			Start(effect, collection, gathering, level);
		}
		Births(effect, collection, gathering, radius);
		EndStrike(collection, gathering, collectionAge);

		const float ratioScale = storm::GatherScale(ratioMaxCollection, collectionAge);
		const float radiusScale = storm::GatherScale(radiusInitialScale, collectionAge);
		std::vector<Atom*> striking;
		for (auto& cloud : collection.atoms)
		{
			auto& data = cloud->data[this];
			if (effect.AtomAge(*cloud) > timeToForm)
			{
				// It has lived its life: it is born again somewhere else
				Rebirth(effect, *cloud, data, radius);
			}
			auto own = ReadCloud(data);
			const float life = effect.AtomAge(*cloud) / timeToForm;
			if (life > k_StrikingShare)
			{
				striking.push_back(cloud.get());
			}
			own.angle += storm::CloudAngularSpeed(life, maxAngularSpeed, gathering.direction) * effect.GetDt();
			own.angle = std::fmod(own.angle, k_TwoPi);
			const float distance = storm::CloudDistance(life, own.radius, collectionAge, own.angle, radiusScale);
			const auto look = storm::LookOf(life, scale, ratioScale, shades);
			cloud->rgba = {look.grey, look.grey, look.grey, look.alpha};
			cloud->ruleScale = look.scale;
			cloud->stretch = look.ratio;
			cloud->position = {middle.x + std::cos(own.angle) * distance,
			                   cloud->baseScale * own.heightVary * cloud->ruleScale + cloudHeight,
			                   middle.z + std::sin(own.angle) * distance};
			// A struck cloud flashes white blue
			cloud->specular = {0, 0, 0};
			if (own.lit)
			{
				const float lit = effect.AtomAge(*cloud) - own.litAge;
				if (lit > specLife)
				{
					own.lit = false;
				}
				else
				{
					const auto light = storm::FlashLight(lit, specLife);
					cloud->specular = {light.r, light.g, light.b};
				}
			}
			WriteCloud(data, own);
		}
		Strike(effect, collection, gathering, striking, collectionAge);

		// The clouds float above the land under them
		const float ground = level == k_TornadoLevel ? effect.GetOrigin().y : GroundUnder(effect, middle, radius);
		for (auto& cloud : collection.atoms)
		{
			cloud->position.y += ground;
		}

		// The first core's clouds lay the storm's weather over the land, and keep it under them
		if (gathering.ownsStorm && !gathering.storm)
		{
			LayStorm(effect, gathering, middle, radius, cloudHeight);
		}
		// A storm a script ended is laid again at the next step, fading in again
		if (gathering.storm && !gathering.storm->Move(middle))
		{
			gathering.storm.reset();
		}
		return true;
	}

private:
	void Start(Effect& effect, Collection& collection, Gathering& gathering, int level) const
	{
		gathering.lightning = level != -1;
		const auto* core = collection.parent;
		const auto* cores = core != nullptr ? core->collection : nullptr;
		if (cores != nullptr && !cores->atoms.empty() && cores->atoms.front().get() == core)
		{
			gathering.ownsStorm = true;
			if (level == k_TornadoLevel)
			{
				const std::array groups {tornadoGroup};
				effect.AddSubCollections(*collection.parent, groups);
			}
		}
		gathering.rate = static_cast<float>(numAtoms) / timeToForm;
		gathering.direction = effect.Random(1.0f) < 0.5f ? -1.0f : 1.0f;
		gathering.nextStrike = effect.Random(0.5f, 1.0f) * switchLife + lightningDelay;
	}

	void Births(Effect& effect, Collection& collection, Gathering& gathering, float radius) const
	{
		gathering.due += effect.GetDt() * gathering.rate;
		const auto* cloudCreator = effect.FindCreator(creator);
		while (static_cast<float>(gathering.made) < gathering.due && static_cast<int>(collection.atoms.size()) < numAtoms)
		{
			++gathering.made;
			auto& cloud = effect.NewAtom(collection, cloudCreator, {});
			Rebirth(effect, cloud, cloud.data[this], radius);
		}
	}

	/// A cloud starts out far round the middle, unlit
	void Rebirth(Effect& effect, Atom& cloud, AtomRuleData& data, float radius) const
	{
		cloud.birth = effect.GetAge();
		WriteCloud(data, {.radius = storm::NewCloudRadius(effect.Random(0.7f, 1.0f), radius),
		                  .angle = effect.Random(k_TwoPi),
		                  .heightVary = effect.Random(heightVary),
		                  .lit = false,
		                  .litAge = 0.0f});
		cloud.specular = {0, 0, 0};
	}

	/// The flying bolt ends: its cloud lets go of it
	void EndStrike(Collection& collection, Gathering& gathering, float collectionAge) const
	{
		if (gathering.boltCloud == nullptr || !(gathering.strikeEnds < collectionAge))
		{
			return;
		}
		for (auto& cloud : collection.atoms)
		{
			if (cloud.get() != gathering.boltCloud)
			{
				continue;
			}
			auto& subs = cloud->subCollections;
			const auto found =
			    std::ranges::find_if(subs, [&gathering](const auto& sub) { return sub.get() == gathering.bolt; });
			if (found != subs.end())
			{
				gathering.spareBolt = std::move(*found);
				subs.erase(found);
				gathering.spareBolt->parent = nullptr;
			}
		}
		gathering.boltCloud = nullptr;
	}

	/// Now and then one of the older clouds lets fly a bolt, lights up and thunders
	void Strike(Effect& effect, Collection& collection, Gathering& gathering, const std::vector<Atom*>& striking,
	            float collectionAge) const
	{
		if (!gathering.lightning || striking.empty() || effect.Closing() || !(collectionAge - gathering.nextStrike > 0.0f))
		{
			return;
		}
		const auto* sink = effect.GetSink();
		const std::optional<float> tribal = sink != nullptr ? std::optional(sink->TribalPower()) : std::nullopt;
		gathering.nextStrike = collectionAge + storm::NextStrikeWait(effect.Random(0.5f, 1.0f), switchLife, tribal);
		auto& cloud = *striking.at(static_cast<size_t>(effect.Rand(static_cast<int32_t>(striking.size()))));
		if (gathering.boltCloud != nullptr)
		{
			EndStrike(collection, gathering, std::numeric_limits<float>::max());
		}
		// The bolt is made the first time and carried from cloud to cloud after
		if (gathering.spareBolt)
		{
			auto bolt = std::move(gathering.spareBolt);
			bolt->parent = &cloud;
			// It keeps its forks and targets, looking again only once it has been held a second or has moved half its
			// search radius from where it last looked
			gathering.bolt = bolt.get();
			cloud.subCollections.push_back(std::move(bolt));
		}
		else if (lightningGroup >= 0)
		{
			const std::array groups {lightningGroup};
			effect.AddSubCollections(cloud, groups);
			gathering.bolt = cloud.subCollections.back().get();
		}
		StopAllAtomSounds(cloud);
		auto sound = thunder;
		sound.size = storm::ThunderSize(effect.Random(1.0f));
		StartAtomSound(effect, cloud, sound);
		gathering.boltCloud = &cloud;
		gathering.strikeEnds = collectionAge + effect.Random(0.5f, 1.0f) * lightningLife;
		auto& data = cloud.data[this];
		auto own = ReadCloud(data);
		own.lit = true;
		own.litAge = effect.AtomAge(cloud);
		WriteCloud(data, own);
	}

	/// The land's height on average over a grid round the middle, a third of the clouds' radius apart
	[[nodiscard]] static float GroundUnder(const Effect& effect, glm::vec3 middle, float radius)
	{
		const auto& world = effect.Services().world;
		const float spacing = radius * k_GroundSampleSpacing;
		float sum = 0.0f;
		for (int x = -1; x <= 1; ++x)
		{
			for (int z = -1; z <= 1; ++z)
			{
				sum +=
				    world.LandHeight({middle.x + static_cast<float>(x) * spacing, middle.z + static_cast<float>(z) * spacing});
			}
		}
		return sum / 9.0f;
	}

	void LayStorm(Effect& effect, Gathering& gathering, glm::vec3 middle, float radius, float cloudHeight) const
	{
		auto& world = effect.Services().world;
		const auto& info = effect.GetProcessInfo();
		const auto heading = Heading(effect);
		const auto* sink = effect.GetSink();
		const auto windBytes = storm::WindBytes(heading, effect.GetMagnitude(), info.power, wind);
		const auto rain = storm::RainByte(true, sink != nullptr ? sink->RainAmount() : std::nullopt, info.power);
		const auto id = world.AddRainStorm(storm::RainStormFor(middle, radius, cloudHeight, timeToForm, windBytes, rain));
		if (id != 0)
		{
			gathering.storm = std::make_shared<RainStormLease>(world, id);
		}
		else
		{
			gathering.ownsStorm = false;
		}
	}

	std::string creator;
	int numAtoms;
	float timeToForm;
	float maxAngularSpeed;
	int lightningGroup;
	float specLife;
	float lightningLife;
	float lightningDelay;
	float switchLife;
	float heightVary;
	float ratioMaxCollection;
	float radiusInitialScale;
	int tornadoGroup;
	storm::StormWind wind;
	storm::CloudShades shades;
	std::string radiusProvider;
	std::string scaleProvider;
	std::string heightProvider;
	ParticleSound thunder;
};

/// The storm's cores drift with the wind once it has formed, over hill and dale alike, and the miracle with them
class CloudMover final: public Modifier
{
public:
	explicit CloudMover(const ParticleObject& object)
	    : delay(object.Float("DelayBeforeMove", 5.0f))
	    , damping(object.Float("WindDamping", 0.06f))
	    , magnification(object.Float("WindMagnification", 30.0f))
	{
	}

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		if (slot.first)
		{
			slot.first = false;
			for (auto& core : collection.atoms)
			{
				core->position = effect.GetOrigin();
			}
		}
		if (effect.GetAge() < delay)
		{
			return true;
		}
		const float dt = effect.GetDt();
		const auto& world = effect.Services().world;
		for (auto& core : collection.atoms)
		{
			const auto before = core->position;
			// A storm a script casts feels no wind, so it stays where it was put
			const auto wind = storm::DriftWind(world.SmoothWindAt(before), effect.IsScriptCasting());
			core->velocity = storm::DriftVelocity(core->velocity, wind, magnification, damping, dt);
			core->position = before + (core->velocity * dt);
			if (auto* sink = effect.GetSink())
			{
				sink->MoveTo(core->position);
			}
			// The tornado's storm is turned away by shields
			if (effect.PowerUpLevel() == 1)
			{
				DeflectOffShields(effect, *core, before);
			}
		}
		return true;
	}

private:
	float delay;
	float damping;
	float magnification;
};

/// What the swirl keeps between steps: where it is heading and how wide it is
struct Swirl
{
	glm::vec3 velocity {0.0f};
	float radius {0.0f};
};

/// The swirl where a storm is cast: dark sprites circling its point, tightening and turning faster, drifting off the way
/// it was thrown, then spreading out and fading
class StormCast final: public Modifier
{
public:
	explicit StormCast(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , count(object.Int("NumAtoms", 30))
	    , params({.maxRadius = object.Float("MaxRadius", 1.1f),
	              .minRadius = object.Float("MinRadius", 0.2f),
	              .thetaDotMinRadius = object.Float("ThetaDotMinRadius", 4.0f),
	              .thetaDotMaxRadius = object.Float("ThetaDotMaxRadius", 0.6f),
	              .radiusDot = object.Float("RadiusDot", 0.5f),
	              .dispersalAge = object.Float("DispersalAge", 2.4f),
	              .fadeOutTime = object.Float("FadeOutTime", 2.0f),
	              .accelerationStart = object.Float("AccnStartTime", 1.0f),
	              .accelerationEnd = object.Float("AccnEndTime", 3.0f)})
	    , initHeight(object.Float("InitHeight", 0.06f))
	    , thetaDotSpread(object.Float("ThetaDotSpread", 0.7f))
	    , scaleSpread(object.Float("InitScaleSpread", 0.3f))
	    , radiusSpread(object.Float("InitRadiusSpread", 0.85f))
	    , maxSpeed(object.Float("MaxSpeed", 20.0f))
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		auto* parent = collection.parent;
		if (parent == nullptr)
		{
			return true;
		}
		if (!slot.data.has_value())
		{
			slot.data = Swirl {};
		}
		auto& swirl = *std::any_cast<Swirl>(&slot.data);
		const float dt = effect.GetDt();
		if (slot.first)
		{
			slot.first = false;
			Start(effect, collection, swirl, *parent);
		}
		const float age = effect.CollectionAge(collection);
		parent->position += swirl.velocity * dt * storm::SwirlAcceleration(age, params);
		swirl.radius = storm::SwirlRadius(swirl.radius, age, dt, params);
		const auto alpha = storm::SwirlAlpha(age, params);
		if (!alpha.has_value())
		{
			collection.atoms.clear();
			return true;
		}
		collection.alpha = *alpha;
		const float turning = storm::SwirlTurning(swirl.radius, params);
		const auto& world = effect.Services().world;
		for (auto& atom : collection.atoms)
		{
			auto& data = atom->data[this];
			data.a.x += dt * data.a.y * turning;
			const float out = data.a.z * swirl.radius;
			auto global = effect.LocalToGlobal(collection, {std::cos(data.a.x) * out, 0.0f, std::sin(data.a.x) * out});
			global.y = world.LandHeight({global.x, global.z}) + effect.GetMagnitude() * initHeight;
			atom->position = effect.GlobalToLocal(collection, global);
		}
		return true;
	}

private:
	void Start(Effect& effect, Collection& collection, Swirl& swirl, Atom& parent) const
	{
		swirl.radius = params.maxRadius;
		// It heads the way it was cast at its top speed; cast looking straight down, it stays where it is
		const auto heading = Heading(effect);
		const float length = glm::length(heading);
		swirl.velocity = length != 0.0f ? heading * (maxSpeed / length) : glm::vec3(0.0f);
		const auto* spriteCreator = effect.FindCreator(creator);
		for (int i = 0; i < count; ++i)
		{
			auto& atom = effect.NewAtom(collection, spriteCreator, nextGroups);
			atom.baseScale *= effect.Random(1.0f - scaleSpread, 1.0f + scaleSpread);
			auto& data = atom.data[this];
			// Where round it is, how fast it turns, and how far out
			data.a.x = effect.Random(k_TwoPi);
			data.a.y = effect.Random(-thetaDotSpread, thetaDotSpread) + 1.0f;
			data.a.z = effect.Random(1.0f - radiusSpread, 1.0f);
		}
		// The swirl is as wide as the storm
		parent.ruleScale = effect.GetMagnitude();
	}

	std::string creator;
	std::vector<int> nextGroups;
	int count;
	storm::SwirlParams params;
	float initHeight;
	float thetaDotSpread;
	float scaleSpread;
	float radiusSpread;
	float maxSpeed;
};

/// Flashes of lightning about the storm held in the hand: sprites made now and then, each lighting the cloud they come
/// from white blue for a moment
class LightningSprite final: public Modifier
{
public:
	explicit LightningSprite(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , limits({
	          .frequency = object.Float("EmissionFreq", 0.001f),
	          .maxAlive = object.Int("MaxAtoms", -1),
	          .maxTotal = object.Int("MaxTotalAtomsToEmit", -1),
	          .randomise = object.Bool("Randomise", true),
	      })
	    , flashLife(object.Float("SpecLife", 0.5f))
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		const auto* spriteCreator = effect.FindCreator(creator);
		if (spriteCreator == nullptr)
		{
			return true;
		}
		const float age = effect.CollectionAge(collection);
		if (storm::ShouldFlash(slot.emitter, limits, age, effect.GetDt(), static_cast<int>(collection.atoms.size()),
		                       [&effect] { return effect.Random(0.75f); }))
		{
			// state.x: flashing, state.y: since when
			slot.state.x = 1.0f;
			slot.state.y = age;
			effect.NewAtom(collection, spriteCreator, nextGroups);
		}
		if (slot.state.x != 0.0f && collection.parent != nullptr)
		{
			const float lit = age - slot.state.y;
			glm::u8vec3 light(0);
			if (lit <= flashLife)
			{
				light = storm::FlashLight(lit, flashLife);
			}
			else
			{
				slot.state.x = 0.0f;
			}
			collection.parent->specular = {light.r, light.g, light.b};
		}
		return true;
	}

private:
	std::string creator;
	std::vector<int> nextGroups;
	maths::EmitterLimits limits;
	float flashLife;
};
} // namespace

void openblack::particles::RegisterStormRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_CloudGather", ParticleClassRegistry::Make<CloudGather>);
	registry.AddModifier("UR_CloudMoverNew", ParticleClassRegistry::Make<CloudMover>);
	registry.AddModifier("UR_StormCast", ParticleClassRegistry::Make<StormCast>);
	registry.AddModifier("EmitterRuleLightningSprite", ParticleClassRegistry::Make<LightningSprite>);
}
