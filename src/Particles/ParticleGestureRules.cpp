/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The rules of the hand's gestures: the trail a recognised gesture leaves on the land, and the chain that follows the
// hand while it draws one

#include <cmath>

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ParticleFile.h>

#include "GestureTrail.h"
#include "LightSheet.h"
#include "ParticleClassRegistry.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;
namespace trail = openblack::particles::gesture_trail;

namespace
{
/// The in-game bank's sound of a gesture recognised
constexpr uint32_t k_RecognisedSample = 36;
constexpr float k_ByteMax = 255.0f;

/// What the trail rule keeps for each of its atoms, one atom a trail
struct TrailAtom
{
	std::shared_ptr<GestureTrail> trail;
	std::shared_ptr<LightSheet> sheet;
	/// Its particles have been made, when its collection was this old
	bool started {false};
	float startAge {0.0f};
	/// The particles' size, from the shape's length
	float scale {1.0f};
	/// The box round the shape once raised
	glm::vec3 boxMin {0.0f};
	glm::vec3 boxMax {0.0f};
	/// The drawing player's colour, 0xRRGGBB
	uint32_t colour {0};
	/// The particles in a shuffled order, so that the wiggles of neighbours differ
	std::vector<int32_t> order;
};
using TrailAtoms = std::unordered_map<const Atom*, TrailAtom>;

/// A recognised gesture's trail: an atom for each trail the gesture system lays out, with the gesture's recognised sound,
/// a collection of particles that flow from the path the hand drew to the gesture's shape over the land, wiggling and
/// flashing in the player's colour before they disperse, and a sheet of light rising along the shape. The hand glows the
/// player's colour as the particles flash.
class GesturingRecognised final: public Modifier
{
public:
	explicit GesturingRecognised(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , spriteCreator(object.String("SpriteCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , sheetHeightScale(object.Float("LightSheetHeightScale", 0.0f))
	    , timeToIdeal(object.Float("TimeToIdeal", 0.0f))
	    , interpGain(object.Float("InterpGain", 0.0f))
	    , dieAge(object.Float("DieAge", 0.0f))
	    , sheetDieAge(object.Float("LightSheetDieAge", 0.0f))
	    , maxAlpha(object.Float("MaxAlpha", 0.0f))
	    , numAtoms(object.Int("NumAtoms", 0))
	    , wiggleFreq(object.Float("WiggleFreq", 0.0f))
	    , wiggleMag(object.Float("WiggleMag", 0.0f))
	    , wiggleMagY(object.Float("WiggleMagY", 0.0f))
	    , wiggleSpeed(object.Float("WiggleSpeed", 0.0f))
	    , wigglePhaseSpeed(object.Float("WigglePhaseSpeed", 0.0f))
	    , dispersalTime(object.Float("DispersalTime", 0.0f))
	    , collectionAlphaPulse(object.Int("CollectionAlphaPulse", 0))
	    , collectionAlphaInit(object.Int("CollectionAlphaInit", 0))
	    , shrinkTime(object.Float("ShrinkTimeAfterDispersal", 0.0f))
	    , handPulseDuration(object.Float("HandPulseDuration", 0.0f))
	    , sparkleGroup(object.Int("SparkleGroup", 0))
	    , goToIdeal(object.Bool("GoToIdeal", false))
	    , doTransition(object.Bool("DoTransition", false))
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		const auto* pointCreator = effect.FindCreator(creator);
		if (pointCreator == nullptr)
		{
			return false;
		}
		if (!slot.data.has_value())
		{
			slot.data = TrailAtoms {};
		}
		auto& trails = *std::any_cast<TrailAtoms>(&slot.data);
		auto& world = effect.Services().world;

		// One waiting trail a step becomes an atom, heard as it appears
		if (auto waiting = world.TakeGestureTrail())
		{
			auto& atom = effect.NewAtom(collection, pointCreator, nextGroups);
			trails[&atom].trail = std::move(waiting);
			world.PlayListenerSound(k_RecognisedSample);
		}

		// A trail ends when its atom has lived its time, and the hand's glow goes with it
		std::erase_if(collection.atoms, [&](const std::unique_ptr<Atom>& atom) {
			if (effect.AtomAge(*atom) <= dieAge)
			{
				return false;
			}
			if (trails.erase(atom.get()) > 0)
			{
				world.SetHandGlow(0);
			}
			return true;
		});

		// The newest first
		for (auto it = collection.atoms.rbegin(); it != collection.atoms.rend(); ++it)
		{
			auto& atom = **it;
			const auto found = trails.find(&atom);
			if (found == trails.end() || found->second.trail == nullptr)
			{
				continue;
			}
			auto& data = found->second;
			for (auto& sub : atom.subCollections)
			{
				if (sub->group == sparkleGroup && data.started)
				{
					CarryRound(effect, atom, *sub, data);
				}
				else
				{
					ModifyParticles(effect, *sub, data);
				}
			}
			const float age = effect.AtomAge(atom);
			if (data.sheet == nullptr)
			{
				std::vector<glm::vec3> points;
				points.reserve(trail::k_SheetPoints);
				const auto step = 1.0f / static_cast<float>(trail::k_SheetPoints - 1);
				for (int i = 0; i < trail::k_SheetPoints; ++i)
				{
					points.push_back(data.trail->ideal.At(static_cast<float>(static_cast<double>(i) * step)));
				}
				data.sheet = std::make_shared<LightSheet>();
				data.sheet->Start(std::move(points), data.colour, data.scale * sheetHeightScale, trail::k_SheetShiftSeconds);
				world.AddLightSheet(data.sheet);
			}
			data.sheet->SetStrength(trail::SheetStrength(age, sheetDieAge));
		}
		return true;
	}

private:
	/// The trail's atom goes round the shape, fading out over the end of its life
	void CarryRound(Effect& effect, Atom& atom, Collection& sub, const TrailAtom& data) const
	{
		const float age = effect.AtomAge(atom);
		const auto round = static_cast<float>(std::fmod(static_cast<double>(age) / trail::k_CircuitSeconds, 1.0));
		atom.position = data.trail->ideal.At(round);
		const double left = static_cast<double>(dieAge) - age;
		if (left < trail::k_CircuitFadeSeconds)
		{
			const double alpha = std::clamp(left * (k_ByteMax / trail::k_CircuitFadeSeconds), 0.0, 255.0);
			const auto level = static_cast<uint8_t>(static_cast<int>(alpha));
			atom.rgba[3] = level;
			sub.alpha = level;
		}
	}

	/// The particles flowing from the drawn path to the shape
	void ModifyParticles(Effect& effect, Collection& sub, TrailAtom& data) const
	{
		auto& world = effect.Services().world;
		if (!data.started)
		{
			Start(effect, sub, data);
		}
		const float age = effect.CollectionAge(sub);

		// From the drawn path to the shape over a time, eased by the gain
		const auto grown = static_cast<float>(
		    std::min(static_cast<double>(age) - data.startAge, static_cast<double>(timeToIdeal)) / timeToIdeal);
		const auto transition = trail::Transition(grown, interpGain);
		const auto width = data.boxMax.x - data.boxMin.x;
		const auto depth = data.boxMax.z - data.boxMin.z;
		const auto wiggle = trail::WiggleAmount(age, wigglePhaseSpeed, dispersalTime, shrinkTime);

		// A while in, the particles flash and the hand glows in the player's colour, both dying away
		const auto sinceFlash = static_cast<float>(static_cast<double>(age) - trail::k_FlashStart);
		if (sinceFlash > 0.0f)
		{
			const double flash = collectionAlphaPulse * trail::FlashLeft(sinceFlash, trail::k_FlashSeconds);
			sub.alpha = static_cast<uint8_t>(static_cast<int>(flash));
			const auto glow = static_cast<uint8_t>(
			    static_cast<int>(trail::FlashLeft(sinceFlash, handPulseDuration) * static_cast<double>(k_ByteMax)));
			world.SetHandGlow(trail::Dimmed(data.colour, glow));
		}

		const auto count = sub.atoms.size();
		const auto step = static_cast<float>(1.0 / (static_cast<double>(count) - 1.0));
		const auto perSecond = 1.0f / effect.GetDt();
		const auto& noise = effect.Services().noise;
		// The newest first
		for (size_t i = 0; i < count; ++i)
		{
			auto& atom = *sub.atoms[count - 1 - i];
			const auto along = static_cast<float>(static_cast<double>(i) * step);
			atom.rgba[3] = trail::RevealAlpha(along, grown, maxAlpha);

			glm::vec3 position;
			if (doTransition)
			{
				const auto drawn = data.trail->drawn.At(along);
				const auto ideal = data.trail->ideal.At(along);
				position = drawn + ((ideal - drawn) * transition);
			}
			else
			{
				position = goToIdeal ? data.trail->ideal.At(along) : data.trail->drawn.At(along);
			}

			// Each wiggles by its own place in the shuffled order, across the shape's box
			const auto place = static_cast<float>(static_cast<double>(data.order.at(i)) * step);
			const auto phase = [&](float offset) {
				return static_cast<float>((static_cast<double>(age) * wiggleSpeed) +
				                          ((static_cast<double>(place) + offset) * wiggleFreq));
			};
			const auto acrossX = static_cast<float>(noise.Smooth(phase(0.0f)) * static_cast<double>(width) * wiggleMag);
			const auto acrossZ =
			    static_cast<float>(noise.Smooth(phase(trail::k_WiggleOffsetZ)) * static_cast<double>(depth) * wiggleMag);
			const double up = (noise.Smooth(phase(trail::k_WiggleOffsetY)) + 1.0) * 0.5 * wiggleMagY * depth;
			position.x = (acrossX * wiggle) + position.x;
			position.y = static_cast<float>(position.y + (up * wiggle));
			position.z = position.z + (acrossZ * wiggle);

			atom.velocity = (position - atom.position) * perSecond;
			atom.position = position;
		}
	}

	/// The first step of a trail's particles: the shape is raised towards the camera, the particles are made in the
	/// player's colour, sized by the shape's length, and put in a shuffled order
	void Start(Effect& effect, Collection& sub, TrailAtom& data) const
	{
		auto& world = effect.Services().world;
		data.startAge = effect.CollectionAge(sub);
		data.started = true;
		sub.alpha = static_cast<float>(static_cast<uint8_t>(collectionAlphaInit));
		data.scale = static_cast<float>(static_cast<double>(data.trail->ideal.Length()) * trail::k_ScalePerLength);

		const auto camera = world.CameraPosition();
		auto& ideal = data.trail->ideal;
		for (size_t i = 0; i < ideal.Size(); ++i)
		{
			ideal.SetPoint(i, trail::Lift(ideal.Points()[i], camera, data.scale));
		}

		data.colour = world.PlayerColour(effect.GetPlayer()) & 0xFFFFFFu;
		if (const auto* sprite = effect.FindCreator(spriteCreator))
		{
			for (int i = 0; i < numAtoms; ++i)
			{
				auto& atom = effect.NewAtom(sub, sprite, {});
				atom.rgba[0] = static_cast<uint8_t>(data.colour >> 16u);
				atom.rgba[1] = static_cast<uint8_t>(data.colour >> 8u);
				atom.rgba[2] = static_cast<uint8_t>(data.colour);
				atom.baseScale *= data.scale;
			}
		}
		if (sub.parent != nullptr)
		{
			sub.parent->ruleScale *= data.scale;
		}

		data.boxMin = data.boxMax = ideal.Points().front();
		for (const auto& point : ideal.Points())
		{
			data.boxMin = glm::min(data.boxMin, point);
			data.boxMax = glm::max(data.boxMax, point);
		}

		data.order.resize(static_cast<size_t>(std::max(numAtoms, 0)));
		for (size_t i = 0; i < data.order.size(); ++i)
		{
			data.order[i] = static_cast<int32_t>(i);
		}
		for (int k = 0; k < numAtoms * 2; ++k)
		{
			const auto a = static_cast<size_t>(effect.Rand(numAtoms));
			const auto b = static_cast<size_t>(effect.Rand(numAtoms));
			std::swap(data.order.at(a), data.order.at(b));
		}
	}

	std::string creator;
	std::string spriteCreator;
	std::vector<int> nextGroups;
	float sheetHeightScale;
	float timeToIdeal;
	float interpGain;
	float dieAge;
	float sheetDieAge;
	float maxAlpha;
	int numAtoms;
	float wiggleFreq;
	float wiggleMag;
	float wiggleMagY;
	float wiggleSpeed;
	float wigglePhaseSpeed;
	float dispersalTime;
	int collectionAlphaPulse;
	int collectionAlphaInit;
	float shrinkTime;
	float handPulseDuration;
	int sparkleGroup;
	bool goToIdeal;
	bool doTransition;
};

/// What the chain rule keeps for each of its atoms
struct ChainAtom
{
	/// It follows the hand until the hand stops gesturing, then waits a while to go
	bool active {true};
	bool first {true};
	float stoppedAt {0.0f};
	/// Where the hand was at the last step, and where the last joint was laid
	glm::vec3 last {0.0f};
	glm::vec3 lastJoint {0.0f};
	/// How many joints the hand's movement has asked for so far, and how many have been laid
	float wanted {0.0f};
	float laid {0.0f};
};
struct ChainState
{
	bool gesturing {false};
	std::unordered_map<const Atom*, ChainAtom> atoms;
};

/// The chain that follows the hand while it draws a gesture: each time the hand starts gesturing, whatever chain there
/// was goes and an atom is made whose collection of joints is the chain. The joints are laid at the hand as it moves,
/// one each time the hand has gone the emit distance from the last (no faster than the whole chain each lifetime), the
/// older joints moving down the chain. Once the hand stops gesturing, the chain is left to fade and goes a few seconds
/// later.
class ChainGesture final: public Modifier
{
public:
	/// A chain the hand has stopped drawing goes this long after
	static constexpr float k_LeftSeconds = 5.0f;

	explicit ChainGesture(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , nextGroups(object.IntArray("NextGroups"))
	    , dieAge(object.Float("DieAge", 0.0f))
	    , minEmitDist(object.Float("MinEmitDist", 0.0f))
	    , adjustInitialScale(object.String("AdjustInitialScale"))
	    , inTestMode(object.Bool("InTestMode", false))
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		const auto* pointCreator = effect.FindCreator(creator);
		if (pointCreator == nullptr)
		{
			return false;
		}
		if (!slot.data.has_value())
		{
			slot.data = ChainState {};
		}
		auto& state = *std::any_cast<ChainState>(&slot.data);
		const bool was = state.gesturing;
		state.gesturing = inTestMode || effect.GetProcessInfo().enabled;
		if (state.gesturing && !was)
		{
			collection.atoms.clear();
			state.atoms.clear();
			effect.NewAtom(collection, pointCreator, nextGroups);
		}

		// The newest first
		for (size_t n = collection.atoms.size(); n-- > 0;)
		{
			auto& atom = *collection.atoms[n];
			auto& data = state.atoms[&atom];
			if (!data.active && static_cast<double>(effect.AtomAge(atom)) - data.stoppedAt > k_LeftSeconds)
			{
				state.atoms.erase(&atom);
				collection.atoms.erase(collection.atoms.begin() + static_cast<std::ptrdiff_t>(n));
				continue;
			}
			if (!data.active)
			{
				continue;
			}
			if (!state.gesturing)
			{
				data.active = false;
				data.stoppedAt = effect.AtomAge(atom);
				continue;
			}
			if (atom.subCollections.empty())
			{
				return true;
			}
			Lay(effect, data, *atom.subCollections.back(), atom.position);
		}
		return true;
	}

private:
	/// The hand at a point lays the joints its movement asks for, each between where the hand was and where it is
	void Lay(Effect& effect, ChainAtom& data, Collection& chain, glm::vec3 hand) const
	{
		if (chain.atoms.empty())
		{
			return;
		}
		if (data.first)
		{
			data.lastJoint = hand;
			data.last = hand;
			for (auto& joint : chain.atoms)
			{
				joint->position = hand;
				joint->ruleScale = 0.0f;
			}
		}
		const float headScale = adjustInitialScale.empty() ? 1.0f : effect.FloatProvider(adjustInitialScale, 1.0f);
		const float before = data.wanted;
		const auto most = static_cast<float>(static_cast<double>(chain.atoms.size()) / dieAge * effect.GetDt());
		double asked = 1.0;
		if (data.laid != 0.0f)
		{
			const auto away = data.lastJoint - hand;
			asked = std::sqrt((static_cast<double>(away.x) * away.x) + (static_cast<double>(away.y) * away.y) +
			                  (static_cast<double>(away.z) * away.z)) /
			        minEmitDist;
		}
		if (!(most > asked))
		{
			asked = most;
		}
		const double total = asked + before;
		data.wanted = static_cast<float>(total);
		for (bool more = total - 1.0 > data.laid; more; more = static_cast<double>(data.wanted) - 1.0 > data.laid)
		{
			data.laid += 1.0f;
			// Each joint takes the place, age and size of the one newer than it
			for (size_t j = 0; j + 1 < chain.atoms.size(); ++j)
			{
				auto& joint = *chain.atoms[j];
				const auto& newer = *chain.atoms[j + 1];
				joint.position = newer.position;
				joint.birth = newer.birth;
				joint.baseScale = newer.baseScale;
			}
			auto& head = *chain.atoms.back();
			glm::vec3 point = hand;
			if (data.wanted != before)
			{
				const auto t =
				    static_cast<float>((static_cast<double>(data.laid) - before) / (static_cast<double>(data.wanted) - before));
				point = data.last + ((hand - data.last) * t);
				head.birth = effect.GetAge() - (effect.GetDt() * t);
			}
			head.position = point;
			data.lastJoint = point;
			head.baseScale = headScale;
			head.ruleScale = 1.0f;
		}
		data.first = false;
		data.last = hand;
	}

	std::string creator;
	std::vector<int> nextGroups;
	float dieAge;
	float minEmitDist;
	std::string adjustInitialScale;
	bool inTestMode;
};

/// The joints of a chain, all made at once, and the rule is done
class MakeChain final: public Modifier
{
public:
	explicit MakeChain(const ParticleObject& object)
	    : creator(object.String("PCreator"))
	    , numAtoms(object.Int("NumAtoms", 0))
	{
	}

	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		if (const auto* joint = effect.FindCreator(creator))
		{
			for (int i = 0; i < numAtoms; ++i)
			{
				effect.NewAtom(collection, joint, {});
			}
		}
		return false;
	}

	std::string creator;
	int numAtoms;
};
} // namespace

void openblack::particles::RegisterGestureRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("UR_GesturingRecognised", ParticleClassRegistry::Make<GesturingRecognised>);
	registry.AddModifier("ZR_ChainGesture", ParticleClassRegistry::Make<ChainGesture>);
	registry.AddModifier("CreateRuleMakeChain", ParticleClassRegistry::Make<MakeChain>);
}
