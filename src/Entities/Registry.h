#include <entt/entt.hpp>

namespace OpenBlack
{
class Camera;
}

namespace OpenBlack
{
namespace Graphics
{
class ShaderManager;
}
} // namespace OpenBlack

namespace OpenBlack
{
namespace Entities
{
class Registry
{
  public:
	void DebugCreateEntities(float x, float y, float z);
	void DrawModels(const Camera& camera, Graphics::ShaderManager& shaderManager);
	void Update();
	decltype(auto) Create() { return _registry.create(); }
	template <typename Component, typename... Args>
	decltype(auto) Assign(entt::entity entity, [[maybe_unused]] Args&&... args) { return _registry.assign<Component>(entity, std::forward<Args>(args)...); }
	void Reset() { _registry.reset(); };

  private:
	entt::registry _registry;
};

} // namespace Entities
} // namespace OpenBlack
