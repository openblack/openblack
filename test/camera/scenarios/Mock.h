/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <optional>

#include <3D/LandIsland.h>
#include <Camera/Camera.h>
#include <ECS/Systems/DynamicsSystemInterface.h>
#include <Input/GameActionMapInterface.h>
#include <Locator.h>
#include <Windowing/WindowingInterface.h>
#include <glm/gtx/vec_swizzle.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4716)
#endif

namespace openblack
{
class Camera;
}

constexpr uint16_t k_Width = 800;
constexpr uint16_t k_Height = 600;
constexpr auto k_ScreenCentreLine = []() -> std::array<glm::u16vec2, 16> {
	std::array<glm::u16vec2, 16> results = {};
	const auto x = static_cast<uint16_t>(k_Width / 2);
	for (size_t i = 0; auto& result : results)
	{
		const auto y = i * static_cast<float>(k_Height) / results.size();
		const auto yFloor = static_cast<uint16_t>(y);
		// constexpr round
		result = {x, (y - yFloor) >= 0.5f ? yFloor + 1 : yFloor};
		++i;
	}
	return results;
}();
constexpr glm::u16vec2 k_MockMousePos = {k_Width - 10, k_Height - 10};

const uint32_t k_StabilizeFrames = 200;
const uint32_t k_InteractionFrames = 100;

class MockWindowingSystem final: public openblack::windowing::WindowingInterface
{
	[[nodiscard]] void* GetHandle() const final { assert(false); }
	[[nodiscard]] NativeHandles GetNativeHandles() const final { assert(false); }
	[[nodiscard]] uint32_t GetID() const final { assert(false); }
	[[nodiscard]] glm::ivec2 GetSize() const final { return {k_Width, k_Height}; }
	[[nodiscard]] float GetAspectRatio() const final { return static_cast<float>(k_Width) / static_cast<float>(k_Height); }
	WindowingInterface& SetDisplayMode(openblack::windowing::DisplayMode) final { return *this; }
};

class MockTerrain final: public openblack::LandIslandInterface
{
	[[nodiscard]] float GetHeightAt(glm::vec2) const final { return 0.0f; }
	[[nodiscard]] glm::vec3 GetNormalAt(glm::vec2) const final { return {0.0f, 1.0f, 0.0f}; }
	[[nodiscard]] const openblack::lnd::LNDCell& GetCell(const glm::u16vec2&) const final { assert(false); }
	void DumpTextures() const final { assert(false); }
	void DumpMaps() const final { assert(false); }
	[[nodiscard]] std::vector<openblack::LandBlock>& GetBlocks() final { assert(false); }
	[[nodiscard]] const std::vector<openblack::LandBlock>& GetBlocks() const final { assert(false); }
	[[nodiscard]] const std::vector<openblack::lnd::LNDCountry>& GetCountries() const final { assert(false); }
	[[nodiscard]] const openblack::graphics::Texture2D& GetAlbedoArray() const final { assert(false); }
	[[nodiscard]] const openblack::graphics::Texture2D& GetBump() const final { assert(false); }
	[[nodiscard]] const openblack::graphics::Texture2D& GetHeightMap() const final { assert(false); }
	[[nodiscard]] const openblack::graphics::FrameBuffer& GetFootprintFramebuffer() const final { assert(false); }
	[[nodiscard]] openblack::U16Extent2 GetIndexExtent() const final { assert(false); }
	[[nodiscard]] glm::mat4 GetOrthoView() const final { assert(false); }
	[[nodiscard]] glm::mat4 GetOrthoProj() const final { assert(false); }
	[[nodiscard]] openblack::Extent2 GetExtent() const final { assert(false); }
	uint8_t GetNoise(glm::u8vec2) final { assert(false); }
};

class MockAction: public openblack::input::GameActionInterface
{
public:
	MockAction() = default;
	virtual ~MockAction() = default;

	[[nodiscard]] bool GetBindable(openblack::input::BindableActionMap) const override { return false; }
	[[nodiscard]] bool GetUnbindable(openblack::input::UnbindableActionMap) const override { return false; }
	[[nodiscard]] bool GetBindableChanged(openblack::input::BindableActionMap) const final { return false; }
	[[nodiscard]] bool GetUnbindableChanged(openblack::input::UnbindableActionMap) const final { return false; }
	[[nodiscard]] bool GetBindableRepeat(openblack::input::BindableActionMap) const final { return false; }
	[[nodiscard]] bool GetUnbindableRepeat(openblack::input::UnbindableActionMap) const final { return false; }
	[[nodiscard]] glm::uvec2 GetMousePosition() const override { return k_MockMousePos; }
	[[nodiscard]] glm::ivec2 GetMouseDelta() const final { return {}; }
	[[nodiscard]] std::array<std::optional<glm::vec3>, 2> GetHandPositions() const override { return {}; }
	void Frame() final {}
	void ProcessEvent(const SDL_Event& event) final {}

	uint32_t frameNumber = 0;
};

// The mock land should work perfectly well and if not, there is something wrong with the physics
class MockDynamicsSystem: public openblack::ecs::systems::DynamicsSystemInterface
{
public:
	virtual ~MockDynamicsSystem() = default;

	void Reset() override {}
	void Update(std::chrono::microseconds& dt) override {}
	void AddRigidBody(btRigidBody* object) override {}
	void RegisterRigidBodies() override {}
	void RegisterIslandRigidBodies(openblack::LandIslandInterface& island) override {}
	void UpdatePhysicsTransforms() override {}
	[[nodiscard]] virtual std::optional<glm::vec2> RayCastClosestHitScreenCoord(glm::u16vec2 screenCoord) const = 0;
	[[nodiscard]] std::optional<std::pair<openblack::ecs::components::Transform, openblack::RigidBodyDetails>>
	RayCastClosestHit(const glm::vec3& origin, [[maybe_unused]] const glm::vec3& direction,
	                  [[maybe_unused]] float tMax) const override
	{
		const auto& terrain = openblack::Locator::terrainSystem::value();
		const auto screenCoord = GetWindowCoordinates(origin);
		if (!screenCoord.has_value())
		{
			return std::nullopt;
		}
		auto hit = RayCastClosestHitScreenCoord(*screenCoord);
		if (!hit.has_value())
		{
			return std::nullopt;
		}
		return {{{{hit->x, terrain.GetHeightAt(*hit), hit->y}}, {}}};
	}

	[[nodiscard]] std::optional<glm::u16vec2> GetWindowCoordinates(const glm::vec3& position) const
	{
		glm::vec3 screenPosition;
		const auto size = openblack::Locator::windowing::value().GetSize();
		const auto viewport = glm::vec4(0, 0, size.x, size.y);
		if (!camera->ProjectWorldToScreen(position, viewport, screenPosition, openblack::Camera::Interpolation::Target))
		{
			return std::nullopt;
		}
		// Move point because of precision loss in project/deproject
		return static_cast<glm::u16vec2>(glm::round(glm::round(glm::xy(screenPosition * 10.0f)) / 10.0f));
	}

	uint16_t frameNumber = 0;
	const openblack::Camera* camera;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#pragma GCC diagnostic pop
#pragma clang diagnostic pop
