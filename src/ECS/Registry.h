/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

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
	template <typename It>
	void Create(It first, It last)
	{
		_registry.create(first, last);
	}
	virtual void Release(entt::entity entity);
	template <typename It>
	void Release(It first, It last)
	{
		ENTT_ASSERT(std::all_of(first, last, [this](const auto entt) { return orphan(entt); }), "Non-orphan entity");
		_registry.storage<entt::entity>().erase(std::move(first), std::move(last));
	}
	virtual void Destroy(entt::entity entity);
	template <typename It>
	void Destroy(It first, It last)
	{
		_registry.destroy(first, last);
	}
	template <typename Component, typename... Args>
	decltype(auto) Assign(entt::entity entity, [[maybe_unused]] Args&&... args)
	{
		SetDirty();
		return _registry.emplace<Component>(entity, std::forward<Args>(args)...);
	}
	template <typename Component, typename... Args>
	decltype(auto) AssignOrReplace(entt::entity entity, [[maybe_unused]] Args&&... args)
	{
		SetDirty();
		return _registry.emplace_or_replace<Component>(entity, std::forward<Args>(args)...);
	}
	template <typename Component, typename... Other>
	decltype(auto) Remove(entt::entity entity)
	{
		SetDirty();
		return _registry.remove<Component, Other...>(entity);
	}
	template <typename After, typename Before, typename... Args>
	decltype(auto) SwapComponents(entt::entity entity, [[maybe_unused]] Before previousComponent,
	                              [[maybe_unused]] Args&&... args)
	{
		Remove<Before>(entity);
		return Assign<After>(entity, std::forward<Args>(args)...);
	}
	virtual void SetDirty();
	virtual RegistryContext& Context();
	[[nodiscard]] virtual const RegistryContext& Context() const;
	virtual void Reset();
	template <typename Component>
	size_t Size()
	{
		return _registry.storage<Component>().size();
	}
	template <typename... Components>
	[[nodiscard]] bool AllOf(entt::entity entity) const
	{
		return _registry.all_of<Components...>(entity);
	}
	template <typename... Components>
	[[nodiscard]] bool AnyOf(entt::entity entity) const
	{
		return _registry.any_of<Components...>(entity);
	}
	template <typename... Components>
	decltype(auto) Get(entt::entity entity)
	{
		return _registry.get<Components...>(entity);
	}
	template <typename... Components>
	[[nodiscard]] decltype(auto) Get(entt::entity entity) const
	{
		return _registry.get<Components...>(entity);
	}
	template <typename... Components>
	[[nodiscard]] decltype(auto) Front() const
	{
		return _registry.view<Components...>().front();
	}
	template <typename... Components, typename... Exclude, typename Func>
	decltype(auto) Each(Func func, Exclude... exclude)
	{
		return _registry.view<Components...>(exclude...).each(func);
	}
	template <typename... Components, typename... Exclude, typename Func>
	[[nodiscard]] decltype(auto) Each(Func func, Exclude... exclude) const
	{
		return _registry.view<Components...>(exclude...).each(func);
	}
	template <typename Component>
	[[nodiscard]] decltype(auto) ToEntity(const Component& component) const
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
	[[nodiscard]] decltype(auto) Size() const
	{
		return _registry.view<Components...>().size();
	}
	[[nodiscard]] decltype(auto) Valid(entt::entity entity) const { return _registry.valid(entity); }
	virtual ~Registry() = default;

protected:
	entt::registry _registry;
};

} // namespace openblack::ecs
