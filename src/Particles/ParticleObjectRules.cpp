/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The rules that tie an effect to a game object: an unseen atom kept on each object the effect is given, and atoms let
// out from random points of the object's model under that atom, as the sparkles over a pile of food that speeds up the
// people who take from it.

#include "ParticleObjectRules.h"

#include <cmath>

#include <algorithm>
#include <any>
#include <numbers>
#include <vector>

#include <ParticleFile.h>

#include "ParticleClassRegistry.h"
#include "ParticleEffect.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;

/// An unseen atom on each object the effect is given, one a step, with its next groups under it. Every atom of the
/// collection is kept where its object stands, raised by the offset, until the object is gone; then it stays put.
class CreateRuleGameObjectRef final: public Modifier
{
public:
	explicit CreateRuleGameObjectRef(const ParticleObject& object)
	    : _creator(object.String("PCreator"))
	    , _nextGroups(object.IntArray("NextGroups"))
	    , _alpha(static_cast<uint8_t>(std::clamp(object.Int("Alpha", 255), 0, 255)))
	    , _offsetY(object.Float("OffsetY", 0.0f))
	{
	}
	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& /*slot*/) const override
	{
		auto& world = effect.Services().world;
		if (const auto target = effect.TakeTarget(); target.has_value())
		{
			auto& atom = effect.NewAtom(collection, effect.FindCreator(_creator), _nextGroups);
			atom.rgba[3] = _alpha;
			atom.data[this].object = *target;
		}
		for (auto& atom : collection.atoms)
		{
			const auto found = atom->data.find(this);
			if (found == atom->data.end() || found->second.object == entt::null)
			{
				continue;
			}
			const auto position = world.ObjectPosition(found->second.object);
			if (!position.has_value())
			{
				// Its object has gone: it stays where it was
				found->second.object = entt::null;
				continue;
			}
			atom->position = effect.GlobalToLocal(collection, *position + glm::vec3(0.0f, _offsetY, 0.0f));
		}
		return true;
	}

	/// The object an atom of the rule is kept on, none for any other atom or once it has gone
	[[nodiscard]] entt::entity ObjectOf(const Atom& atom) const
	{
		const auto found = atom.data.find(this);
		return found != atom.data.end() ? found->second.object : entt::null;
	}

private:
	std::string _creator;
	std::vector<int> _nextGroups;
	uint8_t _alpha;
	float _offsetY;
};

/// What the emitter keeps for a collection: how many atoms it is owed and has let out, and the last point it picked
struct EmitterState
{
	float owed {0.0f};
	int emitted {0};
	glm::vec3 point {0.0f};
};

/// Atoms let out at random points of the model of the object the collection's parent atom is kept on, at the most
/// atoms over the age they last, and no more than that many at once; each starts still, where it was let out. They may
/// grow and pulse over their lives, and go once that age is past.
class EmitFromParentAtom final: public Modifier
{
public:
	explicit EmitFromParentAtom(const ParticleObject& object)
	    : _creator(object.String("PCreator"))
	    , _nextGroups(object.IntArray("NextGroups"))
	    , _maxAtoms(object.Int("MaxAtoms", 20))
	    , _pulseMagnitude(object.Float("PulseMagnitude", 1.0f))
	    , _pulseSpeed(object.Float("PulseSpeed", 1.0f))
	    , _ageMaxSize(object.Float("AtomAgeMaxSize", 0.25f))
	    , _ageZeroSize(object.Float("AtomAgeZeroSize", 2.0f))
	    , _parentCondition(object.String("EmitConditionOfParent"))
	    , _doScaling(object.Bool("DoScaling", true))
	    , _deleteAtoms(object.Bool("DeleteAtoms", true))
	    , _onlyAboveLand(object.Bool("EmitOnlyAboveLandscape", false))
	{
	}
	[[nodiscard]] bool Creates() const override { return true; }

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		const auto* atomCreator = effect.FindCreator(_creator);
		auto* parent = collection.parent;
		if (atomCreator == nullptr || parent == nullptr)
		{
			return false;
		}
		if (!slot.data.has_value())
		{
			slot.data = EmitterState {};
		}
		auto& state = std::any_cast<EmitterState&>(slot.data);
		const float rate = static_cast<float>(_maxAtoms) / _ageZeroSize;
		if (!(rate > 0.0f))
		{
			return false;
		}
		if (_parentCondition.empty() || _parentCondition == "NULL" || effect.ConditionForAtom(_parentCondition, *parent))
		{
			Emit(effect, collection, *parent, *atomCreator, state, rate);
		}
		for (size_t i = 0; i < collection.atoms.size();)
		{
			auto& atom = *collection.atoms[i];
			const float age = effect.AtomAge(atom);
			if (_doScaling)
			{
				const float pulse = std::max(0.0f, std::cos(age * _pulseSpeed * k_TwoPi) * _pulseMagnitude * 0.5f + 1.0f);
				const float ramp =
				    age < _ageMaxSize ? age / _ageMaxSize : 1.0f - (age - _ageMaxSize) / (_ageZeroSize - _ageMaxSize);
				atom.ruleScale = std::clamp(ramp, 0.0f, 1.0f) * pulse;
			}
			if (_deleteAtoms && age > _ageZeroSize)
			{
				collection.atoms.erase(collection.atoms.begin() + static_cast<std::ptrdiff_t>(i));
				continue;
			}
			++i;
		}
		return true;
	}

private:
	void Emit(Effect& effect, Collection& collection, const Atom& parent, const Creator& atomCreator, EmitterState& state,
	          float rate) const
	{
		state.owed += effect.GetDt() * rate;
		if (!(state.owed > static_cast<float>(state.emitted)))
		{
			return;
		}
		auto& world = effect.Services().world;
		const auto object = ParentObject(effect, parent);
		do
		{
			if (static_cast<int>(collection.atoms.size()) >= _maxAtoms)
			{
				break;
			}
			++state.emitted;
			// A point of the object's model; where the parent is once the object has gone; the last point picked when
			// there is no model to pick from
			const auto picked =
			    object != entt::null ? world.RandomSurfacePoint(object, effect) : ParticleWorldInterface::SurfacePoint {};
			if (picked.kind == ParticleWorldInterface::SurfacePoint::Kind::Point)
			{
				state.point = picked.position;
			}
			else if (picked.kind == ParticleWorldInterface::SurfacePoint::Kind::Gone)
			{
				state.point = effect.LocalToGlobal(*parent.collection, parent.position);
			}
			if (_onlyAboveLand && !(world.LandHeight({state.point.x, state.point.z}) < state.point.y))
			{
				continue;
			}
			auto& atom = effect.NewAtom(collection, &atomCreator, _nextGroups);
			atom.position = effect.GlobalToLocal(collection, state.point);
		} while (static_cast<float>(state.emitted) < state.owed);
	}

	/// The object the parent atom is kept on by the rule that made it
	static entt::entity ParentObject(const Effect& /*effect*/, const Atom& parent)
	{
		for (const auto& [modifier, data] : parent.data)
		{
			if (dynamic_cast<const CreateRuleGameObjectRef*>(modifier) != nullptr)
			{
				return data.object;
			}
		}
		return entt::null;
	}

	std::string _creator;
	std::vector<int> _nextGroups;
	int _maxAtoms;
	float _pulseMagnitude;
	float _pulseSpeed;
	float _ageMaxSize;
	float _ageZeroSize;
	std::string _parentCondition;
	bool _doScaling;
	bool _deleteAtoms;
	bool _onlyAboveLand;
};
} // namespace

void openblack::particles::RegisterObjectRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("CreateRule_GameObjectRef", ParticleClassRegistry::Make<CreateRuleGameObjectRef>);
	registry.AddModifier("ER_EmitFromParentAtom", ParticleClassRegistry::Make<EmitFromParentAtom>);
}
