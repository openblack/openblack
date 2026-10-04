/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureCaveEffects.h"

#include <algorithm>

#include <L3DFile.h>
#include <entt/core/hashed_string.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <spdlog/spdlog.h>

#include "3D/TempleInteriorInterface.h"
#include "Common/RandomNumberManager.h"
#include "ECS/Components/MistDome.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "FileSystem/FileSystemInterface.h"
#include "Graphics/LightBeams.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;

namespace
{
// The smoke texture's and the fire's frames are 8 by 8 cells of their textures (LH3DSprite)
constexpr float k_Cell = 1.0f / 8.0f;

// CreatureRoom::InitEngine's flames: four sprites of S_Fire over the fire, orange and added by alpha
constexpr size_t k_Flames = 4;
constexpr glm::vec4 k_FlameColour {1.0f, 128.0f / 255.0f, 64.0f / 255.0f, 128.0f / 255.0f};

// Its smoke over the fire, grey, two units above it
constexpr size_t k_FireSmokes = 2;
constexpr glm::vec3 k_FireSmokeColour {64.0f / 255.0f};
constexpr float k_FireSmokeSize = 2.0f;

// The spray at the waterfall's foot, sixteen smokes thrown up from places CreatureRoom::Draw picks again every frame
constexpr size_t k_Sprays = 16;
constexpr float k_SpraySize = 4.0f;

// The mist there, four domes of mist.l3d
constexpr size_t k_Mists = 4;
constexpr glm::vec4 k_MistColour {1.0f, 1.0f, 1.0f, 128.0f / 255.0f};

// fn_007F8E00's rates: the particles' spin a second, their drift, the pull of the wind on them, how they fall when
// thrown up, and the steps of age a second
constexpr float k_Spin = 0.765f;
constexpr float k_DriftSpeed = 2.55f;
constexpr float k_Pull = 1.5f;
constexpr float k_RisingFall = 2.5f;
constexpr float k_AgePerSecond = 255.0f;
/// The longest step it takes at once
constexpr float k_LongestStep = 100.0f;
/// How big the particles grow, from half their smoke's size, by age
constexpr float k_GrowthPerAge = 0.0022222223f;
constexpr float k_SmallestSize = 1e-4f;

float Random(float min, float max)
{
	return Locator::rng::value().NextValue(min, max);
}

const graphics::Texture2D& Texture(const char* name)
{
	return *Locator::resources::value().GetTextures().Handle(entt::hashed_string(name));
}

/// The cell of a frame of an 8 by 8 texture
glm::vec2 Cell(uint32_t frame)
{
	return {static_cast<float>(frame % 8) * k_Cell, static_cast<float>(frame / 8) * k_Cell};
}

/// The dome of data/landscape/mist.l3d, with its corners' places in the texture's first cell
std::shared_ptr<graphics::BeamMesh> LoadMistDome()
{
	auto& fileSystem = Locator::filesystem::value();
	const auto path = fileSystem.GetPath<filesystem::Path::Landscape>() / "mist.l3d";
	l3d::L3DFile file;
	if (!fileSystem.Exists(path) || file.ReadFile(*fileSystem.GetData(path)) != l3d::L3DResult::Success)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "The mist's dome, {}, can't be read", path.generic_string());
		return nullptr;
	}
	auto dome = std::make_shared<graphics::BeamMesh>();
	for (uint32_t submesh = 0; submesh < file.GetSubmeshHeaders().size(); ++submesh)
	{
		const auto base = static_cast<uint16_t>(dome->vertices.size());
		const auto& vertices = file.GetVertexSpan(submesh);
		for (const auto& vertex : vertices)
		{
			dome->vertices.push_back({
			    .position = {vertex.position.x, vertex.position.y, vertex.position.z},
			    .colour = glm::u8vec4(255),
			    .uv = {vertex.texCoord.x, vertex.texCoord.y},
			});
		}
		uint16_t primitiveBase = 0;
		uint32_t index = 0;
		const auto& indices = file.GetIndexSpan(submesh);
		for (const auto& primitive : file.GetPrimitiveSpan(submesh))
		{
			for (uint32_t i = 0; i < primitive.numTriangles * 3 && index < indices.size(); ++i, ++index)
			{
				dome->indices.push_back(static_cast<uint16_t>(base + primitiveBase + indices[index]));
			}
			primitiveBase = static_cast<uint16_t>(primitiveBase + primitive.numVertices);
		}
	}
	return dome;
}

entt::entity CreateSprite(const char* texture, const char* alpha, bool additive, glm::vec3 position)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto entity = registry.Create();
	registry.Assign<TempleInteriorPart>(entity, TempleRoom::CreatureCave);
	registry.Assign<Sprite>(entity, Texture(texture).GetNativeHandle(), glm::vec2(0.0f), glm::vec2(k_Cell), glm::vec4(1.0f),
	                        additive, true, Texture(alpha).GetNativeHandle());
	registry.Assign<Transform>(entity, position, glm::mat3(1.0f), glm::vec3(1.0f));
	return entity;
}

/// LH3DSmoke::Create: its particles start on their way up from the world's origin, unseen until they first start over
CreatureCaveEffects::Smoke CreateSmoke()
{
	CreatureCaveEffects::Smoke smoke;
	for (size_t i = 0; i < smoke.particles.size(); ++i)
	{
		auto& particle = smoke.particles[i];
		particle.position = {0.0f, static_cast<float>(i) * 0.5f, 0.0f};
		particle.age = static_cast<int32_t>(i) * 90;
		particle.angle = Random(0.0f, glm::pi<float>());
		particle.spinsBack = (static_cast<int32_t>(Random(1.0f, 100.0f)) & 1) != 0;
		particle.hidden = true;
		particle.sprite = CreateSprite("raw/smoke", "raw/smokea", false, particle.position);
	}
	return smoke;
}
} // namespace

CreatureCaveEffects::CreatureCaveEffects(glm::vec3 fire)
{
	// CreatureRoom::InitEngine
	auto& registry = Locator::entitiesRegistry::value();
	for (size_t i = 0; i < k_Flames; ++i)
	{
		const glm::vec3 place = fire + glm::vec3(Random(-1.0f, 1.0f), Random(1.0f, 2.0f), Random(-1.0f, 1.0f));
		const auto flame = CreateSprite("raw/S_Fire", "raw/S_Firea", true, place);
		const float size = std::max(static_cast<float>(i) * 0.5f + Random(0.8f, 1.2f) + 2.0f, k_SmallestSize);
		registry.Get<Transform>(flame).scale = glm::vec3(size, size, 1.0f);
		registry.Get<Sprite>(flame).tint = k_FlameColour;
		_flames.push_back(flame);
	}

	for (size_t i = 0; i < k_Sprays; ++i)
	{
		auto spray = CreateSmoke();
		// From blue to white across the sixteen, as their colours' bytes wrap
		const auto step = static_cast<int32_t>(i * 0xff) / 16;
		const auto colour = ((static_cast<uint32_t>((step << 12) >> 8) - 0x1100U) & 0xff00U) |
		                    ((static_cast<uint32_t>((step * 0x2f0000) >> 8) - 0x300000U) & 0xff0000U) | 0xffU;
		spray.colour = glm::vec3((colour >> 16) & 0xff, (colour >> 8) & 0xff, colour & 0xff) / 255.0f;
		spray.drift = {Random(0.0f, 1.0f) - 0.5f, Random(0.0f, 1.0f), Random(0.0f, 1.0f) - 0.5f};
		spray.rising = true;
		spray.size = k_SpraySize;
		_spray.push_back(std::move(spray));
	}

	for (size_t i = 0; i < k_FireSmokes; ++i)
	{
		auto smoke = CreateSmoke();
		smoke.place = fire + glm::vec3(Random(-1.0f, 1.0f), Random(-1.0f, 1.0f) + 2.0f, Random(-1.0f, 1.0f));
		smoke.colour = k_FireSmokeColour;
		smoke.size = k_FireSmokeSize;
		_fireSmoke.push_back(std::move(smoke));
	}

	_mistDome = LoadMistDome();
	for (size_t i = 0; i < k_Mists; ++i)
	{
		const glm::vec3 place {Random(0.0f, 1.0f) + 160.0f - 0.5f, -45.0f - Random(0.0f, 8.0f),
		                       Random(0.0f, 1.0f) - 30.0f - 0.5f};
		auto mist = registry.Create();
		registry.Assign<TempleInteriorPart>(mist, TempleRoom::CreatureCave);
		registry.Assign<Transform>(mist, place, glm::mat3(1.0f), glm::vec3(1.0f));
		registry.Assign<MistDome>(mist, _mistDome, glm::vec2(0.0f), k_MistColour);
		// LH3DMist starts at a random frame of its texture
		_mists.push_back({mist, static_cast<int32_t>(Random(0.0f, 16.0f)) & 0xf});
	}
}

CreatureCaveEffects::~CreatureCaveEffects()
{
	// The registry may have gone before the temple, as the game closes
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	for (const auto entity : _flames)
	{
		registry.Destroy(entity);
	}
	for (const auto* smokes : {&_spray, &_fireSmoke})
	{
		for (const auto& smoke : *smokes)
		{
			for (const auto& particle : smoke.particles)
			{
				registry.Destroy(particle.sprite);
			}
		}
	}
	for (const auto& mist : _mists)
	{
		registry.Destroy(mist.entity);
	}
}

uint32_t CreatureCaveEffects::FlameFrame(uint32_t tickCount, uint32_t flame)
{
	// CreatureRoom::DrawAdditional: the flames' 32 frames, every 32 milliseconds, each a quarter of the way on from the
	// one before it
	return 0x1fU - (((tickCount >> 5U) + flame * 8U) & 0x1fU);
}

uint8_t CreatureCaveEffects::SmokeAlpha(int32_t age, bool rising)
{
	constexpr int32_t k_Alpha = 0x4f;
	constexpr int32_t k_Fading = 0xe2;
	if (age < k_Fading)
	{
		// Smoke thrown up fades in over its first hundred steps
		return static_cast<uint8_t>(rising && age <= 99 ? age * k_Alpha / 100 : k_Alpha);
	}
	return static_cast<uint8_t>((0xe1 - age) * k_Alpha / 0x2a3 + k_Alpha);
}

void CreatureCaveEffects::StepSmoke(Smoke& smoke, uint32_t milliseconds)
{
	// fn_007F8E00. The cave's smokes are out of the wind: only smoke thrown up pulls down.
	glm::vec3 pull {0.0f};
	if (smoke.rising)
	{
		pull.y -= k_RisingFall;
	}
	const float seconds = std::min(static_cast<float>(milliseconds) * 0.001f, k_LongestStep);
	const float spin = seconds * k_Spin;
	const auto ageing = static_cast<int32_t>(seconds * k_AgePerSecond);

	const auto move = [&smoke, pull](Smoke::Particle& particle, float time) {
		const auto velocity = particle.velocity + pull * time * k_Pull;
		particle.position += (particle.velocity + velocity) * time * 0.5f;
		particle.velocity = velocity;
		particle.position += smoke.drift * time * k_DriftSpeed;
	};
	for (auto& particle : smoke.particles)
	{
		particle.age += ageing;
		particle.angle += particle.spinsBack ? spin : -spin;
		if (particle.age <= k_SmokeLife)
		{
			move(particle, seconds);
			continue;
		}
		// It starts over from the smoke's place, as far on as it has gone past its end
		particle.position = smoke.place;
		particle.age %= k_SmokeLife;
		particle.hidden = false;
		particle.velocity =
		    smoke.rising ? glm::vec3(Random(-4.0f, 4.0f), Random(3.0f, 4.0f), Random(-4.0f, 4.0f)) : glm::vec3(0.0f);
		move(particle, static_cast<float>(particle.age) / k_AgePerSecond);
	}
}

void CreatureCaveEffects::ShowSmoke(const Smoke& smoke) const
{
	auto& registry = Locator::entitiesRegistry::value();
	for (const auto& particle : smoke.particles)
	{
		auto& sprite = registry.Get<Sprite>(particle.sprite);
		auto& transform = registry.Get<Transform>(particle.sprite);
		// Its frame of the first 16 of the smoke texture, its size and its fade by its age
		sprite.uvMin = Cell(static_cast<uint32_t>(particle.age * 45 / k_SmokeLife) & 0xfU);
		const float size = std::max((static_cast<float>(particle.age) * k_GrowthPerAge + 0.5f) * smoke.size, k_SmallestSize);
		const float alpha = particle.hidden ? 0.0f : static_cast<float>(SmokeAlpha(particle.age, smoke.rising)) / 255.0f;
		sprite.tint = glm::vec4(smoke.colour, alpha);
		transform.position = particle.position;
		transform.rotation = glm::mat3(glm::rotate(particle.angle, glm::vec3(0.0f, 0.0f, 1.0f)));
		transform.scale = glm::vec3(size, size, 1.0f);
	}
}

void CreatureCaveEffects::Update(uint32_t milliseconds, uint32_t tickCount)
{
	auto& registry = Locator::entitiesRegistry::value();

	// CreatureRoom::Draw: the spray from new places about the waterfall's foot, and the fire's smoke
	for (auto& spray : _spray)
	{
		spray.place = {Random(0.0f, 5.0f) + 160.0f - 2.5f, -30.0f - Random(0.0f, 23.0f), Random(0.0f, 1.0f) - 16.0f - 0.5f};
		StepSmoke(spray, milliseconds);
		ShowSmoke(spray);
	}
	for (auto& smoke : _fireSmoke)
	{
		StepSmoke(smoke, milliseconds);
		ShowSmoke(smoke);
	}

	// LH3DMist::Draw: each dome steps on through 16 frames of the smoke texture
	for (auto& mist : _mists)
	{
		mist.counter += static_cast<int32_t>(static_cast<float>(milliseconds) * 0.255f);
		if (mist.counter > k_SmokeLife)
		{
			mist.counter %= k_SmokeLife;
		}
		const auto frame = static_cast<uint32_t>(mist.counter * 45 / k_SmokeLife) & 0xfU;
		registry.Get<MistDome>(mist.entity).uvOffset = Cell(frame);
	}

	// CreatureRoom::DrawAdditional: the flames' frames
	for (uint32_t i = 0; i < _flames.size(); ++i)
	{
		registry.Get<Sprite>(_flames[i]).uvMin = Cell(FlameFrame(tickCount, i));
	}
}
