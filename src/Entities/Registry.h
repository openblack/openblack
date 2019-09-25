#include <entt/entt.hpp>

namespace openblack
{
class Camera;
}

namespace openblack::graphics
{
class ShaderManager;
} // namespace openblack

namespace openblack::Entities
{
class Registry
{
  public:
	void DebugCreateEntities(float x, float y, float z);
	void DrawModels(uint8_t viewId, graphics::ShaderManager &shaderManager);
	void Update();
	decltype(auto) Create() { return _registry.create(); }
	template <typename Component, typename... Args>
	decltype(auto) Assign(entt::entity entity, [[maybe_unused]] Args&&... args) { return _registry.assign<Component>(entity, std::forward<Args>(args)...); }
	void Reset() { _registry.reset(); };
	template <typename Component>
	size_t Size() { return _registry.size<Component>(); }
	template <typename Component>
	decltype(auto) Get(entt::entity entity) { return _registry.get<Component>(entity); }

  private:
	entt::registry _registry;
};

} // namespace openblack
