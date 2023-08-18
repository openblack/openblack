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
#include <Input/GameActionMapInterface.h>
#include <Windowing/WindowingInterface.h>

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

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#pragma GCC diagnostic pop
#pragma clang diagnostic pop
