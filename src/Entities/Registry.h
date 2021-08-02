/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <entt/entt.hpp>

#include "Entities/RegistryContext.h"
#include "Graphics/RenderPass.h"

namespace openblack
{
class Camera;
}

namespace openblack::graphics
{
class DebugLines;
class ShaderManager;
} // namespace openblack::graphics

namespace openblack::entities
{
class Registry
{
public:
	Registry();

	void PrepareDraw(bool drawBoundingBox, bool drawFootpaths, bool drawStreams);
	decltype(auto) Create() { return _registry.create(); }
	template<typename It>
	void Create(It first, It last) { _registry.create(first, last); }
	decltype(auto) Destroy(entt::entity entity) { return _registry.destroy(entity); }
	template <typename Component>
	decltype(auto) Remove(entt::entity entity) { return _registry.remove<Component>(entity); }
	template <typename Component>
	decltype(auto) DestroyAll()
	{
		auto entities = _registry.view<Component>();
		return _registry.destroy(entities.begin(), entities.end());
	}
	template <typename It>
	decltype(auto) DestroyAll(It first, It last)
	{
		return _registry.destroy(first, last);
	}
	template <typename Component, typename... Args>
	decltype(auto) Assign(entt::entity entity, [[maybe_unused]] Args&&... args)
	{
		SetDirty();
		return _registry.emplace<Component>(entity, std::forward<Args>(args)...);
	}
	decltype(auto) Valid(entt::entity entity)
	{
		return _registry.valid(entity);
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
		return _registry.size<Component>();
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
	template <typename Component>
	decltype(auto) OnConstruct()
	{
		return _registry.on_construct<Component>();
	}
	template <typename Component>
	decltype(auto) OnUpdate()
	{
		return _registry.on_update<Component>();
	}
	template <typename Component>
	decltype(auto) OnDestroy()
	{
		return _registry.on_destroy<Component>();
	}

private:
	void PrepareDrawDescs(bool drawBoundingBox);
	void PrepareDrawUploadUniforms(bool drawBoundingBox);

	entt::registry _registry;
};

} // namespace openblack::entities
