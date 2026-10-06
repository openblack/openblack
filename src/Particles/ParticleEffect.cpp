/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ParticleEffect.h"

#include <algorithm>
#include <iterator>
#include <ranges>

#include <glm/matrix.hpp>

#include "Common/GameRandom.h"
#include "ParticleClassRegistry.h"

using namespace openblack;
using namespace openblack::particles;

namespace
{
/// A sprite drawn with a random size takes between this and 1 of its creator's scale
constexpr float k_RandomScaleMinimum = 0.3f;
constexpr float k_RandomScaleRange = 1.0f - k_RandomScaleMinimum;
/// The random number every atom carries is below this
constexpr int32_t k_AtomRandomRange = 0x100;
/// Above this of k_AtomRandomRange an atom plays its animation backwards, when its creator asks for half to
constexpr int32_t k_BackwardsAbove = 0x80;
/// An atom fainter than this is not drawn
constexpr float k_MinimumDrawnAlpha = 1.0f;

/// A class the game does not run yet: it is attached so its group behaves the same, but it does nothing
class UnportedModifier final: public Modifier
{
public:
	[[nodiscard]] bool Unported() const override { return true; }
};

bool IsCreatorClass(std::string_view className)
{
	return className.starts_with("Particle") && className.find("Creator") != std::string_view::npos;
}
} // namespace

bool Modifier::ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const
{
	std::erase_if(collection.atoms, [&](const std::unique_ptr<Atom>& atom) {
		return effect.ConditionForAtom(condition, *atom) && !ModifyAtom(effect, *atom, slot);
	});
	return true;
}

Effect::Effect(std::shared_ptr<const psys::ParticleFile> file, EffectServices services, glm::vec3 origin, float magnitude,
               bool synced)
    : _file(std::move(file))
    , _services(services)
    , _origin(origin)
    , _magnitude(magnitude)
    , _synced(synced)
{
	const auto hierarchies = _file->header.IntArray("Hierarchies");
	for (size_t g = 0; g < _hierarchies.size() && g < hierarchies.size(); ++g)
	{
		_hierarchies.at(g) = hierarchies[g] != 0;
	}
	_deleteOnCloseDown = _file->header.Bool("DeleteOnCloseDown", true);
	_maxSpellAge = _file->header.Float("MaxSpellAge", -1.0f);

	for (const auto& object : _file->objects)
	{
		if (const auto* factory = _services.classes.FindCreator(object.className))
		{
			_creators.insert_or_assign(object.name, (*factory)(object));
			continue;
		}
		if (IsCreatorClass(object.className))
		{
			// Meshes, ribbons and the like make atoms that are not drawn yet
			auto creator = std::make_unique<Creator>();
			ReadCreatorProperties(object, *creator);
			_creators.insert_or_assign(object.name, std::move(creator));
			if (std::ranges::find(_unported, object.className) == _unported.end())
			{
				_unported.push_back(object.className);
			}
			continue;
		}
		std::unique_ptr<Modifier> modifier;
		if (const auto* factory = _services.classes.FindModifier(object.className))
		{
			modifier = (*factory)(object);
		}
		else if (object.Has("Group"))
		{
			modifier = std::make_unique<UnportedModifier>();
			if (std::ranges::find(_unported, object.className) == _unported.end())
			{
				_unported.push_back(object.className);
			}
		}
		if (!modifier)
		{
			// Conditions and float providers are looked up by name when they are used
			continue;
		}
		modifier->group = object.Int("Group", -1);
		modifier->removeOnCloseDown = object.Bool("RemoveOnCloseDown", false);
		modifier->condition = object.String("Condition");
		if (modifier->group >= 0 && modifier->group < static_cast<int>(k_GroupCount))
		{
			_groups.at(static_cast<size_t>(modifier->group)).push_back(modifier.get());
			_modifiers.push_back(std::move(modifier));
		}
	}

	// The groups made at the start, at the origin
	const auto initially = _file->header.IntArray("InitiallyCreated");
	for (size_t g = 0; g < initially.size() && g < k_GroupCount; ++g)
	{
		if (initially[g] != 0)
		{
			CreateCollection(static_cast<int>(g), nullptr, _roots);
		}
	}
}

Atom::~Atom()
{
	for (const auto& sound : sounds)
	{
		sound->atom = nullptr;
	}
}

Effect::~Effect() = default;

void Effect::SetSink(SpellSink* sink)
{
	_sink = sink;
	if (_sink != nullptr)
	{
		SendSpellEvent({.type = SpellEventInfo::Type::Started, .position = _origin});
	}
}

bool Effect::SendSpellEvent(const SpellEventInfo& event) const
{
	return _sink != nullptr && _sink->SpellEvent(event);
}

bool Effect::IsHierarchy(int group) const
{
	return group >= 0 && group < static_cast<int>(k_GroupCount) && _hierarchies.at(static_cast<size_t>(group));
}

float Effect::Random(float max)
{
	return _services.random.ParticleFloatRand(max);
}

float Effect::Random(float min, float max)
{
	return _services.random.ParticleFloatRand(min, max);
}

int32_t Effect::Rand(int32_t n)
{
	return _services.random.ParticleRand(n);
}

glm::vec3 Effect::RandomInBall()
{
	return _services.random.ParticleRandR3();
}

float Effect::FloatProvider(const std::string& name, float fallback) const
{
	const auto it = _floatValues.find(name);
	return it == _floatValues.end() ? fallback : it->second;
}

bool Effect::ConditionForCollection(const std::string& name, const Collection& collection) const
{
	const auto* object = _file->Find(name);
	if (object == nullptr)
	{
		return true;
	}
	const auto* test = _services.classes.FindCollectionCondition(object->className);
	if (test == nullptr)
	{
		// An atom's condition is asked of each atom; one the game does not run yet holds
		return true;
	}
	return (*test)(*this, *object, collection) != object->Bool("InvertResponse", false);
}

bool Effect::ConditionForAtom(const std::string& name, const Atom& atom) const
{
	const auto* object = _file->Find(name);
	if (object == nullptr)
	{
		return true;
	}
	const auto* test = _services.classes.FindAtomCondition(object->className);
	if (test == nullptr)
	{
		return ConditionForCollection(name, *atom.collection);
	}
	return (*test)(*this, *object, atom) != object->Bool("InvertResponse", false);
}

const Creator* Effect::FindCreator(const std::string& name) const
{
	const auto it = _creators.find(name);
	return it == _creators.end() ? nullptr : it->second.get();
}

glm::vec3 Effect::GlobalPosition(const Atom& atom) const
{
	return atom.collection != nullptr ? LocalToGlobal(*atom.collection, atom.position) : atom.position;
}

glm::vec3 Effect::FrameScale(const Atom& atom)
{
	const float s = atom.baseScale * atom.ruleScale;
	return {s, s * atom.stretch, s};
}

glm::vec3 Effect::LocalToGlobal(const Collection& collection, const glm::vec3& local) const
{
	glm::vec3 p = local;
	if (!collection.hierarchy)
	{
		return p;
	}
	for (const Atom* a = collection.parent; a != nullptr && a->collection != nullptr; a = a->collection->parent)
	{
		if (IsHierarchy(a->collection->group))
		{
			p = a->position + a->rotation * (FrameScale(*a) * p);
		}
	}
	return p;
}

glm::vec3 Effect::GlobalToLocal(const Collection& collection, const glm::vec3& global) const
{
	if (!collection.hierarchy)
	{
		return global;
	}
	std::vector<const Atom*> frames;
	for (const Atom* a = collection.parent; a != nullptr && a->collection != nullptr; a = a->collection->parent)
	{
		if (IsHierarchy(a->collection->group))
		{
			frames.push_back(a);
		}
	}
	glm::vec3 p = global;
	for (auto it = frames.rbegin(); it != frames.rend(); ++it)
	{
		const auto s = FrameScale(**it);
		p = glm::inverse((*it)->rotation) * (p - (*it)->position);
		p = {s.x != 0.0f ? p.x / s.x : 0.0f, s.y != 0.0f ? p.y / s.y : 0.0f, s.z != 0.0f ? p.z / s.z : 0.0f};
	}
	return p;
}

glm::vec3 Effect::SpawnPosition(const Collection& collection) const
{
	if (collection.parent == nullptr)
	{
		return _origin;
	}
	const auto* parentCollection = collection.parent->collection;
	if (parentCollection != nullptr && IsHierarchy(parentCollection->group))
	{
		return glm::vec3(0.0f);
	}
	return collection.parent->position;
}

Atom& Effect::NewAtom(Collection& collection, const Creator* creator, std::span<const int> nextGroups)
{
	auto atom = std::make_unique<Atom>();
	atom->collection = &collection;
	atom->creator = creator;
	atom->birth = _age;
	atom->position = SpawnPosition(collection);
	atom->random = static_cast<uint32_t>(Rand(k_AtomRandomRange));
	if (creator != nullptr)
	{
		atom->rgba = creator->rgba;
		if (creator->usePlayerColour && _player >= 0)
		{
			atom->rgba =
			    maths::TintWithPlayerColour(atom->rgba, _services.world.PlayerColour(_player), creator->playerColourBlend);
		}
		atom->baseScale = creator->initialScale;
		// Only sprites draw a random size; the other creators that want one draw it themselves
		if (creator->kind == Creator::Kind::Sprite && creator->randomiseScale)
		{
			const float r = Random(k_RandomScaleRange);
			const float scale = r + k_RandomScaleMinimum;
			atom->baseScale = scale * creator->initialScale;
		}
		atom->stretch = creator->stretch;
		if (creator->kind == Creator::Kind::Sprite)
		{
			atom->frame = creator->randomiseInitFrame ? static_cast<float>(Rand(creator->numFrames))
			                                          : static_cast<float>(creator->initFrame);
			atom->frameRate = creator->frameRate;
			atom->playAnim = creator->playAnim;
			if (creator->randomiseFrameDirection && Rand(k_AtomRandomRange) > k_BackwardsAbove)
			{
				atom->frameRate = -atom->frameRate;
			}
		}
		creator->InitAtom(*this, *atom);
	}
	auto& result = *atom;
	collection.atoms.push_back(std::move(atom));
	for (const int group : nextGroups)
	{
		if (group >= 0 && group < static_cast<int>(k_GroupCount))
		{
			CreateCollection(group, &result, result.subCollections);
		}
	}
	return result;
}

void Effect::AddSubCollections(Atom& atom, std::span<const int> groups)
{
	for (const int group : groups)
	{
		if (group >= 0 && group < static_cast<int>(k_GroupCount))
		{
			CreateCollection(group, &atom, atom.subCollections);
		}
	}
}

Atom* Effect::NewAtomInGroup(int group, const Creator* creator)
{
	if (group < 0 || group >= static_cast<int>(k_GroupCount))
	{
		return nullptr;
	}
	auto found = std::ranges::find(_roots, group, [](const auto& root) { return root->group; });
	if (found == _roots.end())
	{
		CreateCollection(group, nullptr, _roots);
		found = std::prev(_roots.end());
	}
	return &NewAtom(**found, creator, {});
}

void Effect::CreateCollection(int group, Atom* parent, std::vector<std::unique_ptr<Collection>>& into)
{
	auto collection = std::make_unique<Collection>();
	collection->group = group;
	collection->parent = parent;
	collection->birth = _age;
	// In a hierarchy when any ancestor's group is one, not only the parent's
	collection->hierarchy = parent != nullptr && (IsHierarchy(parent->collection->group) || parent->collection->hierarchy);
	for (const auto* modifier : _groups.at(static_cast<size_t>(group)))
	{
		collection->modifiers.push_back({.modifier = modifier});
	}
	into.push_back(std::move(collection));
}

void Effect::UpdateCollection(Collection& collection)
{
	for (auto& slot : collection.modifiers)
	{
		if (!slot.attached)
		{
			continue;
		}
		if (_closing && slot.modifier->removeOnCloseDown)
		{
			slot.attached = false;
			continue;
		}
		if (!ConditionForCollection(slot.modifier->condition, collection))
		{
			continue;
		}
		if (!slot.modifier->ModifyCollection(*this, collection, slot))
		{
			slot.attached = false;
		}
	}
	// By index: a rule may add a collection to an atom while they are walked, and it runs this step too
	for (size_t a = 0; a < collection.atoms.size(); ++a)
	{
		auto& atom = collection.atoms[a];
		for (size_t s = 0; s < atom->subCollections.size(); ++s)
		{
			UpdateCollection(*atom->subCollections[s]);
		}
	}
}

void Effect::PostUpdate(Collection& collection, const glm::vec3& parentPosition, const glm::mat3& parentRotation,
                        const glm::vec3& parentScale)
{
	// The parent's frame is that of the nearest ancestor atom whose group is a hierarchy; its scale also scales the atoms
	// drawn in it
	const bool flagged = IsHierarchy(collection.group);
	for (auto& atom : collection.atoms)
	{
		atom->previous = atom->current;
		auto& draw = atom->current;
		draw.rotation = collection.hierarchy ? parentRotation * atom->rotation : atom->rotation;
		draw.position =
		    collection.hierarchy ? parentPosition + parentRotation * (parentScale * atom->position) : atom->position;
		// Approximated: the drawn size takes its parents' scale across, not their stretch
		draw.scale = atom->baseScale * atom->ruleScale * (collection.hierarchy ? parentScale.x : 1.0f);
		draw.stretch = atom->stretch;
		draw.alpha = static_cast<float>(atom->rgba[3]) * collection.alpha / 255.0f;
		float previousFrame = atom->frame;
		maths::AdvanceFrame(previousFrame, atom->frame, _dt, atom->frameRate,
		                    atom->creator != nullptr ? atom->creator->numFrames : 0, atom->playAnim);
		atom->previous.frame = previousFrame;
		draw.frame = atom->frame;
		if (!atom->drawn)
		{
			atom->previous = draw;
			atom->drawn = true;
		}
		const glm::vec3 subScale =
		    flagged ? FrameScale(*atom) * (collection.hierarchy ? parentScale : glm::vec3(1.0f)) : parentScale;
		for (auto& sub : atom->subCollections)
		{
			PostUpdate(*sub, flagged ? draw.position : parentPosition, flagged ? draw.rotation : parentRotation, subScale);
		}
	}
}

void Effect::UpdateFloatProviders()
{
	_floatValues.clear();
	for (const auto& object : _file->objects)
	{
		const auto& c = object.className;
		if (!c.ends_with("FloatProvider"))
		{
			continue;
		}
		float value = 1.0f;
		const float scaleBy = object.Float("ScaleBy", 1.0f);
		if (c == "ConstFloatProvider")
		{
			value = object.Float("ConstValue", 1.0f);
		}
		else if (c == "MagnitudeFloatProvider" || c == "MagnitudeTimesStrengthFloatProvider" || c == "StrengthFloatProvider")
		{
			float base = _magnitude;
			if (c == "StrengthFloatProvider")
			{
				base = _info.power;
			}
			else if (c == "MagnitudeTimesStrengthFloatProvider")
			{
				base = _magnitude * _info.power;
			}
			value = std::clamp(base * scaleBy, object.Float("Minimum", -1e6f), object.Float("Maximum", 1e6f));
		}
		else if (c == "RenderHandScaleFloatProvider" || c == "RenderHandScaleTimesStrengthFloatProvider")
		{
			// The hand's drawn scale reaches the effect as its magnitude
			value = _magnitude * scaleBy;
		}
		_floatValues.insert_or_assign(object.name, value);
	}
}

void Effect::Step(float dt)
{
	// Its random numbers come from its own stream during the step, and give 0 outside it
	const ParticleRandomStep step(_services.random, _synced);
	_dt = dt;
	UpdateFloatProviders();
	for (auto& root : _roots)
	{
		UpdateCollection(*root);
	}
	_atomCount = 0;
	const auto count = [this](const auto& self, const Collection& c) -> void {
		_atomCount += c.atoms.size();
		for (const auto& atom : c.atoms)
		{
			for (const auto& sub : atom->subCollections)
			{
				self(self, *sub);
			}
		}
	};
	for (auto& root : _roots)
	{
		PostUpdate(*root, glm::vec3(0.0f), glm::mat3(1.0f), glm::vec3(1.0f));
		count(count, *root);
	}
	_age += dt;
}

void Effect::CloseDown()
{
	if (!_closing)
	{
		_closing = true;
		_closeAge = _age;
	}
}

bool Effect::AnyCreatorLeft(const Collection& collection) const
{
	for (const auto& slot : collection.modifiers)
	{
		const bool creates = slot.modifier->Creates() || (!_closing && slot.modifier->KeepsAlive()) ||
		                     (_sink != nullptr && !_closing && slot.modifier->Unported());
		if (slot.attached && creates && !(_closing && slot.modifier->removeOnCloseDown))
		{
			return true;
		}
	}
	return std::ranges::any_of(collection.atoms, [this](const auto& atom) {
		return std::ranges::any_of(atom->subCollections, [this](const auto& sub) { return AnyCreatorLeft(*sub); });
	});
}

bool Effect::Finished() const
{
	if (_maxSpellAge > 0.0f && _age > _maxSpellAge)
	{
		return true;
	}
	if (_atomCount != 0)
	{
		return false;
	}
	return std::ranges::none_of(_roots, [this](const auto& root) { return AnyCreatorLeft(*root); });
}

size_t Effect::CollectionCount() const
{
	const auto count = [](const auto& self, const Collection& c) -> size_t {
		size_t total = 1;
		for (const auto& atom : c.atoms)
		{
			for (const auto& sub : atom->subCollections)
			{
				total += self(self, *sub);
			}
		}
		return total;
	};
	size_t total = 0;
	for (const auto& root : _roots)
	{
		total += count(count, *root);
	}
	return total;
}

std::optional<Effect::DrawAtom> Effect::Interpolate(const Atom& atom, float t, bool interpolated) const
{
	const float k = interpolated ? std::clamp(t, 0.0f, 1.0f) : 1.0f;
	const auto& a = atom.previous;
	const auto& b = atom.current;
	const float alpha = (a.alpha + (b.alpha - a.alpha) * k) * _globalAlpha / 255.0f;
	if (!atom.visible || !atom.drawn || atom.creator == nullptr)
	{
		return std::nullopt;
	}
	// The drawn time lies between the last two steps; the effect's age is the current step's end
	const float age = _age - (_dt * (1.0f - k)) - atom.birth;
	return DrawAtom {
	    .creator = atom.creator,
	    .position = a.position + (b.position - a.position) * k,
	    .rotation = a.rotation + (b.rotation - a.rotation) * k,
	    .scale = a.scale + (b.scale - a.scale) * k,
	    .stretch = a.stretch + (b.stretch - a.stretch) * k,
	    .alpha = alpha,
	    .frame = maths::LerpFrame(a.frame, b.frame, t, atom.creator->loopAnim),
	    .rgb = {atom.rgba[0], atom.rgba[1], atom.rgba[2]},
	    .age = std::max(age, 0.0f),
	    .creatorValue = atom.creatorValue,
	};
}

void Effect::WalkCollection(const Collection& collection, float t, DrawWalk& out) const
{
	// The newest atom first, as the game keeps its lists
	const Creator* chainCreator = nullptr;
	for (const auto& atom : std::ranges::reverse_view(collection.atoms))
	{
		const auto drawn = Interpolate(*atom, t, collection.interpolated);
		if (!drawn.has_value())
		{
			continue;
		}
		const auto kind = drawn->creator->kind;
		if (kind == Creator::Kind::Chain)
		{
			chainCreator = drawn->creator;
		}
		else if (kind != Creator::Kind::Point && kind != Creator::Kind::Other && drawn->alpha >= k_MinimumDrawnAlpha)
		{
			out.steps.push_back({.chain = false, .index = static_cast<uint32_t>(out.atoms.size())});
			out.atoms.push_back(*drawn);
		}
	}
	// Then the ribbon through its joints, from the first made
	if (chainCreator != nullptr)
	{
		const auto first = static_cast<uint32_t>(out.joints.size());
		for (const auto& atom : collection.atoms)
		{
			if (atom->creator != nullptr && atom->creator->kind == Creator::Kind::Chain)
			{
				if (const auto joint = Interpolate(*atom, t, collection.interpolated))
				{
					out.joints.push_back(*joint);
				}
			}
		}
		const auto count = static_cast<uint32_t>(out.joints.size()) - first;
		if (count >= 2)
		{
			out.steps.push_back({.chain = true, .index = static_cast<uint32_t>(out.chains.size())});
			out.chains.push_back({.creator = chainCreator,
			                      .firstJoint = first,
			                      .jointCount = count,
			                      .textureRepeats = collection.textureRepeats});
		}
		else
		{
			out.joints.resize(first);
		}
	}
	// Then what is under each atom in turn
	for (const auto& atom : std::ranges::reverse_view(collection.atoms))
	{
		for (const auto& sub : std::ranges::reverse_view(atom->subCollections))
		{
			WalkCollection(*sub, t, out);
		}
	}
}

void Effect::Walk(float t, DrawWalk& out) const
{
	for (const auto& root : std::ranges::reverse_view(_roots))
	{
		WalkCollection(*root, t, out);
	}
}

void Effect::Collect(float t, std::vector<DrawAtom>& out, Creator::Kind kind) const
{
	DrawWalk walk;
	Walk(t, walk);
	if (kind == Creator::Kind::Chain)
	{
		out.insert(out.end(), walk.joints.begin(), walk.joints.end());
		return;
	}
	std::ranges::copy_if(walk.atoms, std::back_inserter(out),
	                     [kind](const DrawAtom& atom) { return atom.creator->kind == kind; });
}

std::optional<entt::entity> Effect::TakeTarget()
{
	if (_targets.empty())
	{
		return std::nullopt;
	}
	const auto target = _targets.back();
	_targets.pop_back();
	return target;
}
