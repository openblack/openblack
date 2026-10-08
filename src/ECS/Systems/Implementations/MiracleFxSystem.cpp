/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "MiracleFxSystem.h"

#include <cmath>

#include <numbers>
#include <vector>

#include <entt/core/hashed_string.hpp>
#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Camera/Camera.h"
#include "ECS/Components/Hand.h"
#include "ECS/Components/HandMiracleFx.h"
#include "ECS/Components/MagicPile.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/ResourcePile.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/DispenserRules.h"
#include "Magic/MagicTables.h"
#include "Magic/MiracleVisuals.h"
#include "Particles/ParticleEffect.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace visuals = openblack::magic::visuals;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// The creature spells' seeds, whose phials pulse
constexpr auto k_FirstPhial = SpellSeedType::CreatureSpellFreeze;
constexpr auto k_LastPhial = SpellSeedType::CreatureSpellItchy;

bool IsPhial(SpellSeedType seed)
{
	return static_cast<int>(seed) >= static_cast<int>(k_FirstPhial) && static_cast<int>(seed) <= static_cast<int>(k_LastPhial);
}

/// How a creature takes the spell of a phial, which decides its pulse
int ReceiveTypeOf(const InfoConstants& info, SpellSeedType seed)
{
	const auto type = magic::GetSpellSeedInfo(info, seed).magicTypes[0];
	const auto* spell = magic::GetMagicInfoAs<GMagicCreatureSpellInfo>(info, type);
	return spell != nullptr ? static_cast<int>(spell->creatureReceiveSpellType) : 0;
}

HandMiracleFx* HandFx()
{
	// The hand of the player at this computer
	if (!Locator::handSystem::has_value())
	{
		return nullptr;
	}
	auto& registry = Locator::entitiesRegistry::value();
	const auto hand = Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(HandSystemInterface::Side::Left)];
	if (!registry.Valid(hand) || !registry.AllOf<Hand>(hand))
	{
		return nullptr;
	}
	auto* found = registry.TryGet<HandMiracleFx>(hand);
	return found != nullptr ? found : &registry.Assign<HandMiracleFx>(hand);
}

void PlaySound(entt::id_type id)
{
	if (Locator::audio::has_value())
	{
		Locator::audio::value().StartSoundEffect(id, {});
	}
}
} // namespace

entt::id_type MiracleFxSystem::LoadMesh(entt::id_type id, std::string_view file)
{
	if (!Locator::resources::has_value())
	{
		return id;
	}
	auto& meshes = Locator::resources::value().GetMeshes();
	if (meshes.Contains(id) || !Locator::filesystem::has_value())
	{
		return id;
	}
	auto& fileSystem = Locator::filesystem::value();
	try
	{
		meshes.Load(id, resources::L3DLoader::FromDiskTag {},
		            fileSystem.FindPath(fileSystem.GetPath<filesystem::Path::Data>() / file));
	}
	catch (const std::exception& error)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "Miracles: cannot load {}: {}", file, error.what());
	}
	return id;
}

void MiracleFxSystem::LoadMeshes()
{
	LoadMesh(OneOffSpellSeed::k_MeshId.value(), OneOffSpellSeed::k_MeshFile);
	LoadMesh(OneOffSpellSeed::k_RingMeshId.value(), OneOffSpellSeed::k_RingMeshFile);
}

entt::id_type MiracleFxSystem::BubbleMesh()
{
	return LoadMesh(OneOffSpellSeed::k_MeshId.value(), OneOffSpellSeed::k_MeshFile);
}

entt::id_type MiracleFxSystem::BandMesh()
{
	return LoadMesh(OneOffSpellSeed::k_RingMeshId.value(), OneOffSpellSeed::k_RingMeshFile);
}

void MiracleFxSystem::Update(float /*seconds*/, float gameSeconds)
{
	if (!Locator::infoConstants::has_value() || !Locator::entitiesRegistry::has_value())
	{
		return;
	}
	UpdateGlobes(gameSeconds);
	// The hand's bands and glow keep to the game's time, stopping while it is paused
	UpdateHand(gameSeconds);
	UpdatePiles(gameSeconds);
}

void MiracleFxSystem::UpdatePiles(float gameSeconds)
{
	if (!Locator::resources::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	const auto& info = Locator::infoConstants::value();
	const auto& meshes = Locator::resources::value().GetMeshes();
	registry.Each<ResourcePile, const Pot, const Mesh, Transform>([&](entt::entity entity, ResourcePile& pile, const Pot& pot,
	                                                                  const Mesh& mesh, Transform& transform) {
		// A miracle's pile stands on the land where it is, as the land is now, sunk or raised from there
		if (registry.AllOf<MagicPile>(entity) && Locator::terrainSystem::has_value())
		{
			transform.position.y = Locator::terrainSystem::value().GetHeightAt({transform.position.x, transform.position.z});
		}
		if (pile.height <= 0.0f)
		{
			if (!meshes.Contains(mesh.id))
			{
				return;
			}
			pile.height = meshes.Handle(mesh.id)->GetBoundingBox().Size().y * transform.scale.y;
		}
		const auto& potInfo = info.pot.at(static_cast<size_t>(pot.type));
		const auto target = magic::piles::SunkOffset(
		    magic::piles::ProportionRaised(potInfo.resourceType, pot.amount, potInfo.maxAmountInPot), pile.height);
		if (!pile.risen)
		{
			pile.rise = magic::piles::SunkRise(pile.height);
			magic::piles::RiseTo(pile.rise, target);
			pile.risen = true;
			pile.shownAmount = pot.amount;
		}
		else if (pot.amount != pile.shownAmount)
		{
			magic::piles::RiseTo(pile.rise, target);
			pile.shownAmount = pot.amount;
		}
		magic::piles::StepRise(pile.rise, gameSeconds);
	});
}

void MiracleFxSystem::UpdateGlobes(float seconds)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto& info = Locator::infoConstants::value();
	auto& meshes = Locator::resources::value().GetMeshes();
	LoadMeshes();
	const auto bubble = OneOffSpellSeed::k_MeshId.value();
	const glm::vec3 centre = meshes.Contains(bubble) ? meshes.Handle(bubble)->GetBoundingBox().Center() : glm::vec3(0.0f);
	const auto eye = Locator::camera::has_value() ? Locator::camera::value().GetOrigin() : glm::vec3(0.0f);
	auto* particles = Locator::particleSystem::has_value() ? &Locator::particleSystem::value() : nullptr;

	std::vector<entt::entity> globes;
	registry.Each<OneOffSpellSeed, Transform>([&](entt::entity entity, OneOffSpellSeed& globe, Transform& transform) {
		globes.push_back(entity);
		globe.spin = std::fmod(globe.spin + magic::k_OrbSeedSpin * seconds, k_TwoPi);
		globe.ringSpin = std::fmod(globe.ringSpin + visuals::k_RingSpin * seconds, k_TwoPi);
		globe.glintFrame = visuals::StepGlint(globe.glintFrame, seconds);
		// The dome's top faces the camera, turning about the middle of the globe
		globe.middle = globe.position + centre * transform.scale;
		transform.rotation = visuals::FacingCamera(globe.middle, eye, visuals::Facing::Globe);
		transform.position = globe.middle - transform.rotation * (centre * transform.scale);
		const auto& seed = magic::GetSpellSeedInfo(info, globe.seedType);
		if (IsPhial(globe.seedType))
		{
			const int receiveType = ReceiveTypeOf(info, globe.seedType);
			globe.phialFrame = visuals::StepPhialFrame(globe.phialFrame, seconds);
			globe.phialPhase = std::fmod(globe.phialPhase + visuals::PhialPulseSpeed(receiveType) * seconds, 1.0f);
		}
		// The miracle's own effect in the globe steps as the globe is drawn, before the seed's model is placed for this
		// frame, so its glints sparkle on where the model was last drawn
		// TODO(raffclar): the game steps it only while the globe is drawn on screen
		if (const auto stepped = _holderEffects.find(entity); stepped != _holderEffects.end() && particles != nullptr)
		{
			particles->ProcessByFrame(stepped->second, seconds);
		}
		// The seed's model as it is drawn, which glints of its effect sparkle on from the next step: below the middle,
		// spun, at the seed's scale and a phial's swell or squash, a big one's growing copy last
		{
			const float drawn = transform.scale.x * visuals::k_GlobeSeedScale;
			float size = 1.0f;
			glm::vec3 shape(1.0f);
			if (IsPhial(globe.seedType))
			{
				const int receiveType = ReceiveTypeOf(info, globe.seedType);
				size = visuals::PhialDraws(receiveType, globe.phialPhase, visuals::PhialPulse(globe.phialPhase),
				                           visuals::k_GlobeAlpha)
				           .back()
				           .size;
				shape = visuals::PhialScale(receiveType, globe.phialPhase);
			}
			const auto at = globe.middle + glm::vec3(0.0f, seed.meshHeight * drawn, 0.0f);
			globe.seedPlacement = glm::translate(glm::mat4(1.0f), at) * glm::eulerAngleY(globe.spin) *
			                      glm::scale(glm::mat4(1.0f), seed.scale * drawn * size * shape);
		}
		// The miracle's own effect plays in the globe, faint as the globe is
		if (particles == nullptr || seed.holderParticle == ParticleType::None)
		{
			return;
		}
		const float scale = transform.scale.x * visuals::k_GlobeSeedScale;
		const auto point = globe.middle + glm::vec3(0.0f, seed.holderHeight * scale, 0.0f);
		auto found = _holderEffects.find(entity);
		if (found == _holderEffects.end())
		{
			const auto effect = particles->Start(seed.holderParticle, point, scale);
			if (effect == ParticleSystemInterface::k_NoEffect)
			{
				return;
			}
			// What it acts on is the seed's model, for the glints a frozen creature's phial shows
			particles->AddTarget(effect, entity);
			if (auto* running = particles->Find(effect))
			{
				// Nobody owns a globe: its effect takes the neutral player's colour
				running->SetPlayer(static_cast<int>(PlayerNames::NEUTRAL));
				running->SetGlobalAlpha(static_cast<float>(visuals::k_GlobeAlpha));
			}
			found = _holderEffects.emplace(entity, effect).first;
		}
		particles->SetOrigin(found->second, point);
	});
	// The effects of the globes that have gone go with them
	std::erase_if(_holderEffects, [&](const auto& entry) {
		if (std::ranges::find(globes, entry.first) != globes.end())
		{
			return false;
		}
		if (particles != nullptr)
		{
			particles->Delete(entry.second);
		}
		return true;
	});
}

void MiracleFxSystem::UpdateHand(float seconds)
{
	auto* fx = HandFx();
	if (fx == nullptr)
	{
		return;
	}
	fx->glowing = Locator::magicSystem::has_value() && Locator::magicSystem::value().GetHeldSeed().has_value();
	fx->glowFrame = visuals::StepGlowFrame(fx->glowFrame, seconds);
	visuals::StepBands(fx->bands, seconds);
}

void MiracleFxSystem::SeedInHand(int powerUp, int previousPowerUp)
{
	if (auto* fx = HandFx())
	{
		// A bracelet for each power-up, put on once the miracle has settled
		visuals::SetBracelets(fx->bands, powerUp + 1, true);
		if (previousPowerUp <= powerUp)
		{
			visuals::AddFlyIn(fx->bands, false);
		}
	}
	if (previousPowerUp <= powerUp)
	{
		PlaySound(static_cast<entt::id_type>(audio::SoundId::G_SpellPowerUpBand));
	}
	if (const auto voice = visuals::PowerUpVoiceSample(powerUp))
	{
		PlaySound(entt::hashed_string(fmt::format("SpellDialogue.sad/{}", *voice).c_str()).value());
	}
}

void MiracleFxSystem::SeedLeftHand()
{
	if (auto* fx = HandFx())
	{
		visuals::SetBracelets(fx->bands, 0, false);
	}
}

void MiracleFxSystem::SeedShakenOff()
{
	if (auto* fx = HandFx())
	{
		visuals::AddFlyOff(fx->bands);
	}
}

void MiracleFxSystem::Reset()
{
	_runners.clear();
	_ring = nullptr;
	if (Locator::particleSystem::has_value())
	{
		for (const auto& [globe, effect] : _holderEffects)
		{
			Locator::particleSystem::value().Delete(effect);
		}
	}
	_holderEffects.clear();
	if (Locator::entitiesRegistry::has_value())
	{
		if (auto* fx = HandFx())
		{
			*fx = {};
		}
	}
}
