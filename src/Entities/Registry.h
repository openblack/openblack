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

	void PrepareDraw(bool drawBoundingBox, bool drawStreams);
	decltype(auto) Create() { return _registry.create(); }
	template <typename Component, typename... Args>
	decltype(auto) Assign(entt::entity entity, [[maybe_unused]] Args&&... args)
	{
		return _registry.assign<Component>(entity, std::forward<Args>(args)...);
	}
	RegistryContext& Context();
	[[nodiscard]] const RegistryContext& Context() const;
	void Reset()
	{
		_registry.reset();
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

private:
	void PrepareDrawDescs(bool drawBoundingBox);
	void PrepareDrawUploadUniforms(bool drawBoundingBox);

	entt::registry _registry;
};

} // namespace openblack::entities
