/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ParticleClassRegistry.h"

#include <algorithm>

#include <glm/geometric.hpp>

#include "ParticleCreators.h"

using namespace openblack::particles;

namespace
{
template <typename Map, typename Value>
void Insert(Map& map, std::string_view key, Value value)
{
	map.insert_or_assign(std::string(key), std::move(value));
}

template <typename Map>
const typename Map::mapped_type* Lookup(const Map& map, std::string_view key)
{
	const auto it = map.find(key);
	return it != map.end() ? &it->second : nullptr;
}

/// The sprite creator's class name; the point creator's is the other the files use
constexpr std::string_view k_SpriteCreator = "ParticleSpriteCreator";
constexpr std::string_view k_PointCreator = "ParticlePointCreator";
} // namespace

void ParticleClassRegistry::AddModifier(std::string_view className, ModifierFactory factory)
{
	Insert(_modifiers, className, std::move(factory));
}

void ParticleClassRegistry::AddCreator(std::string_view className, CreatorFactory factory)
{
	Insert(_creators, className, std::move(factory));
}

void ParticleClassRegistry::AddCollectionCondition(std::string_view className, CollectionCondition condition)
{
	Insert(_collectionConditions, className, std::move(condition));
}

void ParticleClassRegistry::AddAtomCondition(std::string_view className, AtomCondition condition)
{
	Insert(_atomConditions, className, std::move(condition));
}

const ParticleClassRegistry::ModifierFactory* ParticleClassRegistry::FindModifier(std::string_view className) const
{
	return Lookup(_modifiers, className);
}

const ParticleClassRegistry::CreatorFactory* ParticleClassRegistry::FindCreator(std::string_view className) const
{
	return Lookup(_creators, className);
}

const ParticleClassRegistry::CollectionCondition*
ParticleClassRegistry::FindCollectionCondition(std::string_view className) const
{
	return Lookup(_collectionConditions, className);
}

const ParticleClassRegistry::AtomCondition* ParticleClassRegistry::FindAtomCondition(std::string_view className) const
{
	return Lookup(_atomConditions, className);
}

ParticleClassRegistry ParticleClassRegistry::WithAllClasses(CreatorResourcesInterface* resources)
{
	ParticleClassRegistry registry;
	RegisterCreators(registry);
	RegisterDrawnCreators(registry, resources);
	RegisterConditions(registry);
	return registry;
}

std::string openblack::particles::TextureBaseName(std::string_view path)
{
	if (const auto slash = path.find_last_of("\\/"); slash != std::string_view::npos)
	{
		path = path.substr(slash + 1);
	}
	if (const auto dot = path.find_last_of('.'); dot != std::string_view::npos)
	{
		path = path.substr(0, dot);
	}
	return std::string(path);
}

void openblack::particles::ReadCreatorProperties(const psys::ParticleObject& object, Creator& creator)
{
	const auto channel = [&object](std::string_view key) {
		return static_cast<uint8_t>(std::clamp(object.Int(key, 255), 0, 255));
	};
	creator.className = object.className;
	creator.kind = object.className == k_SpriteCreator  ? Creator::Kind::Sprite
	               : object.className == k_PointCreator ? Creator::Kind::Point
	                                                    : Creator::Kind::Other;
	creator.rgba = {channel("ColorR"), channel("ColorG"), channel("ColorB"), channel("ColorA")};
	creator.usePlayerColour = object.Bool("UsePlayerColor", false);
	creator.playerColourBlend = object.Float("UsePlayerColorBlend", 1.0f);
	creator.initialScale = object.Float("InitialScale", 1.0f);
	creator.randomiseScale = object.Bool("RandomiseScale", false);
	creator.loopAnim = object.Bool("LoopAnim", true);
	if (creator.kind != Creator::Kind::Sprite)
	{
		return;
	}
	creator.texture = TextureBaseName(object.String("TextureFileName"));
	creator.fileOffset = object.Int("FileOffset", 0);
	creator.spritesPerRow = std::max(1, object.Int("NumSpritesPerRow", 8));
	creator.numFrames = std::max(1, object.Int("NumFrames", 1));
	creator.initFrame = object.Int("InitFrame", 0);
	creator.randomiseInitFrame = object.Bool("RandomiseInitFrame", false);
	creator.randomiseFrameDirection = object.Bool("RandomiseFrameDirection", false);
	creator.frameRate = object.Float("FrameRate", 1.0f);
	creator.playAnim = object.Bool("PlayAnim", false);
	creator.additive = object.Bool("UseAdditiveAlpha", true);
	creator.writeDepth = object.Bool("MaterialUpdateZBuffer", false);
	creator.scaleAlpha = object.Int("ScaleAlpha", 255);
	creator.stretch = object.Float("StretchVertically", 1.0f);
	// The files spell it so
	creator.horizontal = object.Bool("SetHorozontal", false);
	creator.centreAtBase = object.Bool("CentreAtBase", false);
	creator.ignoreRotation = object.Bool("IgnoreRotation", false);
	creator.origin = {object.Float("SpriteOriginX", 0.0f), object.Float("SpriteOriginY", 0.0f)};
}

void openblack::particles::RegisterCreators(ParticleClassRegistry& registry)
{
	const auto plain = [](const psys::ParticleObject& object) {
		auto creator = std::make_unique<Creator>();
		ReadCreatorProperties(object, *creator);
		return creator;
	};
	registry.AddCreator(k_PointCreator, plain);
	registry.AddCreator(k_SpriteCreator, plain);
}

void openblack::particles::RegisterConditions(ParticleClassRegistry& registry)
{
	registry.AddCollectionCondition("EventConditionTrueOnCloseDown", [](const Effect& effect, const psys::ParticleObject&,
	                                                                    const Collection&) { return effect.Closing(); });
	registry.AddCollectionCondition("EventConditionCollectionDelay",
	                                [](const Effect& effect, const psys::ParticleObject& object, const Collection& c) {
		                                return effect.CollectionAge(c) > object.Float("DelayTime", 0.0f);
	                                });
	registry.AddCollectionCondition("EventConditionCollectionLimitedTime",
	                                [](const Effect& effect, const psys::ParticleObject& object, const Collection& c) {
		                                const float age = effect.CollectionAge(c);
		                                return age >= object.Float("StartTime", 0.0f) && age < object.Float("StopTime", 0.0f);
	                                });
	// While the miracle is still being cast
	registry.AddCollectionCondition(
	    "EventConditionTrueWhenEnabled",
	    [](const Effect& effect, const psys::ParticleObject&, const Collection&) { return effect.GetProcessInfo().enabled; });
	// Approximated: always emitting, until the rules that stop a collection emitting are ported
	registry.AddCollectionCondition("EC_CollectionShouldBeEmitting",
	                                [](const Effect&, const psys::ParticleObject&, const Collection&) { return true; });

	registry.AddAtomCondition("EventConditionAtomDelay",
	                          [](const Effect& effect, const psys::ParticleObject& object, const Atom& atom) {
		                          return effect.AtomAge(atom) > object.Float("DelayTime", 0.0f);
	                          });
	registry.AddAtomCondition("EventConditionAtomLimitedTime",
	                          [](const Effect& effect, const psys::ParticleObject& object, const Atom& atom) {
		                          const float age = effect.AtomAge(atom);
		                          return age >= object.Float("StartTime", 0.0f) && age < object.Float("StopTime", 0.0f);
	                          });
	registry.AddAtomCondition("EventConditionAtomInUse",
	                          [](const Effect&, const psys::ParticleObject&, const Atom& atom) { return atom.visible; });
	registry.AddAtomCondition("EventConditionAtomBelowSpeed",
	                          [](const Effect&, const psys::ParticleObject& object, const Atom& atom) {
		                          return glm::length(atom.velocity) < object.Float("CutOffSpeed", 0.0f);
	                          });
	registry.AddAtomCondition("EventConditionAtomBelowHeight",
	                          [](const Effect& effect, const psys::ParticleObject& object, const Atom& atom) {
		                          const auto p = effect.GlobalPosition(atom);
		                          const float ground = effect.Services().world.LandHeight({p.x, p.z});
		                          return p.y - ground < object.Float("CutOffHeight", 0.0f);
	                          });
	registry.AddAtomCondition("EC_AtomAlphaAbove", [](const Effect&, const psys::ParticleObject& object, const Atom& atom) {
		return static_cast<int>(atom.rgba[3]) > object.Int("AlphaValue", 0);
	});
}
