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
	[[nodiscard]] bool GetBindable(openblack::input::BindableActionMap) const final { return false; }
	[[nodiscard]] bool GetUnbindable(openblack::input::UnbindableActionMap) const final { return false; }
	[[nodiscard]] bool GetBindableChanged(openblack::input::BindableActionMap) const final { return false; }
	[[nodiscard]] bool GetUnbindableChanged(openblack::input::UnbindableActionMap) const final { return false; }
	[[nodiscard]] bool GetBindableRepeat(openblack::input::BindableActionMap) const final { return false; }
	[[nodiscard]] bool GetUnbindableRepeat(openblack::input::UnbindableActionMap) const final { return false; }
	[[nodiscard]] glm::uvec2 GetMousePosition() const final { return k_MockMousePos; }
	[[nodiscard]] glm::ivec2 GetMouseDelta() const final { return {}; }
	[[nodiscard]] std::array<std::optional<glm::vec3>, 2> GetHandPositions() const final { return {}; }
	void Frame() final {}
	void ProcessEvent(const SDL_Event& event) final {}
};

// The mock land should work perfectly well and if not, there is something wrong with the physics
class MockDynamicsSystem: public openblack::ecs::systems::DynamicsSystemInterface
{
public:
	explicit MockDynamicsSystem(openblack::Camera& camera)
	    : _camera(camera) {};

	void Reset() override {}
	void Update(std::chrono::microseconds& dt) override {}
	void AddRigidBody(btRigidBody* object) override {}
	void RegisterRigidBodies() override {}
	void RegisterIslandRigidBodies(openblack::LandIslandInterface& island) override {}
	void UpdatePhysicsTransforms() override {}
	[[nodiscard]] std::optional<std::pair<openblack::ecs::components::Transform, openblack::RigidBodyDetails>>
	RayCastClosestHit(const glm::vec3& origin, [[maybe_unused]] const glm::vec3& direction,
	                  [[maybe_unused]] float tMax) const override
	{
		const auto& terrain = openblack::Locator::terrainSystem::value();
		const auto screenCoord = GetWindowCoordinates(origin);

		auto makeHit = [&terrain](float x,
		                          float z) -> std::pair<openblack::ecs::components::Transform, openblack::RigidBodyDetails> {
			auto p = glm::vec2(x, z);
			return {{{p.x, terrain.GetHeightAt(p), p.y}}, {}};
		};

		if (screenCoord == k_ScreenCentreLine[0])
		{
			switch (_frameNumber)
			{
			case 0:
				return std::nullopt;
			}
		}
		else if (screenCoord == k_ScreenCentreLine[1])
		{
			switch (_frameNumber)
			{
			case 0:
				return std::nullopt;
			}
		}
		else if (screenCoord == k_ScreenCentreLine[2])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, 563.2467041f);
			}
		}
		else if (screenCoord == k_ScreenCentreLine[3])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, 189.2355652f);
			}
		}
		else if (screenCoord == k_ScreenCentreLine[4])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, 100.4535294f);
			}
		}
		else if (screenCoord == k_ScreenCentreLine[5])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, 62.1956444f);
			}
		}
		else if (screenCoord == k_ScreenCentreLine[6])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, 39.9825211f);
			}
		}
		else if (screenCoord == k_ScreenCentreLine[7])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, 26.0952835f);
			}
		}
		else if (screenCoord == k_ScreenCentreLine[8])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, 16.1538353f);
			}
		}
		else if (screenCoord == k_ScreenCentreLine[9])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, 9.0254517f);
			}
		}
		else if (screenCoord == k_ScreenCentreLine[10])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, 3.4066925f);
			}
		}
		else if (screenCoord == k_ScreenCentreLine[11])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, -0.9229774f);
			}
		}
		else if (screenCoord == k_ScreenCentreLine[12])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, -4.5307636f);
			}
		}
		else if (screenCoord == k_ScreenCentreLine[13])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, -7.4373074f);
			}
		}
		else if (screenCoord == k_ScreenCentreLine[14])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, -9.9486294f);
			}
		}
		else if (screenCoord == k_ScreenCentreLine[15])
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(0.0f, -12.0339508f);
			}
		}
		else if (screenCoord == k_MockMousePos)
		{
			switch (_frameNumber)
			{
			case 0:
				return makeHit(15.8141842f, -13.4212151f);
			}
		}
		assert(false); // Shouldn't be any unaccounted raycasts
		return std::nullopt;
	}

	[[nodiscard]] glm::u16vec2 GetWindowCoordinates(const glm::vec3& position) const
	{
		glm::vec3 screenPosition;
		const auto size = openblack::Locator::windowing::value().GetSize();
		const auto viewport = glm::vec4(0, 0, size.x, size.y);
		_camera.ProjectWorldToScreen(position, viewport, screenPosition, openblack::Camera::Interpolation::Target);
		// Move point because of precision loss in project/deproject
		return static_cast<glm::u16vec2>(glm::round(glm::round(glm::xy(screenPosition * 10.0f)) / 10.0f));
	}

private:
	uint16_t _frameNumber = 0;
	openblack::Camera& _camera;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#pragma GCC diagnostic pop
#pragma clang diagnostic pop
