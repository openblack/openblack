/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <type_traits>

namespace openblack::ecs::components
{
struct Abode;
struct AtHome;
struct CreatureMorph;
struct Feature;
struct Mesh;
struct MobileStatic;
struct MorphWithTerrain;
struct StoragePit;
struct TempleInteriorPart;
struct Transform;
struct Translucent;
struct Tree;
struct Unlit;
} // namespace openblack::ecs::components

namespace openblack::ecs
{

/// The components the draw lists are made from: which entities are drawn, with which meshes, and how (see
/// systems::RenderingSystem). An entity gaining or losing one of them has the draw lists made again; any other change
/// only has the instances uploaded again.
template <typename Component>
constexpr bool k_ChangesDrawLayout = [] {
	using namespace components;
	using C = std::remove_cv_t<Component>;
	return std::is_same_v<C, Mesh> || std::is_same_v<C, Transform> || std::is_same_v<C, MorphWithTerrain> ||
	       std::is_same_v<C, Tree> || std::is_same_v<C, TempleInteriorPart> || std::is_same_v<C, AtHome> ||
	       std::is_same_v<C, Abode> || std::is_same_v<C, Feature> || std::is_same_v<C, MobileStatic> ||
	       std::is_same_v<C, StoragePit> || std::is_same_v<C, Unlit> || std::is_same_v<C, CreatureMorph> ||
	       std::is_same_v<C, Translucent>;
}();

} // namespace openblack::ecs
