/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <entt/entt.hpp>

#include "ECS/RegistryContext.h"

namespace openblack
{
class Camera;
}

namespace openblack::graphics
{
class DebugLines;
class ShaderManager;
} // namespace openblack::graphics

namespace openblack::ecs
{
class Registry
{
public:
	Registry();

	decltype(auto) Create() { return _registry.create(); }
	template <typename Component, typename... Args>
	decltype(auto) Assign(entt::entity entity, [[maybe_unused]] Args&&... args)
	{
		SetDirty();
		return _registry.emplace<Component>(entity, std::forward<Args>(args)...);
	}
	void SetDirty();
	RegistryContext& Context();
	[[nodiscard]] const RegistryContext& Context() const;
	void Reset()
	{
		SetDirty();
		_registry.clear();
		_registry.set<RegistryContext>();
	};
	template <typename Component>
	size_t Size()
	{
		return _registry.storage<Component>().size();
	}
	template <typename... Components>
	decltype(auto) AllOf(entt::entity entity) const
	{
		return _registry.all_of<Components...>(entity);
	}
	template <typename... Components>
	decltype(auto) Get(entt::entity entity)
	{
		return _registry.get<Components...>(entity);
	}
	template <typename... Components>
	decltype(auto) Get(entt::entity entity) const
	{
		return _registry.get<Components...>(entity);
	}
	template <typename... Components, typename Func>
	decltype(auto) Each(Func func)
	{
		return _registry.view<Components...>().each(func);
	}
	template <typename... Components, typename Func>
	decltype(auto) Each(Func func) const
	{
		return _registry.view<Components...>().each(func);
	}
	template <typename Component>
	decltype(auto) ToEntity(const Component& component) const
	{
		return entt::to_entity(_registry, component);
	}
	template <typename Dst, typename Src>
	decltype(auto) As(Src& component)
	{
		return Get<Dst>(ToEntity(component));
	}
	template <typename Dst, typename Src>
	decltype(auto) As(const Src& component) const
	{
		return Get<Dst>(ToEntity(component));
	}
	template <typename... Components>
	decltype(auto) Size() const
	{
		return _registry.view<Components...>().size();
	}

private:
	entt::registry _registry;
};

} // namespace openblack::ecs
