/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// How the miracles look outside their effects, as the renderer draws them: each one-shot globe added over what is
// behind it with its glint running, the miracle spinning inside it, and the rings round an extreme one; and the bands
// flying onto the hand that holds a miracle and the bracelets it wears.

#define LOCATOR_IMPLEMENTATIONS

#include <cstring>

#include <algorithm>
#include <iterator>
#include <optional>
#include <vector>

#include <bgfx/bgfx.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "3D/L3DMesh.h"
#include "3D/TempleInteriorInterface.h"
#include "Camera/Camera.h"
#include "ECS/Components/Hand.h"
#include "ECS/Components/HandMiracleFx.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Player.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/MiracleFxSystemInterface.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "Graphics/InstanceDesc.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/Texture2D.h"
#include "Graphics/ZSort.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/MagicTables.h"
#include "Magic/MiracleVisuals.h"
#include "Renderer.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::graphics;
namespace visuals = openblack::magic::visuals;

namespace
{
constexpr float k_ByteMax = 255.0f;
/// The seeds whose models take the game's first environment map in a globe: food and the creature spells' phials
constexpr auto k_FirstPhial = SpellSeedType::CreatureSpellFreeze;
constexpr auto k_LastPhial = SpellSeedType::CreatureSpellItchy;
/// The freeze phial's creature takes its spell in this way
constexpr int k_FreezeReceiveType = 0;
/// The beam explosion's seed is added over what is behind it without writing depth
constexpr auto k_AddedSeed = SpellSeedType::BeamExplosion;
/// The flying flock's seed is a bat for a god evil enough, otherwise a dove
constexpr auto k_DoveMesh = MeshId::AnimalSpellDove;
constexpr auto k_BatMesh = MeshId::AnimalBat1;

bool IsPhial(SpellSeedType seed)
{
	return static_cast<int>(seed) >= static_cast<int>(k_FirstPhial) && static_cast<int>(seed) <= static_cast<int>(k_LastPhial);
}

glm::vec3 ColourOf(uint32_t rgb)
{
	return glm::vec3(static_cast<float>((rgb >> 16u) & 0xFFu), static_cast<float>((rgb >> 8u) & 0xFFu),
	                 static_cast<float>(rgb & 0xFFu)) /
	       k_ByteMax;
}

/// What blends in the world, tested against its depth
constexpr uint64_t k_BlendedState = BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_MSAA;
/// Added over what is behind by its texture's alpha times the alpha it is tinted with
constexpr uint64_t k_AddedState = k_BlendedState | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE);
} // namespace

void Renderer::DrawGlobes(const DrawSceneDesc& desc) const
{
	const bool reflection = desc.viewId == RenderPass::Reflection;
	if ((desc.viewId != RenderPass::Main && !reflection) ||
	    (Locator::temple::has_value() && Locator::temple::value().Active()) || !Locator::infoConstants::has_value())
	{
		return;
	}
	using ecs::components::OneOffSpellSeed;
	using ecs::components::Transform;
	const auto& meshes = Locator::resources::value().GetMeshes();
	const auto& textures = Locator::resources::value().GetTextures();
	const auto& info = Locator::infoConstants::value();
	const auto& camera = *desc.camera;
	const auto eye = camera.GetOrigin();
	const auto envmap = entt::hashed_string("raw/envmap").value();
	const auto* environment = textures.Contains(envmap) ? &*textures.Handle(envmap) : nullptr;
	const auto ice = entt::hashed_string("raw/S_IceEnvMap").value();
	const auto* iceMap = textures.Contains(ice) ? &*textures.Handle(ice) : nullptr;
	const auto* objectProgram = _shaderManager->GetShader("Object");
	const auto* environmentProgram = _shaderManager->GetShader("ObjectEnvironment");
	const auto translucent = TranslucentPassOf(desc.viewId);
	const bool haveBubble = meshes.Contains(OneOffSpellSeed::k_MeshId.value());
	const bool haveRing = meshes.Contains(OneOffSpellSeed::k_RingMeshId.value());
	// A globe nobody owns shows its rings in the colour of the player at this computer
	const auto ringColour = ColourOf(ecs::components::Player::k_Colours.at(static_cast<size_t>(PlayerNames::PLAYER_ONE)));

	desc.entities.Each<const OneOffSpellSeed, const Transform>([&](const OneOffSpellSeed& globe, const Transform& transform) {
		const float scale = transform.scale.x * visuals::k_GlobeSeedScale;
		const auto& seed = magic::GetSpellSeedInfo(info, globe.seedType);

		// The miracle's seed spinning inside, below the middle, for the seeds that show one: lit by the land where it is,
		// food and the phials shining with the environment map, the beam added over what is behind it, a phial running
		// through its texture and drawn as its creature takes its spell
		auto meshId = seed.mesh;
		if (globe.seedType == SpellSeedType::FlockFlying)
		{
			const auto* flock = magic::GetMagicInfoAs<GMagicFlockFlyingInfo>(info, seed.magicTypes[0]);
			// Nobody owns a globe, and nobody's alignment of 0 is evil enough for the bat
			meshId = flock == nullptr || flock->alignmentSwitch <= 0.0f ? k_DoveMesh : k_BatMesh;
		}
		const auto seedMesh = resources::HashIdentifier(meshId);
		if (seed.useMesh != 0 && meshes.Contains(seedMesh))
		{
			const bool phial = IsPhial(globe.seedType);
			int receiveType = -1;
			if (phial)
			{
				if (const auto* spell = magic::GetMagicInfoAs<GMagicCreatureSpellInfo>(info, seed.magicTypes[0]))
				{
					receiveType = static_cast<int>(spell->creatureReceiveSpellType);
				}
			}
			const float pulse = visuals::PhialPulse(globe.phialPhase);
			const auto draws = phial ? visuals::PhialDraws(receiveType, globe.phialPhase, pulse, visuals::k_GlobeAlpha)
			                         : std::vector<visuals::PhialDraw> {{.size = 1.0f, .alpha = 255, .lit = true}};
			const auto shape = phial ? visuals::PhialScale(receiveType, globe.phialPhase) : glm::vec3(1.0f);
			const bool environmentMapped = environment != nullptr && (phial || globe.seedType == SpellSeedType::Food);
			const bool added = globe.seedType == k_AddedSeed;
			const auto mesh = meshes.Handle(seedMesh);
			const auto at = globe.middle + glm::vec3(0.0f, seed.meshHeight * scale, 0.0f);
			for (const auto& draw : draws)
			{
				const auto model = glm::translate(glm::mat4(1.0f), at) * glm::eulerAngleY(globe.spin) *
				                   glm::scale(glm::mat4(1.0f), seed.scale * scale * draw.size * shape);
				// A model of several bones (the flocks' birds and wolves) has each placed by its rest matrix
				std::vector<glm::mat4> bones;
				if (mesh->IsBoned() && !mesh->GetBoneMatrices().empty())
				{
					bones.reserve(mesh->GetBoneMatrices().size());
					std::ranges::transform(mesh->GetBoneMatrices(), std::back_inserter(bones),
					                       [&model](const glm::mat4& bone) { return model * bone; });
				}
				L3DMeshSubmitDesc submitDesc = {};
				submitDesc.program = environmentMapped ? environmentProgram : objectProgram;
				submitDesc.environment = environmentMapped ? environment : nullptr;
				submitDesc.modelMatrices = bones.empty() ? &model : bones.data();
				submitDesc.matrixCount = bones.empty() ? 1 : static_cast<uint8_t>(std::min<size_t>(bones.size(), UINT8_MAX));
				submitDesc.mirrored = reflection;
				submitDesc.useMaterialCulling = true;
				submitDesc.uvOffset = phial ? visuals::PhialUvOffset(globe.phialFrame) : glm::vec2(0.0f);
				submitDesc.sortDepth = zsort::Depth(at, eye);
				const float alpha = static_cast<float>(draw.alpha) / k_ByteMax;
				if (added)
				{
					// Added by its own alpha without writing depth, lit as the rest
					submitDesc.viewId = translucent;
					submitDesc.state = k_AddedState | BGFX_STATE_WRITE_A;
				}
				else if (!draw.lit)
				{
					// Plain white at its alpha, blended over what is behind
					submitDesc.viewId = translucent;
					submitDesc.state = k_BlendedState | BGFX_STATE_BLEND_ALPHA;
					submitDesc.tint = glm::vec4(1.0f, 1.0f, 1.0f, alpha);
				}
				else if (draw.alpha < 255)
				{
					// Lit, faded by its alpha
					submitDesc.viewId = translucent;
					submitDesc.state =
					    k_BlendedState | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_FACTOR, BGFX_STATE_BLEND_INV_FACTOR);
					submitDesc.rgba = static_cast<uint32_t>(draw.alpha) * 0x01010101u;
				}
				else
				{
					submitDesc.viewId = desc.viewId;
					submitDesc.state = k_BlendedState | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z;
					submitDesc.useMaterialBlending = true;
				}
				// A frozen phial is tinted icy by its pulse, and its ice shines over it
				const bool frozen = phial && receiveType == k_FreezeReceiveType;
				if (frozen)
				{
					submitDesc.tint = glm::vec4(ColourOf(visuals::FreezeTint(pulse)), 0.0f);
				}
				DrawMesh(*mesh, submitDesc, std::numeric_limits<uint8_t>::max());
				if (frozen && iceMap != nullptr)
				{
					submitDesc.viewId = translucent;
					submitDesc.program = environmentProgram;
					submitDesc.environment = iceMap;
					submitDesc.environmentOnlyAlpha = std::max(std::nearbyint(pulse * k_ByteMax), 1.0f) / k_ByteMax;
					submitDesc.useMaterialBlending = false;
					submitDesc.state = k_AddedState;
					submitDesc.tint = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
					DrawMesh(*mesh, submitDesc, std::numeric_limits<uint8_t>::max());
				}
			}
		}

		// The globe, added at 150 of 255 by its texture's alpha and writing its depth, turned to face the camera about its
		// middle, and with its origin pushed towards the camera from its middle by its radius so that it comes after what
		// is inside it, its glint running over its dome
		if (haveBubble)
		{
			const auto bubble = meshes.Handle(OneOffSpellSeed::k_MeshId.value());
			const auto halfExtent = bubble->GetBoundingBox().Size() * 0.5f;
			const float radius = std::max(halfExtent.x, halfExtent.z) * transform.scale.x;
			const auto towards = eye - globe.middle;
			const auto push = glm::length(towards) > 0.0f ? glm::normalize(towards) * radius : glm::vec3(0.0f);
			glm::mat4 model = glm::mat4(transform.rotation * glm::mat3(glm::scale(glm::mat4(1.0f), transform.scale)));
			model[3] = glm::vec4(globe.middle + push, 1.0f);
			L3DMeshSubmitDesc submitDesc = {};
			submitDesc.viewId = translucent;
			submitDesc.program = objectProgram;
			submitDesc.modelMatrices = &model;
			submitDesc.matrixCount = 1;
			submitDesc.mirrored = reflection;
			submitDesc.useMaterialCulling = true;
			submitDesc.state = k_AddedState | BGFX_STATE_WRITE_Z;
			submitDesc.tint = glm::vec4(1.0f, 1.0f, 1.0f, static_cast<float>(visuals::k_GlobeAlpha) / k_ByteMax);
			submitDesc.uvOffset = visuals::GlintUvOffset(globe.glintFrame);
			submitDesc.sortDepth = zsort::Depth(globe.middle + push, eye);
			DrawMesh(*bubble, submitDesc, std::numeric_limits<uint8_t>::max());
		}

		// An extreme miracle's rings, each drawn twice over, added without writing depth, in the player's colour with a
		// grey specular
		const int rings = visuals::RingCount(globe.powerUp);
		if (haveRing && rings > 0)
		{
			const auto ring = meshes.Handle(OneOffSpellSeed::k_RingMeshId.value());
			const float alpha = static_cast<float>(visuals::RingAlpha(visuals::k_GlobeAlpha)) / k_ByteMax;
			for (int i = 0; i < rings; ++i)
			{
				const auto model = visuals::RingModel(i, globe.ringSpin, globe.middle, scale, eye);
				L3DMeshSubmitDesc submitDesc = {};
				submitDesc.viewId = translucent;
				submitDesc.program = objectProgram;
				submitDesc.modelMatrices = &model;
				submitDesc.matrixCount = 1;
				submitDesc.state = k_AddedState;
				submitDesc.tint = glm::vec4(ringColour, alpha);
				submitDesc.lightAdd = ColourOf(visuals::k_RingSpecular);
				submitDesc.sortDepth = zsort::Depth(globe.middle, eye);
				DrawMesh(*ring, submitDesc, std::numeric_limits<uint8_t>::max());
				DrawMesh(*ring, submitDesc, std::numeric_limits<uint8_t>::max());
			}
		}
	});
}

namespace
{
/// The hand that holds the miracles, its look and where it is drawn: its matrix and each of its bones' in the world
struct HandPose
{
	const ecs::components::HandMiracleFx* fx {nullptr};
	glm::mat4 model {1.0f};
	std::vector<glm::mat4> bones;
};

std::optional<HandPose> PoseOfHand(const ecs::Registry& registry)
{
	using ecs::components::Hand;
	using ecs::components::HandMiracleFx;
	using ecs::components::Transform;
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(Hand::k_MeshId))
	{
		return std::nullopt;
	}
	std::optional<HandPose> found;
	registry.Each<const Hand, const HandMiracleFx, const Transform>([&](const Hand& hand, const HandMiracleFx& fx,
	                                                                    const Transform& transform) {
		if (found.has_value())
		{
			return;
		}
		// Placed as the world's models are
		const auto model = glm::translate(glm::mat4(1.0f), transform.position) * glm::mat4(transform.rotation) *
		                   glm::scale(glm::mat4(1.0f), transform.scale);
		const auto mesh = meshes.Handle(Hand::k_MeshId);
		const auto& rest = mesh->GetBoneMatrices();
		const auto& bones = hand.boneMatrices.size() == rest.size() ? hand.boneMatrices : rest;
		HandPose pose {.fx = &fx, .model = model, .bones = {}};
		pose.bones.reserve(bones.size());
		std::ranges::transform(bones, std::back_inserter(pose.bones), [&model](const glm::mat4& bone) { return model * bone; });
		found = std::move(pose);
	});
	return found;
}
} // namespace

const Texture2D* Renderer::HandFlowTexture() const
{
	// The flow's sheet is white where it is used, its alpha the flow; the game draws the two as one
	if (!_handFlowLoaded && Locator::filesystem::has_value())
	{
		_handFlowLoaded = true;
		constexpr uint16_t k_Size = 256;
		constexpr size_t k_Pixels = static_cast<size_t>(k_Size) * k_Size;
		auto& fileSystem = Locator::filesystem::value();
		const auto colourPath = fileSystem.GetPath<filesystem::Path::Textures>() / "S_Hand_Flow.raw";
		const auto alphaPath = fileSystem.GetPath<filesystem::Path::Textures>() / "S_Hand_Flowa.raw";
		if (fileSystem.Exists(colourPath) && fileSystem.Exists(alphaPath))
		{
			const auto colour = fileSystem.ReadAll(colourPath);
			const auto alpha = fileSystem.ReadAll(alphaPath);
			if (colour.size() == k_Pixels * 3 && alpha.size() == k_Pixels)
			{
				const auto* memory = bgfx::alloc(static_cast<uint32_t>(k_Pixels * 4));
				for (size_t i = 0; i < k_Pixels; ++i)
				{
					memory->data[(i * 4) + 0] = colour[(i * 3) + 0];
					memory->data[(i * 4) + 1] = colour[(i * 3) + 1];
					memory->data[(i * 4) + 2] = colour[(i * 3) + 2];
					memory->data[(i * 4) + 3] = alpha[i];
				}
				_handFlowTexture = std::make_unique<Texture2D>("HandFlow");
				_handFlowTexture->CreateWithinFrame(k_Size, k_Size, 1, TextureFormat::RGBA8, Wrapping::Repeat, Filter::Linear,
				                                    memory);
			}
		}
	}
	return _handFlowTexture.get();
}

void Renderer::DrawHandGlow(const DrawSceneDesc& desc, uint32_t handDepth) const
{
	using ecs::components::Hand;
	if (!desc.drawHand || desc.viewId == RenderPass::Reflection || !Locator::rendereringSystem::has_value())
	{
		return;
	}
	const auto* flow = HandFlowTexture();
	const auto pose = PoseOfHand(desc.entities);
	if (!pose.has_value() || !pose->fx->glowing || flow == nullptr || pose->bones.empty())
	{
		return;
	}
	// The hand drawn again over itself, added in its player's colour at four fifths: the flow's sheet, an eighth of it
	// at a time, running backwards over it
	const bool facesTurned = desc.cullBack != Locator::rendereringSystem::value().GetContext().handMirrored;
	L3DMeshSubmitDesc submitDesc = {};
	submitDesc.viewId = TranslucentPassOf(desc.viewId);
	submitDesc.program = _shaderManager->GetShader("Object");
	submitDesc.modelMatrices = pose->bones.data();
	submitDesc.matrixCount = static_cast<uint8_t>(std::min<size_t>(pose->bones.size(), UINT8_MAX));
	submitDesc.state = k_AddedState | (facesTurned ? BGFX_STATE_CULL_CW : BGFX_STATE_CULL_CCW);
	submitDesc.tint = glm::vec4(ColourOf(ecs::components::Player::k_Colours.at(static_cast<size_t>(PlayerNames::PLAYER_ONE))),
	                            visuals::k_GlowShare);
	submitDesc.skinTexture = &flow->GetNativeHandle();
	submitDesc.uvScale = visuals::k_GlowCellSize;
	submitDesc.uvOffset = visuals::GlowUvOffset(pose->fx->glowFrame);
	submitDesc.creatureShadows = false;
	// Just after the hand
	submitDesc.sortDepth = handDepth > 0 ? handDepth - 1 : 0;
	DrawMesh(*Locator::resources::value().GetMeshes().Handle(Hand::k_MeshId), submitDesc, std::numeric_limits<uint8_t>::max());
}

void Renderer::DrawTribalPower(const DrawSceneDesc& desc) const
{
	if (desc.viewId != RenderPass::Main || !Locator::miracleFxSystem::has_value())
	{
		return;
	}
	const auto& fx = Locator::miracleFxSystem::value();
	const auto* texture = fx.GetTextTexture();
	const auto vertices = fx.GetTribalPowerText();
	if (vertices.empty() || texture == nullptr)
	{
		return;
	}
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	const auto count = static_cast<uint32_t>(vertices.size());
	if (bgfx::getAvailTransientVertexBuffer(count, layout) < count)
	{
		return;
	}
	bgfx::TransientVertexBuffer buffer;
	bgfx::allocTransientVertexBuffer(&buffer, count, layout);
	std::memcpy(buffer.data, vertices.data(), count * sizeof(OrientedTextVertex));

	// The game's text in the world: the glyphs blended by their coverage, tested against the depth of what is drawn,
	// seen from both sides
	const auto* shader = _shaderManager->GetShader("Text3D");
	const auto model = glm::mat4(1.0f);
	bgfx::setTransform(glm::value_ptr(model));
	bgfx::setVertexBuffer(0, &buffer);
	shader->SetTextureSampler("s_texture", 0, *texture);
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_MSAA |
	               BGFX_STATE_BLEND_ALPHA);
	shader->Submit(static_cast<bgfx::ViewId>(desc.viewId));
}

void Renderer::DrawHandMiracleBands(const DrawSceneDesc& desc) const
{
	using ecs::components::Hand;
	using ecs::components::HandMiracleFx;
	using ecs::components::OneOffSpellSeed;
	if (desc.viewId != RenderPass::Main || !desc.drawHand || !Locator::rendereringSystem::has_value())
	{
		return;
	}
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(OneOffSpellSeed::k_RingMeshId.value()) || !meshes.Contains(Hand::k_MeshId))
	{
		return;
	}
	const auto pose = PoseOfHand(desc.entities);
	if (!pose.has_value() || (pose->fx->bands.bracelets.empty() && pose->fx->bands.flying.empty()))
	{
		return;
	}
	const auto* fx = pose->fx;
	// The hand's root, where the bands go round it, and the camera's frame they fly from
	const glm::mat4 root = pose->bones.empty() ? pose->model : pose->bones[0];
	const auto& camera = *desc.camera;
	glm::mat4 cameraFrame {glm::vec4(camera.GetRight(), 0.0f), glm::vec4(camera.GetUp(), 0.0f),
	                       glm::vec4(camera.GetForward(), 0.0f), glm::vec4(camera.GetOrigin(), 1.0f)};
	const auto atCamera = cameraFrame * visuals::BandAtCamera(camera.GetNearClip());
	const auto colour = ColourOf(ecs::components::Player::k_Colours.at(static_cast<size_t>(PlayerNames::PLAYER_ONE)));
	const auto ring = meshes.Handle(OneOffSpellSeed::k_RingMeshId.value());
	const auto* program = _shaderManager->GetShader("Object");
	const auto translucent = TranslucentPassOf(desc.viewId);
	const auto draw = [&](const visuals::HandBand& band) {
		const auto pose = visuals::PoseOf(band);
		if (!pose.shown || pose.alpha == 0)
		{
			return;
		}
		// On the hand once there; on the way a bracelet eases every part of its matrix towards the hand without its spin,
		// and a flying band keeps the camera's turn while it shrinks and moves to the spinning band's place
		const auto onHand = root * visuals::BandOnHand(band);
		glm::mat4 model = onHand;
		if (pose.t < 1.0f)
		{
			model = band.kind == visuals::BandKind::Bracelet
			            ? visuals::BandBlended(atCamera, root * visuals::BandOnHandUnspun(band), pose.t)
			            : visuals::BandBetween(atCamera, onHand, pose.t);
		}
		L3DMeshSubmitDesc submitDesc = {};
		submitDesc.viewId = translucent;
		submitDesc.program = program;
		submitDesc.modelMatrices = &model;
		submitDesc.matrixCount = 1;
		submitDesc.state = k_AddedState;
		submitDesc.tint = glm::vec4(colour, std::max(static_cast<float>(pose.alpha) / k_ByteMax, 1.0f / k_ByteMax));
		submitDesc.sortDepth = zsort::Depth(glm::vec3(model[3]), camera.GetOrigin());
		DrawMesh(*ring, submitDesc, std::numeric_limits<uint8_t>::max());
	};
	std::ranges::for_each(fx->bands.bracelets, draw);
	std::ranges::for_each(fx->bands.flying, draw);
}
