/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "ParticleSystem.h"

#include <algorithm>
#include <chrono>

#include <EnumHeader.h>
#include <LNDFile.h>
#include <ParticleFile.h>
#include <StackedBitmap.h>
#include <entt/core/hashed_string.hpp>
#include <fmt/format.h>
#include <glm/geometric.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/spdlog.h>

#include "3D/AllMeshes.h"
#include "3D/CameraPath.h"
#include "3D/L3DAnim.h"
#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "Audio/AudioManagerInterface.h"
#include "Camera/Camera.h"
#include "Common/GameRandom.h"
#include "Common/StringUtils.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/AudioEmitter.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Player.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/CameraPathSystemInterface.h"
#include "ECS/Systems/CreatureHandSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Particles/ParticleClassRegistry.h"
#include "Particles/ParticleSoundRelease.h"
#include "Particles/ParticleTypes.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;

namespace
{
/// Where the particle files are, under the game's data
std::filesystem::path ParticleFileDirectory()
{
	return Locator::filesystem::value().GetPath<filesystem::Path::Data>() / "Spells" / "ZSpellFiles";
}

/// The compressed files' names end so
constexpr std::string_view k_CompressedSuffix = "_txt.zzz";

/// A game turn in seconds, what every effect not owned by a miracle steps by
constexpr float k_TurnSeconds = std::chrono::duration<float>(TimeSystemInterface::k_TurnDuration).count();

entt::id_type ParticleFileId(std::string_view name)
{
	return entt::hashed_string(fmt::format("particles/{}", name).c_str());
}

/// A path as the effect files give it, under the game's data folder, folders split by forward slashes
std::filesystem::path DataRelative(std::string_view path)
{
	std::string name(path);
	std::ranges::replace(name, '\\', '/');
	if (name.starts_with("./"))
	{
		name.erase(0, 2);
	}
	if (string_utils::LowerCase(name).starts_with("data/"))
	{
		name.erase(0, 5);
	}
	return name;
}

/// The spells' sound bank, whose effects the particles' sounds are played from
constexpr std::string_view k_SpellSoundBank = "spells.sad";
/// The key a particle's sound is looked up by besides its size, alignment, ground and action: the default object
constexpr int32_t k_SoundObject = 1;
/// The ground's kind where there is no land, and where the land is under water
constexpr int k_DeepWaterSurface = 6;
constexpr int k_ShallowWaterSurface = 7;
/// A ground of no sound of its own sounds hard
constexpr int k_HardSurface = 3;
constexpr int k_LastSurface = 8;
/// A looping sound is only kept going this close to the camera
constexpr float k_LoopCullDistance = 1200.0f;
/// Points along a way tested for the land rising across it
constexpr int k_LandBlockSamples = 16;
/// Any rain or snow makes a fireball steam
constexpr int k_SteamingRain = 0;
/// The land's cells are this many units across
constexpr float k_CellSize = 10.0f;
/// The weather's wind bytes are eighths of a metre a second
constexpr float k_WindByteSpeed = 0.125f;
/// Sound travels this many metres a second, which thunder takes to be heard
constexpr float k_SpeedOfSound = 347.0f;

/// The sheets the players' symbols are drawn with, whatever effect first shows one
constexpr std::array<std::string_view, 2> k_SymbolTextures {"S_SpriteSheet3", "ChooseSymbol"};
} // namespace

float GameParticleWorld::LandHeight(glm::vec2 xz) const
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(xz) : 0.0f;
}

uint32_t GameParticleWorld::PlayerColour(int player) const
{
	return ecs::components::Player::k_Colours.at(static_cast<size_t>(player) &
	                                             (ecs::components::Player::k_Colours.size() - 1)) &
	       0xFFFFFFu;
}

glm::vec3 GameParticleWorld::CameraRight() const
{
	return Locator::camera::has_value() ? Locator::camera::value().GetRight() : glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec3 GameParticleWorld::CameraUp() const
{
	return Locator::camera::has_value() ? Locator::camera::value().GetUp() : glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 GameParticleWorld::CameraPosition() const
{
	return Locator::camera::has_value() ? Locator::camera::value().GetOrigin() : glm::vec3(0.0f);
}

void GameParticleWorld::PlayListenerSound(uint32_t inGameSample)
{
	if (Locator::audio::has_value())
	{
		const auto id = entt::hashed_string(fmt::format("InGame.sad/{}", inGameSample).c_str()).value();
		// Played once: not started again while the same sample still plays
		Locator::audio::value().PlaySoundEffect(id, std::nullopt);
	}
}

std::optional<particles::ParticleWorldInterface::TargetInfo> GameParticleWorld::Target(entt::entity target, bool centre) const
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return std::nullopt;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(target))
	{
		return std::nullopt;
	}
	const auto* transform = registry.TryGet<ecs::components::Transform>(target);
	if (transform == nullptr)
	{
		return std::nullopt;
	}
	// Its size from its model's box, scaled as it stands
	TargetInfo info {.position = transform->position, .radius = 1.0f, .height = 1.0f, .scale = transform->scale.x};
	if (const auto* mesh = registry.TryGet<ecs::components::Mesh>(target);
	    mesh != nullptr && Locator::resources::has_value() && Locator::resources::value().GetMeshes().Contains(mesh->id))
	{
		const auto box = Locator::resources::value().GetMeshes().Handle(mesh->id)->GetBoundingBox();
		const auto size = box.Size() * transform->scale;
		info.radius = std::max(size.x, size.z) * 0.5f;
		info.height = size.y;
	}
	if (centre)
	{
		info.position.y += info.height * 0.5f;
	}
	return info;
}

bool GameParticleWorld::IsTargetHeld(entt::entity target) const
{
	return Locator::creatureHandSystem::has_value() && Locator::creatureHandSystem::value().GetCreature() == target;
}

void GameParticleWorld::ClaimTarget(entt::entity target, bool claimed)
{
	if (claimed)
	{
		_claimed.insert(target);
	}
	else
	{
		_claimed.erase(target);
	}
}

void GameParticleWorld::StartSound(const particles::Effect& /*effect*/,
                                   const std::shared_ptr<particles::ParticleSoundLink>& sound)
{
	auto& playing = _sounds.emplace_back(PlayingSound {.link = sound});
	if (Locator::entitiesRegistry::has_value())
	{
		playing.owner = Locator::entitiesRegistry::value().Create();
	}
	// Thunder is heard once it has come from the cloud
	if (sound->sound.travelsAtSoundSpeed && Locator::camera::has_value())
	{
		playing.wait = glm::distance(Locator::camera::value().GetOrigin(), sound->position) / k_SpeedOfSound;
		if (playing.wait > 0.0f)
		{
			return;
		}
	}
	Play(playing);
}

void GameParticleWorld::Play(PlayingSound& sound) const
{
	if (!Locator::audio::has_value() || !Locator::camera::has_value())
	{
		return;
	}
	if (!_soundActions.has_value())
	{
		_soundActions.emplace();
		if (Locator::filesystem::has_value())
		{
			auto& fileSystem = Locator::filesystem::value();
			const auto path = fileSystem.GetPath<filesystem::Path::Data>() / "SoundAction.h";
			if (fileSystem.Exists(path))
			{
				const auto bytes = fileSystem.ReadAll(path);
				*_soundActions =
				    psys::ParseEnumHeader(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
			}
		}
	}
	const auto action = _soundActions->find(sound.link->sound.action.sound);
	if (action == _soundActions->end())
	{
		return;
	}
	const std::array<int32_t, 5> keys {sound.link->sound.size, sound.link->sound.alignment, k_SoundObject,
	                                   sound.link->sound.surface, action->second};
	const auto result =
	    Locator::audio::value().PlayAnimEffect(std::string(k_SpellSoundBank), keys, sound.owner, sound.link->position);
	if (result.emitter != entt::null)
	{
		sound.emitter = result.emitter;
	}
}

void GameParticleWorld::ProcessSounds()
{
	auto* registry = Locator::entitiesRegistry::has_value() ? &Locator::entitiesRegistry::value() : nullptr;
	auto* audio = Locator::audio::has_value() ? &Locator::audio::value() : nullptr;
	const auto playing = [&](const PlayingSound& sound) {
		return audio != nullptr && sound.emitter != entt::null && audio->EmitterExists(sound.emitter) &&
		       audio->GetStatus(sound.emitter) != audio::AudioStatus::Stopped;
	};
	std::erase_if(_sounds, [&](PlayingSound& sound) {
		auto& link = *sound.link;
		// On its way to the listener: it plays when it gets there, wherever its particle has gone
		if (sound.wait > 0.0f)
		{
			sound.wait -= k_TurnSeconds;
			if (sound.wait > 0.0f)
			{
				return false;
			}
			Play(sound);
		}
		if (link.atom != nullptr)
		{
			if (link.atom->drawn)
			{
				link.position = link.atom->current.position;
				if (link.sound.onLand)
				{
					link.position.y = LandHeight({link.position.x, link.position.z});
				}
			}
			if (registry != nullptr && sound.emitter != entt::null && registry->Valid(sound.emitter))
			{
				if (auto* emitter = registry->TryGet<ecs::components::AudioEmitter>(sound.emitter))
				{
					emitter->position = link.position;
				}
			}
			// A loop plays on while its particle keeps it, near enough to be heard
			const bool nearby = Locator::camera::has_value() &&
			                    glm::distance(Locator::camera::value().GetOrigin(), link.position) < k_LoopCullDistance;
			if (link.sound.action.looping && nearby && !playing(sound))
			{
				Play(sound);
			}
			return false;
		}
		if (!sound.letGo)
		{
			LetGo(sound);
		}
		else
		{
			Fade(sound);
		}
		if (playing(sound))
		{
			return false;
		}
		Forget(sound);
		return true;
	});
}

void GameParticleWorld::LetGo(PlayingSound& sound)
{
	sound.letGo = true;
	if (!Locator::audio::has_value())
	{
		return;
	}
	auto& audio = Locator::audio::value();
	if (sound.emitter == entt::null || !audio.EmitterExists(sound.emitter))
	{
		return;
	}
	const auto& action = sound.link->sound.action;
	SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Particles: {} let go of", action.sound);
	// The particle's fade step when it stopped the sound, otherwise the one its sound rule gave it
	sound.fade = particles::LetGoOf(action.softRelease, audio.IsEmitterLooping(sound.emitter), sound.link->fadeStep);
	if (sound.fade.stopNow)
	{
		audio.StopEmitter(sound.emitter);
		return;
	}
	// The first step quieter comes on the turn it is let go, before its loop is released
	Fade(sound);
	if (sound.fade.releaseLoop && audio.EmitterExists(sound.emitter))
	{
		audio.ReleaseEmitterLoop(sound.emitter);
	}
}

void GameParticleWorld::Fade(PlayingSound& sound)
{
	if (sound.fade.fadeStep <= 0 || !Locator::audio::has_value())
	{
		return;
	}
	auto& audio = Locator::audio::value();
	if (sound.emitter == entt::null || !audio.EmitterExists(sound.emitter))
	{
		return;
	}
	// Quieter each turn, and stopped once silent when it would otherwise loop or ring on
	const auto volume = particles::FadedVolume(audio.GetEmitterVolume(sound.emitter), sound.fade.fadeStep);
	audio.SetEmitterVolume(sound.emitter, volume);
	if (volume == 0 && sound.fade.stopWhenSilent)
	{
		audio.StopEmitter(sound.emitter);
	}
}

void GameParticleWorld::Forget(PlayingSound& sound)
{
	if (sound.owner != entt::null && Locator::entitiesRegistry::has_value())
	{
		auto& registry = Locator::entitiesRegistry::value();
		if (registry.Valid(sound.owner))
		{
			registry.Destroy(sound.owner);
		}
	}
	sound.owner = entt::null;
}

int GameParticleWorld::SurfaceAt(glm::vec3 point) const
{
	if (!Locator::terrainSystem::has_value() || point.x < 0.0f || point.z < 0.0f)
	{
		return k_DeepWaterSurface;
	}
	const auto& land = Locator::terrainSystem::value();
	const auto* cell = land.FindCell(glm::u16vec2(glm::floor(glm::vec2(point.x, point.z) / k_CellSize)));
	if (cell == nullptr)
	{
		return k_DeepWaterSurface;
	}
	if (cell->properties.hasWater != 0)
	{
		return k_ShallowWaterSurface;
	}
	// The second of the two materials the cell's country blends at its altitude
	const auto& countries = land.GetCountries();
	const auto types = land.GetMaterialTypes();
	int surface = k_HardSurface;
	if (cell->properties.country < countries.size() && Locator::infoConstants::has_value())
	{
		const auto index = countries[cell->properties.country].materials.at(cell->altitude).indices[1];
		const auto& materials = Locator::infoConstants::value().terrainMaterial;
		if (index < types.size() && types[index] < materials.size())
		{
			surface = static_cast<int>(materials.at(types[index]).surfaceSound);
		}
	}
	return surface >= 1 && surface <= k_LastSurface ? surface : k_HardSurface;
}

int GameParticleWorld::SoundAlignment(int player) const
{
	if (player < 0 || !Locator::alignmentSystem::has_value())
	{
		return 2;
	}
	// The alignment in seven steps from devilish to angelic, then evil, middling or good
	const float alignment = Locator::alignmentSystem::value().GetPlayerAlignment(static_cast<PlayerNames>(player));
	const auto discrete = static_cast<int>(std::min((alignment + 1.0f) * 0.5f * 6.9999995f, 6.0f));
	return discrete <= 1 ? 1 : discrete <= 4 ? 2 : 3;
}

bool GameParticleWorld::IsWater(glm::vec3 point) const
{
	if (!Locator::terrainSystem::has_value() || point.x < 0.0f || point.z < 0.0f)
	{
		return true;
	}
	const auto cell = glm::u16vec2(glm::floor(glm::vec2(point.x, point.z) / k_CellSize));
	const auto* landCell = Locator::terrainSystem::value().FindCell(cell);
	return landCell == nullptr || landCell->properties.hasWater != 0;
}

glm::vec3 GameParticleWorld::LandNormal(glm::vec2 xz) const
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetNormalAt(xz) : glm::vec3(0.0f, 1.0f, 0.0f);
}

bool GameParticleWorld::IsRainingAt(glm::vec3 point) const
{
	if (!Locator::weatherSystem::has_value())
	{
		return false;
	}
	const auto weather = Locator::weatherSystem::value().GetWeather(point);
	return weather.rain > k_SteamingRain || weather.snow > k_SteamingRain;
}

glm::vec3 GameParticleWorld::WindAt(glm::vec3 point) const
{
	if (!Locator::weatherSystem::has_value())
	{
		return glm::vec3(0.0f);
	}
	const auto weather = Locator::weatherSystem::value().GetWeather(point);
	return glm::vec3(static_cast<float>(weather.windX), 0.0f, static_cast<float>(weather.windZ)) * k_WindByteSpeed;
}

glm::vec3 GameParticleWorld::SmoothWindAt(glm::vec3 point) const
{
	if (!Locator::weatherSystem::has_value())
	{
		return glm::vec3(0.0f);
	}
	const auto weather = Locator::weatherSystem::value().GetWeatherSmooth(point);
	return glm::vec3(static_cast<float>(weather.windX), 0.0f, static_cast<float>(weather.windZ)) * k_WindByteSpeed;
}

uint32_t GameParticleWorld::AddRainStorm(const particles::storm::RainStorm& storm)
{
	if (!Locator::weatherSystem::has_value() || !Locator::entitiesRegistry::has_value())
	{
		return 0;
	}
	const auto entity = Locator::weatherSystem::value().AddMiracleStorm({
	    .centre = storm.centre,
	    .innerRadius = storm.innerRadius,
	    .outerRadius = storm.outerRadius,
	    .fadeSeconds = storm.fadeInSeconds,
	    .cloudHeight = storm.cloudHeight,
	    .effect = {.temperature = storm.temperature,
	               .rain = storm.rain,
	               .snow = storm.snow,
	               .overcast = storm.overcast,
	               .windX = storm.windX,
	               .windZ = storm.windZ},
	});
	const auto id = _nextRainStorm++;
	_rainStorms.insert_or_assign(id, entity);
	return id;
}

bool GameParticleWorld::MoveRainStorm(uint32_t storm, glm::vec3 centre)
{
	const auto found = _rainStorms.find(storm);
	if (found == _rainStorms.end() || !Locator::weatherSystem::has_value())
	{
		return false;
	}
	if (Locator::weatherSystem::value().MoveMiracleStorm(found->second, centre))
	{
		return true;
	}
	// Ended by a script: the weather lets it go by itself
	_rainStorms.erase(found);
	return false;
}

void GameParticleWorld::RemoveRainStorm(uint32_t storm)
{
	const auto found = _rainStorms.find(storm);
	if (found == _rainStorms.end())
	{
		return;
	}
	if (Locator::weatherSystem::has_value() && Locator::entitiesRegistry::has_value())
	{
		Locator::weatherSystem::value().RemoveMiracleStorm(found->second);
	}
	_rainStorms.erase(found);
}

bool GameParticleWorld::LandBlocks(glm::vec3 from, glm::vec3 to) const
{
	if (!Locator::terrainSystem::has_value())
	{
		return false;
	}
	const auto& land = Locator::terrainSystem::value();
	// The ends themselves may touch the land
	for (int i = 1; i < k_LandBlockSamples; ++i)
	{
		const float t = static_cast<float>(i) / static_cast<float>(k_LandBlockSamples);
		const auto p = from + (to - from) * t;
		if (land.GetHeightAt({p.x, p.z}) > p.y)
		{
			return true;
		}
	}
	return false;
}

void GameParticleWorld::AddShield(const std::shared_ptr<particles::ShieldSphere>& shield)
{
	std::erase_if(_shields, [](const auto& weak) { return weak.expired(); });
	_shields.push_back(shield);
}

std::shared_ptr<particles::ShieldSphere> GameParticleWorld::FindShield(glm::vec3 point, float margin) const
{
	for (const auto& weak : _shields)
	{
		if (auto shield = weak.lock(); shield && glm::distance(point, shield->centre) < shield->radius + margin)
		{
			return shield;
		}
	}
	return nullptr;
}

std::shared_ptr<particles::ShieldSphere> GameParticleWorld::ShieldOf(const particles::Effect& effect) const
{
	for (const auto& weak : _shields)
	{
		if (auto shield = weak.lock(); shield && shield->owner == &effect)
		{
			return shield;
		}
	}
	return nullptr;
}

std::vector<glm::vec3> GameParticleWorld::TargetExtraPoints(entt::entity target) const
{
	std::vector<glm::vec3> points;
	if (!Locator::entitiesRegistry::has_value() || !Locator::resources::has_value())
	{
		return points;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(target))
	{
		return points;
	}
	const auto* transform = registry.TryGet<const ecs::components::Transform>(target);
	std::optional<entt::id_type> meshId;
	if (const auto* mesh = registry.TryGet<const ecs::components::Mesh>(target))
	{
		meshId = mesh->id;
	}
	else if (const auto* dome = registry.TryGet<const ecs::components::ShieldDome>(target))
	{
		meshId = dome->mesh;
	}
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (transform == nullptr || !meshId.has_value() || !meshes.Contains(*meshId))
	{
		return points;
	}
	const glm::mat4 model = glm::translate(glm::mat4(1.0f), transform->position) * glm::mat4(transform->rotation) *
	                        glm::scale(glm::mat4(1.0f), transform->scale);
	for (const auto& extra : meshes.Handle(*meshId)->GetExtraMetrics())
	{
		points.emplace_back(model * extra[3]);
	}
	return points;
}

std::optional<glm::vec3> GameParticleWorld::ObjectPosition(entt::entity object) const
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return std::nullopt;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* transform = registry.Valid(object) ? registry.TryGet<const ecs::components::Transform>(object) : nullptr;
	return transform != nullptr ? std::optional(transform->position) : std::nullopt;
}

GameParticleWorld::SurfacePoint GameParticleWorld::RandomSurfacePoint(entt::entity object, particles::Effect& effect) const
{
	if (!Locator::entitiesRegistry::has_value() || !Locator::resources::has_value())
	{
		return {};
	}
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* transform = registry.Valid(object) ? registry.TryGet<const ecs::components::Transform>(object) : nullptr;
	if (transform == nullptr)
	{
		return {};
	}
	const auto* mesh = registry.TryGet<const ecs::components::Mesh>(object);
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (mesh == nullptr || !meshes.Contains(mesh->id))
	{
		return {.kind = SurfacePoint::Kind::NoModel};
	}
	// A submesh, one of its primitives and one of its triangles, each as likely as the others whatever their size
	const auto& surfaces = meshes.Handle(mesh->id)->GetSurfaces();
	if (surfaces.empty())
	{
		return {.kind = SurfacePoint::Kind::NoModel};
	}
	const auto& surface = surfaces.at(static_cast<size_t>(effect.Rand(static_cast<int32_t>(surfaces.size()))));
	if (surface.primitives.empty())
	{
		return {.kind = SurfacePoint::Kind::NoModel};
	}
	const auto& primitive =
	    surface.primitives.at(static_cast<size_t>(effect.Rand(static_cast<int32_t>(surface.primitives.size()))));
	const auto triangle = static_cast<uint32_t>(effect.Rand(static_cast<int32_t>(primitive.numTriangles)));
	std::array<glm::vec3, 3> corners {};
	for (uint32_t c = 0; c < 3; ++c)
	{
		const auto index = primitive.indexBase + (triangle * 3) + c;
		if (index >= surface.indices.size())
		{
			return {.kind = SurfacePoint::Kind::NoModel};
		}
		const auto vertex = primitive.vertexBase + surface.indices.at(index);
		if (vertex >= surface.positions.size())
		{
			return {.kind = SurfacePoint::Kind::NoModel};
		}
		corners.at(c) = surface.positions.at(vertex);
	}
	// Evenly within the triangle: a point of the parallelogram on two of its sides, folded back into it
	float a = effect.Random(1.0f);
	float b = effect.Random(1.0f);
	if (a + b > 1.0f)
	{
		a = 1.0f - a;
		b = 1.0f - b;
	}
	const auto local = corners[0] + (corners[1] - corners[0]) * a + (corners[2] - corners[0]) * b;
	const glm::mat4 model = glm::translate(glm::mat4(1.0f), transform->position) * glm::mat4(transform->rotation) *
	                        glm::scale(glm::mat4(1.0f), transform->scale);
	return {.kind = SurfacePoint::Kind::Point, .position = glm::vec3(model * glm::vec4(local, 1.0f))};
}

float GameParticleWorld::PlayerAlignment(int player) const
{
	if (!Locator::alignmentSystem::has_value() || player < 0 || player >= static_cast<int>(PlayerNames::_COUNT))
	{
		return 0.0f;
	}
	return Locator::alignmentSystem::value().GetPlayerAlignment(static_cast<PlayerNames>(player));
}

void GameParticleWorld::FollowCameraPath(std::string_view file, std::string_view animation, const glm::mat4& placement,
                                         float pauseSeconds, float speedUp, entt::entity spell)
{
	if (!Locator::filesystem::has_value() || !Locator::cameraPathSystem::has_value())
	{
		return;
	}
	auto& fileSystem = Locator::filesystem::value();
	auto path = std::make_unique<CameraPath>(std::string(file));
	try
	{
		if (!path->LoadFromFile(fileSystem.FindPath(fileSystem.GetPath<filesystem::Path::Data>() / DataRelative(file))))
		{
			return;
		}
	}
	catch (const std::exception& error)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "Particles: cannot load the camera path {}: {}", file, error.what());
		return;
	}
	// The path lasts as long as one play of its animation
	L3DAnim anim;
	if (!anim.LoadFromFile(fileSystem.FindPath(fileSystem.GetPath<filesystem::Path::Data>() / DataRelative(animation))))
	{
		return;
	}
	Locator::cameraPathSystem::value().FollowPlaced(std::move(path), placement, pauseSeconds, speedUp,
	                                                static_cast<float>(anim.GetPlayTime()), spell);
}

void GameParticleWorld::Reset()
{
	_claimed.clear();
	_arcsWaiting.clear();
	_beliefSprites.clear();
	_gestureTrails.clear();
	_lightSheets.clear();
	SetHandGlow(0);
	_arcsWanted = false;
	_bolts.clear();
	// Every sound stops with the land, and its owner goes
	for (auto& sound : _sounds)
	{
		if (Locator::audio::has_value() && sound.emitter != entt::null && Locator::audio::value().EmitterExists(sound.emitter))
		{
			Locator::audio::value().StopEmitter(sound.emitter);
		}
		Forget(sound);
	}
	_sounds.clear();
	_shields.clear();
	// The effects' storms went with the land
	_rainStorms.clear();
	_objectEffects.Reset();
}

std::optional<entt::id_type> GameCreatorResources::MeshByName(std::string_view name)
{
	if (!_meshNames.has_value())
	{
		_meshNames.emplace();
		if (Locator::filesystem::has_value())
		{
			auto& fileSystem = Locator::filesystem::value();
			const auto path = fileSystem.GetPath<filesystem::Path::Data>() / "AllMeshes.h";
			if (fileSystem.Exists(path))
			{
				const auto bytes = fileSystem.ReadAll(path);
				*_meshNames =
				    psys::ParseEnumHeader(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
			}
		}
	}
	const auto found = _meshNames->find(name);
	if (found == _meshNames->end() || found->second <= 0 || found->second >= static_cast<int32_t>(MeshId::_COUNT))
	{
		return std::nullopt;
	}
	return resources::HashIdentifier(static_cast<MeshId>(found->second));
}

std::optional<entt::id_type> GameCreatorResources::MeshByFile(std::string_view path)
{
	if (!Locator::resources::has_value() || !Locator::filesystem::has_value())
	{
		return std::nullopt;
	}
	const auto relative = DataRelative(path);
	const auto id = entt::hashed_string(("particles/" + string_utils::LowerCase(relative.generic_string())).c_str()).value();
	auto& meshes = Locator::resources::value().GetMeshes();
	if (meshes.Contains(id))
	{
		return id;
	}
	auto& fileSystem = Locator::filesystem::value();
	try
	{
		meshes.Load(id, resources::L3DLoader::FromDiskTag {},
		            fileSystem.FindPath(fileSystem.GetPath<filesystem::Path::Data>() / relative));
	}
	catch (const std::exception& error)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "Particles: cannot load the model {}: {}", path, error.what());
		return std::nullopt;
	}
	return id;
}

std::optional<entt::id_type> GameCreatorResources::LightMap(std::string_view path, int pitch, int channels, int framesInFile,
                                                            int framesInUse)
{
	if (!Locator::resources::has_value() || !Locator::filesystem::has_value())
	{
		return std::nullopt;
	}
	const auto relative = DataRelative(path);
	const auto key = fmt::format("particles/{}/{}/{}/{}/{}", string_utils::LowerCase(relative.generic_string()), pitch,
	                             channels, framesInFile, framesInUse);
	const auto id = entt::hashed_string(key.c_str()).value();
	auto& bitmaps = Locator::resources::value().GetParticleBitmaps();
	if (bitmaps.Contains(id))
	{
		return id;
	}
	auto& fileSystem = Locator::filesystem::value();
	try
	{
		bitmaps.Load(id, resources::ParticleBitmapLoader::FromDiskTag {},
		             fileSystem.FindPath(fileSystem.GetPath<filesystem::Path::Data>() / relative),
		             resources::ParticleBitmapLoader::Layout {
		                 .pitch = pitch, .channels = channels, .framesInFile = framesInFile, .framesInUse = framesInUse});
	}
	catch (const std::exception& error)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "Particles: cannot load the light map {}: {}", path, error.what());
		return std::nullopt;
	}
	return id;
}

ParticleSystem::ParticleSystem()
    : _classes(particles::ParticleClassRegistry::WithAllClasses(&_resources))
{
}

ParticleSystem::~ParticleSystem() = default;

std::deque<ParticleSystem::Running>::iterator ParticleSystem::FindRunning(EffectId id)
{
	return std::ranges::find(_effects, id, &Running::id);
}

std::deque<ParticleSystem::Running>::const_iterator ParticleSystem::FindRunning(EffectId id) const
{
	return std::ranges::find(_effects, id, &Running::id);
}

ParticleSystemInterface::EffectId ParticleSystem::Start(std::string_view file, glm::vec3 origin, float magnitude, bool synced)
{
	if (file.empty() || !Locator::resources::has_value() || !Locator::gameRandom::has_value())
	{
		return k_NoEffect;
	}
	auto& files = Locator::resources::value().GetParticleFiles();
	const auto id = ParticleFileId(file);
	if (!files.Contains(id))
	{
		try
		{
			files.Load(id, resources::ParticleFileLoader::FromDiskTag {}, ParticleFileDirectory(), std::string(file));
		}
		catch (const std::exception& error)
		{
			SPDLOG_LOGGER_WARN(spdlog::get("game"), "Particles: cannot load {}: {}", file, error.what());
			return k_NoEffect;
		}
	}
	const std::shared_ptr<const psys::ParticleFile> data = files.Handle(id).handle();
	if (!data)
	{
		return k_NoEffect;
	}
	auto effect = std::make_unique<particles::Effect>(
	    data, particles::EffectServices {_classes, _world, Locator::gameRandom::value(), _noise}, origin, magnitude, synced);
	for (const auto& className : effect->UnportedClasses())
	{
		if (_reportedUnported.insert(className).second)
		{
			SPDLOG_LOGGER_INFO(spdlog::get("game"), "Particles: {} (first in {}) is not run yet", className, file);
		}
	}
	ResolveTextures(*effect);
	effect->StepTwiceFirstTime();
	const auto effectId = _nextId++;
	_effects.push_front({.id = effectId, .file = std::string(file), .effect = std::move(effect)});
	return effectId;
}

ParticleSystemInterface::EffectId ParticleSystem::Start(ParticleType type, glm::vec3 origin, float magnitude, bool synced)
{
	return Start(particles::ParticleTypeFile(type), origin, magnitude, synced);
}

ParticleSystemInterface::EffectId ParticleSystem::StartForSpell(ParticleType type, glm::vec3 origin, glm::vec3 direction,
                                                                float magnitude, particles::SpellSink& sink, bool synced)
{
	const auto id = Start(type, origin, magnitude, synced);
	if (const auto it = FindRunning(id); it != _effects.end())
	{
		it->ownedBySpell = true;
		it->effect->SetDirection(direction);
		it->effect->SetSink(&sink);
	}
	return id;
}

bool ParticleSystem::ProcessForSpell(EffectId id, const particles::ProcessInfo& info, float seconds)
{
	const auto it = FindRunning(id);
	if (it == _effects.end())
	{
		return false;
	}
	it->effect->SetProcessInfo(info);
	// Its step may start other effects, such as a blast's spot visuals, so it is found again after
	if (StepEffect(*it->effect, seconds))
	{
		_effects.erase(FindRunning(id));
		return false;
	}
	return true;
}

bool ParticleSystem::ProcessByFrame(EffectId id, float seconds)
{
	const auto it = FindRunning(id);
	if (it == _effects.end())
	{
		return false;
	}
	it->ownedBySpell = true;
	if (_paused)
	{
		return true;
	}
	if (StepEffect(*it->effect, seconds))
	{
		_effects.erase(FindRunning(id));
		return false;
	}
	return true;
}

ParticleSystemInterface::EffectId ParticleSystem::StartSpotVisual(SpotVisualType type, glm::vec3 position,
                                                                  std::optional<int> turns, entt::entity owner, float magnitude)
{
	if (!Locator::infoConstants::has_value())
	{
		return k_NoEffect;
	}
	const auto& spotVisuals = Locator::infoConstants::value().spotVisual;
	const auto index = static_cast<size_t>(type);
	if (index >= spotVisuals.size())
	{
		return k_NoEffect;
	}
	const auto& info = spotVisuals.at(index);
	// Spot visuals are the same on every machine
	const auto id = Start(info.particleType, position, magnitude, true);
	if (const auto it = FindRunning(id); it != _effects.end())
	{
		it->turnsLeft = turns.value_or(static_cast<int>(info.life));
		it->owner = owner;
		// Some act on their owner, which their rules take as a target
		if (info.targetOwnerObject == 1 && owner != entt::null)
		{
			it->effect->AddTarget(owner);
		}
		// Some are drawn all together at their origin rather than each sprite in its own place
		it->path = info.singleZSort == 1 ? particles::draw::DrawPath::Queued : particles::draw::DrawPath::Sorted;
	}
	return id;
}

void ParticleSystem::SetOrigin(EffectId id, glm::vec3 origin)
{
	if (const auto it = FindRunning(id); it != _effects.end())
	{
		it->effect->SetOrigin(origin);
	}
}

void ParticleSystem::SetPlayer(EffectId id, int player)
{
	if (const auto it = FindRunning(id); it != _effects.end())
	{
		it->effect->SetPlayer(player);
	}
}

void ParticleSystem::SetDrawPath(EffectId id, particles::draw::DrawPath path)
{
	if (const auto it = FindRunning(id); it != _effects.end())
	{
		it->path = path;
	}
}

void ParticleSystem::SetDrawOffset(EffectId id, glm::vec3 offset)
{
	if (const auto it = FindRunning(id); it != _effects.end())
	{
		it->drawOffset = offset;
	}
}

std::shared_ptr<particles::ShieldSphere> ParticleSystem::FindShield(glm::vec3 point, float margin) const
{
	return _world.FindShield(point, margin);
}

void ParticleSystem::AddTarget(EffectId id, entt::entity target)
{
	if (const auto it = FindRunning(id); it != _effects.end())
	{
		it->effect->AddTarget(target);
	}
}

void ParticleSystem::AddTargetPosition(EffectId id, glm::vec3 position)
{
	if (const auto it = FindRunning(id); it != _effects.end())
	{
		it->effect->AddTargetPosition(position);
	}
}

void ParticleSystem::CloseDown(EffectId id)
{
	if (const auto it = FindRunning(id); it != _effects.end())
	{
		it->effect->CloseDown();
	}
}

void ParticleSystem::Delete(EffectId id)
{
	if (const auto it = FindRunning(id); it != _effects.end())
	{
		_effects.erase(it);
	}
}

bool ParticleSystem::IsRunning(EffectId id) const
{
	return FindRunning(id) != _effects.end();
}

particles::Effect* ParticleSystem::Find(EffectId id)
{
	const auto it = FindRunning(id);
	return it != _effects.end() ? it->effect.get() : nullptr;
}

bool ParticleSystem::StepEffect(particles::Effect& effect, float seconds)
{
	if (_paused)
	{
		return false;
	}
	effect.Step(seconds);
	// Closing down ends at once an effect whose file says so
	return effect.Finished() || (effect.Closing() && effect.DeleteOnCloseDown());
}

void ParticleSystem::ProcessTurn()
{
	if (_paused)
	{
		return;
	}
	// The one effect that throws the pieces of everything the blasts break runs all the time, started again whenever
	// it ends
	if (!IsRunning(_explodeObject))
	{
		_explodeObject = Start(ParticleType::ExplodeObject, glm::vec3(0.0f), 1.0f, true);
	}
	KeepGestureTrails();
	const auto* registry = Locator::entitiesRegistry::has_value() ? &Locator::entitiesRegistry::value() : nullptr;
	// By id: a step may start other effects
	std::vector<EffectId> ids;
	ids.reserve(_effects.size());
	std::ranges::transform(_effects, std::back_inserter(ids), &Running::id);
	for (const auto id : ids)
	{
		const auto it = FindRunning(id);
		if (it == _effects.end() || it->ownedBySpell || it->everyFrame)
		{
			continue;
		}
		auto& effect = *it->effect;
		// A spot visual stays where it was made, unless moved on purpose; it ends when its owner goes. One that acts on
		// its owner follows it through its rules, which take the owner as their target.
		if (it->owner != entt::null && (registry == nullptr || !registry->Valid(it->owner)))
		{
			it->owner = entt::null;
			effect.CloseDown();
		}
		if (it->turnsLeft.has_value() && *it->turnsLeft >= 0 && --*it->turnsLeft <= 0)
		{
			effect.CloseDown();
		}
		if (StepEffect(effect, k_TurnSeconds))
		{
			_effects.erase(FindRunning(id));
		}
	}
	_world.ProcessSounds();
	_world.ProcessGlows();
	// The symbols of belief rise in an effect of their own too
	if (_beliefWanted &&
	    std::ranges::none_of(_effects, [](const Running& running) { return running.file == "SF_BeliefSprite"; }))
	{
		Start(ParticleType::BeliefSprite, glm::vec3(0.0f), 1.0f, true);
	}
	_beliefWanted = false;
	// The electric arcs over what lightning struck crawl in an effect of their own, kept running once wanted
	if (_world.TakeArcsWanted() &&
	    std::ranges::none_of(_effects, [](const Running& running) { return running.file == "SF_OnFire"; }))
	{
		Start(ParticleType::OnFire, glm::vec3(0.0f), 1.0f, true);
	}
}

void ParticleSystem::Reset()
{
	_effects.clear();
	_explodeObject = k_NoEffect;
	_gestureTrails = k_NoEffect;
	_gestureChain = k_NoEffect;
	_world.Reset();
}

void ParticleSystem::ResolveTextures(const particles::Effect& effect)
{
	if (_textureStems.empty() && Locator::filesystem::has_value())
	{
		auto& fileSystem = Locator::filesystem::value();
		fileSystem.Iterate(fileSystem.GetPath<filesystem::Path::Textures>(), false, [this](const std::filesystem::path& file) {
			if (string_utils::LowerCase(file.extension().string()) == ".raw")
			{
				const auto stem = file.stem().string();
				_textureStems.insert_or_assign(string_utils::LowerCase(stem), stem);
			}
		});
	}
	const auto resolve = [this](std::string_view texture) {
		if (texture.empty() || _textures.contains(texture))
		{
			return;
		}
		// The files don't always spell a sheet's name as it is on disk
		const auto found = _textureStems.find(string_utils::LowerCase(std::string(texture)));
		const auto stem = found != _textureStems.end() ? found->second : std::string(texture);
		_textures.insert_or_assign(std::string(texture),
		                           std::pair {entt::hashed_string(fmt::format("raw/{}", stem).c_str()).value(),
		                                      entt::hashed_string(fmt::format("raw/{}a", stem).c_str()).value()});
	};
	for (const auto& object : effect.GetFile().objects)
	{
		// A surface's sheet is named by the rule that draws it
		if (object.className == "ZR_SurfRevol")
		{
			resolve(particles::TextureBaseName(object.String("TextureFileName")));
		}
		const auto* creator = effect.FindCreator(object.name);
		if (creator == nullptr)
		{
			continue;
		}
		if (creator->kind == particles::Creator::Kind::Sprite || creator->kind == particles::Creator::Kind::Chain)
		{
			resolve(creator->texture);
		}
		else if (creator->kind == particles::Creator::Kind::Symbol)
		{
			std::ranges::for_each(k_SymbolTextures, resolve);
		}
	}
}

void ParticleSystem::CollectDrawFrame(float turnFraction, particles::draw::Frame& frame) const
{
	frame.Clear();
	const particles::draw::Sources sources {
	    .textures = [this](std::string_view texture) -> std::optional<std::pair<entt::id_type, entt::id_type>> {
		    const auto found = _textures.find(texture);
		    if (found == _textures.end())
		    {
			    return std::nullopt;
		    }
		    return found->second;
	    },
	    .playerColour = [this](int player) { return _world.PlayerColour(player); },
	    .random =
	        [](float range) {
		        return Locator::gameRandom::has_value() ? Locator::gameRandom::value().LocalFloatRand(range) : 0.0f;
	        },
	    .landHeight = [this](glm::vec2 xz) { return _world.LandHeight(xz); },
	};
	for (const auto& running : _effects)
	{
		_walk.Clear();
		running.effect->Walk(turnFraction, _walk);
		if (running.drawOffset != glm::vec3(0.0f))
		{
			for (auto& atom : _walk.atoms)
			{
				atom.position += running.drawOffset * atom.offsetWeight;
			}
			for (auto& joint : _walk.joints)
			{
				joint.position += running.drawOffset * joint.offsetWeight;
			}
		}
		particles::draw::AddEffect(frame, _walk, running.path, running.effect->GetOrigin(), running.effect->GetPlayer(),
		                           sources);
	}
	AddLightSheets(frame);
	_drawStats = {
	    .sprites = frame.sprites.size(),
	    .chains = frame.chains.size(),
	    .meshes = frame.meshes.size(),
	    .mists = frame.mists.size(),
	    .lightStamps = frame.lightStamps.size(),
	    .effects = frame.groups.size(),
	};
}

std::vector<ParticleSystemInterface::EffectInfo> ParticleSystem::GetEffects() const
{
	std::vector<EffectInfo> result;
	result.reserve(_effects.size());
	for (const auto& running : _effects)
	{
		const auto& effect = *running.effect;
		std::optional<float> secondsLeft;
		if (running.turnsLeft.has_value() && *running.turnsLeft >= 0)
		{
			secondsLeft = static_cast<float>(*running.turnsLeft) * k_TurnSeconds;
		}
		result.push_back({
		    .id = running.id,
		    .file = running.file,
		    .origin = effect.GetOrigin(),
		    .age = effect.GetAge(),
		    .atoms = effect.AtomCount(),
		    .collections = effect.CollectionCount(),
		    .closing = effect.Closing(),
		    .ownedBySpell = running.ownedBySpell,
		    .path = running.path,
		    .targets = effect.TargetCount(),
		    .secondsLeft = secondsLeft,
		    .unportedClasses = effect.UnportedClasses(),
		});
	}
	return result;
}

std::vector<std::string> ParticleSystem::GetFileNames() const
{
	std::vector<std::string> names;
	if (!Locator::filesystem::has_value())
	{
		return names;
	}
	Locator::filesystem::value().Iterate(ParticleFileDirectory(), false, [&names](const std::filesystem::path& file) {
		auto name = file.filename().string();
		if (name.ends_with(k_CompressedSuffix))
		{
			names.push_back(name.substr(0, name.size() - k_CompressedSuffix.size()));
		}
		else if (string_utils::LowerCase(file.extension().string()) == ".txt")
		{
			names.push_back(file.stem().string());
		}
	});
	std::ranges::sort(names);
	const auto [first, last] = std::ranges::unique(names);
	names.erase(first, last);
	return names;
}
