/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <filesystem>
#include <string>

#include <entt/locator/locator.hpp>

namespace openblack
{
struct EngineConfig;
class Camera;
class EventManager;
class LandIslandInterface;
class Profiler;
class RandomNumberManagerInterface;
class TempleInteriorInterface;

namespace v120
{
struct InfoConstants;
}
using InfoConstants = v120::InfoConstants;

namespace audio
{
class AudioManagerInterface;
}

namespace debug::gui
{
class DebugGuiInterface;
}

namespace filesystem
{
class FileSystemInterface;
}

namespace graphics
{
class RendererInterface;
}

namespace input
{
class GameActionInterface;
}

namespace resources
{
class ResourcesInterface;
}

namespace windowing
{
enum class DisplayMode : std::uint8_t;
class WindowingInterface;
} // namespace windowing

namespace ecs
{
class Registry;
class MapInterface;
} // namespace ecs

namespace ecs::systems
{
class RenderingSystemInterface;
class DynamicsSystemInterface;
class CameraBookmarkSystemInterface;
class LivingActionSystemInterface;
class TownSystemInterface;
class PathfindingSystemInterface;
class PlayerSystemInterface;
} // namespace ecs::systems

void InitializeWindow(const std::string& title, int width, int height, windowing::DisplayMode displayMode, uint32_t extraFlags);
bool InitializeGame(uint8_t rendererType, bool vsync) noexcept;
void InitializeLevel(const std::filesystem::path& path);

struct Locator
{
	using config = entt::locator<EngineConfig>;
	using infoConstants = entt::locator<const InfoConstants>;
	using profiler = entt::locator<Profiler>;
	using events = entt::locator<EventManager>;
	using windowing = entt::locator<windowing::WindowingInterface>;
	using debugGui = entt::locator<debug::gui::DebugGuiInterface>;
	using filesystem = entt::locator<filesystem::FileSystemInterface>;
	using resources = entt::locator<resources::ResourcesInterface>;
	using rng = entt::locator<RandomNumberManagerInterface>;
	using terrainSystem = entt::locator<LandIslandInterface>;
	using audio = entt::locator<audio::AudioManagerInterface>;
	using camera = entt::locator<Camera>;
	using gameActionSystem = entt::locator<input::GameActionInterface>;
	using rendereringSystem = entt::locator<ecs::systems::RenderingSystemInterface>;
	using rendererInterface = entt::locator<graphics::RendererInterface>;
	using dynamicsSystem = entt::locator<ecs::systems::DynamicsSystemInterface>;
	using cameraBookmarkSystem = entt::locator<ecs::systems::CameraBookmarkSystemInterface>;
	using livingActionSystem = entt::locator<ecs::systems::LivingActionSystemInterface>;
	using townSystem = entt::locator<ecs::systems::TownSystemInterface>;
	using pathfindingSystem = entt::locator<ecs::systems::PathfindingSystemInterface>;
	using entitiesRegistry = entt::locator<ecs::Registry>;
	using entitiesMap = entt::locator<ecs::MapInterface>;
	using playerSystem = entt::locator<ecs::systems::PlayerSystemInterface>;
	using temple = entt::locator<TempleInteriorInterface>;
};
} // namespace openblack
