/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <optional>

#include <3D/LandIslandInterface.h>
#include <3D/LandLine.h>
#include <Camera/Camera.h>
#include <ECS/Systems/PickingSystemInterface.h>
#include <Input/GameActionMapInterface.h>
#include <Locator.h>
#include <Windowing/WindowingInterface.h>
#include <glm/gtx/vec_swizzle.hpp>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4716)
#else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreturn-type"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
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
	[[nodiscard]] const openblack::lnd::LNDCell* FindCell(const glm::u16vec2&) const final { return nullptr; }
	void DumpTextures() const final { assert(false); }
	void DumpMaps() const final { assert(false); }
	[[nodiscard]] std::vector<openblack::LandBlock>& GetBlocks() final { assert(false); }
	[[nodiscard]] const std::vector<openblack::LandBlock>& GetBlocks() const final { assert(false); }
	[[nodiscard]] const std::vector<openblack::lnd::LNDCountry>& GetCountries() const final { assert(false); }
	[[nodiscard]] const openblack::graphics::Texture2D& GetHeightMap() const final { assert(false); }
	[[nodiscard]] const openblack::graphics::Texture2D& GetLuminosityMap() const final { assert(false); }
	[[nodiscard]] const openblack::graphics::Texture2D& GetCellColourMap() const final { assert(false); }
	[[nodiscard]] const openblack::graphics::Texture2D& GetBlockTextures() const final { assert(false); }
	[[nodiscard]] const openblack::graphics::FrameBuffer& GetFootprintFramebuffer() const final { assert(false); }
	[[nodiscard]] const openblack::graphics::FrameBuffer& GetLandAlphaFramebuffer() const final { assert(false); }
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
	[[nodiscard]] glm::ivec2 GetMouseDelta() const override { return {}; }
	[[nodiscard]] float GetMouseWheelDelta() const override { return 0.0f; }
	[[nodiscard]] std::array<std::optional<glm::vec3>, 2> GetHandPositions() const override { return {}; }
	void Frame() final {}
	void ProcessEvent(const SDL_Event& event) final {}

	uint32_t frameNumber = 0;
};

// The land under the cursor, as recorded from the game for each scenario: the land itself has no cells. The recordings
// stand for the land under each pixel, sea or not and already within reach of the map's middle, so `withSea` and the reach
// aren't looked at here.
class MockPickingSystem: public openblack::ecs::systems::PickingSystemInterface
{
public:
	~MockPickingSystem() override = default;

	/// The recorded land under a pixel, across x and z
	[[nodiscard]] virtual std::optional<glm::vec2> LandAtPixel(glm::u16vec2 screenCoord) const = 0;

	[[nodiscard]] std::optional<glm::vec3> LandAlong(glm::vec3 from, glm::vec3 to) const override
	{
		const auto hit = openblack::land_line::LandAlong(from, to, NoCells);
		return hit.has_value() ? std::optional(glm::vec3(hit->x, 0.0f, hit->y)) : std::nullopt;
	}
	[[nodiscard]] std::optional<glm::vec3> LandOrSeaAlong(glm::vec3 from, glm::vec3 to, glm::vec3 camera) const override
	{
		const auto hit = openblack::land_line::LandOrSeaAlong(from, to, camera, NoCells);
		return hit.has_value() ? std::optional(glm::vec3(hit->x, 0.0f, hit->y)) : std::nullopt;
	}
	[[nodiscard]] std::optional<glm::vec3> LandUnderPixel(glm::vec3 camera, glm::vec3 nearPoint,
	                                                      [[maybe_unused]] bool withSea) const override
	{
		const auto& terrain = openblack::Locator::terrainSystem::value();
		// The pixel the line through the near plane's point shows
		const auto screenCoord = GetWindowCoordinates(camera + (nearPoint - camera) * 100.0f);
		if (!screenCoord.has_value())
		{
			return std::nullopt;
		}
		const auto hit = LandAtPixel(*screenCoord);
		if (!hit.has_value())
		{
			return std::nullopt;
		}
		return glm::vec3(hit->x, terrain.GetHeightAt(*hit), hit->y);
	}
	void PickUnderCursor(const Frame& /*unused*/) override {}
	[[nodiscard]] const Pick& GetPick() const override { return _pick; }
	[[nodiscard]] std::optional<openblack::screen_pick::MeshHit> FeelModel(entt::entity /*unused*/, glm::vec3 /*unused*/,
	                                                                       glm::vec3 /*unused*/) const override
	{
		return std::nullopt;
	}

	[[nodiscard]] std::optional<glm::u16vec2> GetWindowCoordinates(const glm::vec3& position) const
	{
		const auto size = glm::vec2(openblack::Locator::windowing::value().GetSize());
		const auto clip =
		    camera->GetViewProjectionMatrix(openblack::Camera::Projection::Normal, openblack::Camera::Interpolation::Target) *
		    glm::vec4(position, 1.0f);
		if (clip.w <= 0.0f)
		{
			return std::nullopt;
		}
		// As the window has it, from the bottom up, then turned to the screen's rows from the top
		auto screenPosition = glm::vec2((clip.x / clip.w + 1.0f) * 0.5f * size.x, (clip.y / clip.w + 1.0f) * 0.5f * size.y);
		if (screenPosition.x < 0.0f || screenPosition.y < 0.0f || glm::round(screenPosition.x) > size.x ||
		    glm::round(screenPosition.y) > size.y)
		{
			return std::nullopt;
		}
		screenPosition.y = size.y - screenPosition.y;
		// Move point because of precision loss in project/deproject
		return static_cast<glm::u16vec2>(glm::round(glm::round(screenPosition * 10.0f) / 10.0f));
	}

	uint16_t frameNumber = 0;
	const openblack::Camera* camera;

private:
	static std::optional<openblack::land_line::CellHeights> NoCells(int32_t /*unused*/, int32_t /*unused*/)
	{
		return std::nullopt;
	}
	Pick _pick;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#pragma clang diagnostic pop
#endif
