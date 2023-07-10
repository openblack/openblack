/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
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
class RegistryInterface
{
public:
	decltype(auto) Create() { return GetRegistry().create(); }
	template <typename It>
	void Create(It first, It last)
	{
		GetRegistry().create(first, last);
	}
	virtual void Release(entt::entity entity) = 0;
	template <typename It>
	void Release(It first, It last)
	{
		ENTT_ASSERT(std::all_of(first, last, [this](const auto entt) { return orphan(entt); }), "Non-orphan entity");
		GetRegistry().storage<entt::entity>().erase(std::move(first), std::move(last));
	}
	virtual void Destroy(entt::entity entity) = 0;
	template <typename It>
	void Destroy(It first, It last)
	{
		GetRegistry().destroy(first, last);
	}
	template <typename Component, typename... Args>
	decltype(auto) Assign(entt::entity entity, [[maybe_unused]] Args&&... args)
	{
		SetDirty();
		return GetRegistry().emplace<Component>(entity, std::forward<Args>(args)...);
	}
	template <typename Component, typename... Args>
	decltype(auto) AssignOrReplace(entt::entity entity, [[maybe_unused]] Args&&... args)
	{
		SetDirty();
		return GetRegistry().emplace_or_replace<Component>(entity, std::forward<Args>(args)...);
	}
	template <typename Component, typename... Other>
	decltype(auto) Remove(entt::entity entity)
	{
		SetDirty();
		return GetRegistry().remove<Component, Other...>(entity);
	}
	template <typename After, typename Before, typename... Args>
	decltype(auto) SwapComponents(entt::entity entity, [[maybe_unused]] Before previousComponent,
	                              [[maybe_unused]] Args&&... args)
	{
		Remove<Before>(entity);
		return Assign<After>(entity, std::forward<Args>(args)...);
	}
	virtual void SetDirty() = 0;
	virtual RegistryContext& Context() = 0;
	[[nodiscard]] virtual const RegistryContext& Context() const = 0;
	virtual void Reset() = 0;
	template <typename Component>
	size_t Size()
	{
		return GetRegistry().storage<Component>().size();
	}
	template <typename... Components>
	[[nodiscard]] bool AllOf(entt::entity entity) const
	{
		return GetRegistry().all_of<Components...>(entity);
	}
	template <typename... Components>
	[[nodiscard]] bool AnyOf(entt::entity entity) const
	{
		return GetRegistry().any_of<Components...>(entity);
	}
	template <typename... Components>
	decltype(auto) Get(entt::entity entity)
	{
		return GetRegistry().get<Components...>(entity);
	}
	template <typename... Components>
	[[nodiscard]] decltype(auto) Get(entt::entity entity) const
	{
		return GetRegistry().get<Components...>(entity);
	}
	template <typename... Components>
	[[nodiscard]] decltype(auto) Front() const
	{
		return GetRegistry().view<Components...>().front();
	}
	template <typename... Components, typename... Exclude, typename Func>
	decltype(auto) Each(Func func, Exclude... exclude)
	{
		return GetRegistry().view<Components...>(exclude...).each(func);
	}
	template <typename... Components, typename... Exclude, typename Func>
	[[nodiscard]] decltype(auto) Each(Func func, Exclude... exclude) const
	{
		return GetRegistry().view<Components...>(exclude...).each(func);
	}
	template <typename Component>
	[[nodiscard]] decltype(auto) ToEntity(const Component& component) const
	{
		return entt::to_entity(GetRegistry(), component);
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
		return GetRegistry().view<Components...>().size();
	}

private:
	[[nodiscard]] virtual entt::registry& GetRegistry() = 0;
	[[nodiscard]] virtual const entt::registry& GetRegistry() const = 0;
};

} // namespace openblack::ecs
