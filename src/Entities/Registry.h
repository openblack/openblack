/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <map>

#include <bgfx/bgfx.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include <AllMeshes.h>
#include <Graphics/RenderPass.h>
#include <Entities/RegistryContext.h>

namespace openblack
{
class Camera;
}

namespace openblack::graphics
{
class DebugLines;
class ShaderManager;
} // namespace openblack::graphics

namespace openblack::Entities
{
class Registry
{
  public:
	Registry();
	virtual ~Registry();

	void PrepareDraw(bool drawBoundingBox, bool drawStreams);
	void DrawModels(graphics::RenderPass viewId, const graphics::ShaderManager& shaderManager) const;
	decltype(auto) Create() { return _registry.create(); }
	template <typename Component, typename... Args>
	decltype(auto) Assign(entt::entity entity, [[maybe_unused]] Args&&... args) { return _registry.assign<Component>(entity, std::forward<Args>(args)...); }
	decltype(auto) Context() { return _registry.ctx<RegistryContext>(); };
	decltype(auto) Context() const { return _registry.ctx<RegistryContext>(); };
	void Reset()
	{
		auto &renderCtx = Context().renderContext;

		if (bgfx::isValid(renderCtx.instanceUniformBuffer))
			bgfx::destroy(renderCtx.instanceUniformBuffer);

		renderCtx.boundingBox.reset();
		renderCtx.streams.reset();
		_registry.reset();
		_registry.unset<RegistryContext>();
		_registry.set<RegistryContext>();
		_registry.ctx<RegistryContext>().renderContext.instanceUniformBuffer = BGFX_INVALID_HANDLE;
		_dirty = true;
	};
	template <typename Component>
	size_t Size() { return _registry.size<Component>(); }
	template <typename Component>
	decltype(auto) Get(entt::entity entity) { return _registry.get<Component>(entity); }

  private:
	void PrepareDrawDescs(bool drawBoundingBox);
	void PrepareDrawUploadUniforms(bool drawBoundingBox);

	entt::registry _registry;

	bool _dirty;
	bool _hasBoundingBoxes;
};

} // namespace openblack::Entities
