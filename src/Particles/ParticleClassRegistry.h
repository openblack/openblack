/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

#include "ParticleEffect.h"

namespace openblack::particles
{
class CreatorResourcesInterface;

/// The particle classes the game runs, by the class names the files use: the rules, the creators and the conditions.
/// A class the files name that is not here does nothing. Each family of classes adds itself with its own Register
/// function; the particle system builds one registry with all of them, and tests build their own.
class ParticleClassRegistry
{
public:
	using ModifierFactory = std::function<std::unique_ptr<Modifier>(const psys::ParticleObject& object)>;
	using CreatorFactory = std::function<std::unique_ptr<Creator>(const psys::ParticleObject& object)>;
	/// The condition's own answer, before the object's InvertResponse
	using CollectionCondition =
	    std::function<bool(const Effect& effect, const psys::ParticleObject& object, const Collection& collection)>;
	using AtomCondition = std::function<bool(const Effect& effect, const psys::ParticleObject& object, const Atom& atom)>;

	/// A later class of the same name replaces the earlier one
	void AddModifier(std::string_view className, ModifierFactory factory);
	void AddCreator(std::string_view className, CreatorFactory factory);
	void AddCollectionCondition(std::string_view className, CollectionCondition condition);
	void AddAtomCondition(std::string_view className, AtomCondition condition);

	/// nullptr when the class is not here
	[[nodiscard]] const ModifierFactory* FindModifier(std::string_view className) const;
	[[nodiscard]] const CreatorFactory* FindCreator(std::string_view className) const;
	[[nodiscard]] const CollectionCondition* FindCollectionCondition(std::string_view className) const;
	[[nodiscard]] const AtomCondition* FindAtomCondition(std::string_view className) const;

	/// A registry of every class the game runs; the creators of models and light maps find them through the resources
	[[nodiscard]] static ParticleClassRegistry WithAllClasses(CreatorResourcesInterface* resources = nullptr);

	/// The usual factory: T(object), or T() for a class without properties
	template <class T>
	static std::unique_ptr<Modifier> Make(const psys::ParticleObject& object)
	{
		if constexpr (std::is_constructible_v<T, const psys::ParticleObject&>)
		{
			return std::make_unique<T>(object);
		}
		else
		{
			return std::make_unique<T>();
		}
	}

private:
	std::map<std::string, ModifierFactory, std::less<>> _modifiers;
	std::map<std::string, CreatorFactory, std::less<>> _creators;
	std::map<std::string, CollectionCondition, std::less<>> _collectionConditions;
	std::map<std::string, AtomCondition, std::less<>> _atomConditions;
};

/// The common properties of every particle creator, and the sprite creator's own
void ReadCreatorProperties(const psys::ParticleObject& object, Creator& creator);
/// The base name of a texture path as the files give it: ".\Data\Textures\S_Fire.raw" is "S_Fire"
[[nodiscard]] std::string TextureBaseName(std::string_view path);

/// The creators of points and sprites
void RegisterCreators(ParticleClassRegistry& registry);
/// The conditions on an effect's, a collection's or an atom's age, state and place
void RegisterConditions(ParticleClassRegistry& registry);
/// The rules that create atoms: once, in a ball, by emitters of several shapes, and along a moving parent's trail
void RegisterCreateRules(ParticleClassRegistry& registry);
/// The rules that remove atoms, fade and scale them, and move and turn them
void RegisterUpdateRules(ParticleClassRegistry& registry);
/// The rules that follow curves over an atom's life, and that line sprites up with their movement
void RegisterCurveRules(ParticleClassRegistry& registry);
/// The heal miracle's rules: the chakra over each person it heals, the burst of sparks under it, and the chakras'
/// wiggle in the hand
void RegisterHealRules(ParticleClassRegistry& registry);
/// The rules that keep the miracle held in the hand on it, and the sprinkling miracles' source on it
void RegisterHandRules(ParticleClassRegistry& registry);
/// The rules that start and stop the sounds particles keep going
void RegisterSoundRules(ParticleClassRegistry& registry);
/// The fireball's rules: its throw, flight, bounce, trail and the event it sends every step
void RegisterFireballRules(ParticleClassRegistry& registry);
/// The lightning bolt's rules: its targets, its forks and its strikes
void RegisterLightningRules(ParticleClassRegistry& registry);
/// The shield's rules: its sphere, its sparks and its dome, and what turns other effects' particles away from it
void RegisterShieldRules(ParticleClassRegistry& registry);

} // namespace openblack::particles
