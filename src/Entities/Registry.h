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

namespace openblack::Entities
{
class RegistryContext
{
  public:
	std::unordered_map<int, entt::entity> streams;
	std::unordered_map<int, entt::entity> towns;
};

class Registry
{
  public:
	Registry();
	virtual ~Registry();

	void DrawModels(uint8_t viewId, graphics::ShaderManager &shaderManager, graphics::DebugLines* boundingBox) const;
	decltype(auto) Create() { return _registry.create(); }
	template <typename Component, typename... Args>
	decltype(auto) Assign(entt::entity entity, [[maybe_unused]] Args&&... args) { return _registry.assign<Component>(entity, std::forward<Args>(args)...); }
	void Reset()
	{
		_registry.reset();
		_registry.unset<RegistryContext>();
		_registry.set<RegistryContext>();
	};
	template <typename Component>
	size_t Size() { return _registry.size<Component>(); }
	template <typename Component>
	decltype(auto) Get(entt::entity entity) { return _registry.get<Component>(entity); }
	decltype(auto) Context() { return _registry.ctx<RegistryContext>(); }

  private:
	entt::registry _registry;
};

} // namespace openblack::Entities
