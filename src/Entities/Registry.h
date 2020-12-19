/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Entities/RegistryContext.h"
#include "Graphics/RenderPass.h"

#include <entt/entt.hpp>

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
		return _registry.size<Component>();
	}
	template <typename Component>
	decltype(auto) Get(entt::entity entity)
	{
		return _registry.get<Component>(entity);
	}
	template <typename... Components, typename Func>
	decltype(auto) Each(Func func)
	{
		return _registry.view<Components...>().each(func);
	}
	template <typename... Components, typename Func>
	decltype(auto) Each(Func func) const
	{
		return _registry.view<const Components...>().each(func);
	}

private:
	void PrepareDrawDescs(bool drawBoundingBox);
	void PrepareDrawUploadUniforms(bool drawBoundingBox);

	entt::registry _registry;
};

} // namespace openblack::entities
