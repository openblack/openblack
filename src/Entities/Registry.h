#include <entt/entt.hpp>

namespace openblack
{
class Camera;
}

namespace openblack
{
namespace graphics
{
class ShaderManager;
}
} // namespace openblack

namespace openblack
{
namespace Entities
{
class Registry
{
  public:
	void DebugCreateEntities(float x, float y, float z);
	void DrawModels(const Camera& camera, graphics::ShaderManager& shaderManager);
	void Update();
	decltype(auto) Create() { return _registry.create(); }
	template <typename Component, typename... Args>
	decltype(auto) Assign(entt::entity entity, [[maybe_unused]] Args&&... args) { return _registry.assign<Component>(entity, std::forward<Args>(args)...); }
	void Reset() { _registry.reset(); };

  private:
	entt::registry _registry;
};

} // namespace Entities
} // namespace openblack
