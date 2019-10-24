#include <entt/entt.hpp>

namespace openblack
{
class Camera;
}

namespace openblack::graphics
{
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
	Registry()
	{
		_registry.set<RegistryContext>();
	}

	void DebugCreateEntities(float x, float y, float z);
	void DrawModels(uint8_t viewId, graphics::ShaderManager &shaderManager) const;
	void Update();
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
