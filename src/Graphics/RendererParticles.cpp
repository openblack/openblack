/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The particle effects as the renderer draws them: gathered once a frame, ordered for each pass's camera, and drawn among
// the other things that blend. The sprites go to the GPU as one buffer of instances a pass, a draw for each run that
// shares a sheet; the ribbons as one buffer of corners the vertex shader moves to face the camera; the models and mists
// through the passes the rest of the world's use. Their light maps light the land's colours.

#define LOCATOR_IMPLEMENTATIONS

#include <cstring>

#include <algorithm>
#include <iterator>
#include <span>
#include <vector>

#include <StackedBitmap.h>
#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/LandLightTable.h"
#include "3D/Mists.h"
#include "3D/TempleInteriorInterface.h"
#include "Camera/Camera.h"
#include "ECS/Components/Hand.h"
#include "ECS/Components/Mist.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/ExplosionSystemInterface.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "Graphics/GraphicsHandleBgfx.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/ModelLight.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/ZSort.h"
#include "Locator.h"
#include "Particles/ParticleBlast.h"
#include "Particles/ParticleCreators.h"
#include "Particles/SurfaceOfRevolution.h"
#include "Profiler.h"
#include "Renderer.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::graphics;
using particles::draw::ItemKind;

namespace
{
/// The corners of each segment of a ribbon, as two triangles
constexpr std::array<uint32_t, 6> k_SegmentIndices {0, 1, 2, 1, 3, 2};
constexpr uint32_t k_CornersPerSegment = 4;

/// What blends in the world: tested against depth but drawn on both sides, added to what is behind or blended over it,
/// writing depth when asked
uint64_t BlendState(render_modes::Mode mode)
{
	const auto& desc = render_modes::Desc(mode);
	uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER;
	state |= desc.blend == render_modes::Blend::Additive
	             ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE)
	             : BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
	if (desc.zWrite)
	{
		state |= BGFX_STATE_WRITE_Z;
	}
	return state;
}

bgfx::VertexLayout ChainLayout()
{
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .add(bgfx::Attrib::TexCoord3, 2, bgfx::AttribType::Float)
	    .end();
	return layout;
}

/// Where the hand is drawn this frame, which the miracle in it is drawn just after
std::optional<glm::vec3> HandPoint()
{
	if (!Locator::rendereringSystem::has_value())
	{
		return std::nullopt;
	}
	const auto& context = Locator::rendereringSystem::value().GetContext();
	const auto hand = context.instancedDrawDescs.find(ecs::components::Hand::k_MeshId);
	if (hand == context.instancedDrawDescs.end() || hand->second.count == 0 ||
	    hand->second.offset >= context.instanceUniforms.size())
	{
		return std::nullopt;
	}
	return glm::vec3(context.instanceUniforms.at(hand->second.offset).model[3]);
}

/// A model turned about its own vertical to face the camera across the ground, its height stretched
glm::mat3 FacingCamera(glm::mat3 axes, const glm::vec3& position, const glm::vec3& eye, float heightStretch)
{
	glm::vec3 forward = axes[2];
	if (forward != glm::vec3(0.0f))
	{
		forward = glm::normalize(forward);
	}
	glm::vec2 toModel(position.x - eye.x, position.z - eye.z);
	if (toModel != glm::vec2(0.0f))
	{
		toModel = glm::normalize(toModel);
	}
	const float turn = std::atan2(toModel.y, toModel.x) - std::atan2(forward.z, forward.x);
	const float c = std::cos(turn);
	const float s = std::sin(turn);
	const glm::vec3 across = axes[0];
	axes[0] = (c * across) + (s * axes[2]);
	axes[2] = (c * axes[2]) - (s * across);
	axes[1] *= heightStretch;
	return axes;
}
} // namespace

void Renderer::CollectParticles() const
{
	auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::ParticlesGather);
	if (!Locator::particleSystem::has_value() || !Locator::time::has_value())
	{
		_particleFrame.Clear();
		return;
	}
	Locator::particleSystem::value().CollectDrawFrame(Locator::time::value().GetTurnFraction(), _particleFrame);
	// What burns, among the other things that blend
	if (Locator::fireSystem::has_value())
	{
		Locator::fireSystem::value().CollectDrawFrame(Locator::time::value().GetTurnFraction(), _particleFrame);
	}
	if (Locator::explosionSystem::has_value())
	{
		Locator::explosionSystem::value().CollectDrawFrame(_particleFrame);
	}
	// The dust that thrown things throw up as they land
	if (Locator::dynamicsSystem::has_value())
	{
		Locator::dynamicsSystem::value().CollectDrawFrame(_particleFrame);
	}
}

const Texture2D* Renderer::ParticleLightMap(entt::id_type bitmap, int frame) const
{
	const uint64_t key = (static_cast<uint64_t>(bitmap) << 8u) | static_cast<uint64_t>(frame & 0xFF);
	if (const auto found = _particleLightMaps.find(key); found != _particleLightMaps.end())
	{
		return found->second.get();
	}
	auto& bitmaps = Locator::resources::value().GetParticleBitmaps();
	if (!bitmaps.Contains(bitmap))
	{
		return nullptr;
	}
	const auto& stacked = *bitmaps.Handle(bitmap);
	const auto texels = stacked.Frame(frame);
	if ((stacked.channels != 3 && stacked.channels != 1) || texels.empty())
	{
		return nullptr;
	}
	// A shadow map's one byte a texel is spread over red, green and blue
	std::vector<uint8_t> rgb;
	if (stacked.channels == 1)
	{
		rgb.reserve(texels.size() * 3);
		for (const auto texel : texels)
		{
			rgb.insert(rgb.end(), {texel, texel, texel});
		}
	}
	const std::span<const uint8_t> image = stacked.channels == 1 ? std::span<const uint8_t>(rgb) : texels;
	auto texture = std::make_unique<Texture2D>("ParticleLightMap");
	const auto side = static_cast<uint16_t>(stacked.pitch);
	texture->CreateWithinFrame(side, side, 1, TextureFormat::RGB8, Wrapping::ClampEdge, Filter::Nearest,
	                           bgfx::copy(image.data(), static_cast<uint32_t>(image.size())));
	return _particleLightMaps.emplace(key, std::move(texture)).first->second.get();
}

void Renderer::DrawParticleMist(const DrawSceneDesc& desc, const particles::draw::MistDraw& mist, uint32_t depth) const
{
	using ecs::components::Mist;
	const auto& meshes = Locator::resources::value().GetMeshes();
	const auto& textures = Locator::resources::value().GetTextures();
	if (!meshes.Contains(Mist::k_MeshId) || !textures.Contains(Mist::k_TextureId) ||
	    !textures.Contains(Mist::k_AlphaTextureId) || !Locator::terrainSystem::has_value())
	{
		return;
	}
	// The atom's colour in the land's light at full luminosity
	uint32_t colour = mist.argb;
	if (_landLightTable)
	{
		const auto texel = _landLightTable->GetTexels().back();
		const std::array<uint32_t, 3> light {texel & 0xFFu, (texel >> 8u) & 0xFFu, (texel >> 16u) & 0xFFu};
		const auto channel = [&colour](uint32_t shift, uint32_t by) {
			return ((((colour >> shift) & 0xFFu) * by) >> 8u) << shift;
		};
		colour = channel(24, 0xFF) | channel(16, light[0]) | channel(8, light[1]) | channel(0, light[2]);
	}
	const auto alpha = static_cast<float>(colour >> 24u);
	if (alpha <= 0.0f)
	{
		return;
	}
	const auto mesh = meshes.Handle(Mist::k_MeshId);
	const auto* program = _shaderManager->GetShader("Mist");
	const auto& camera = *desc.camera;
	const auto origin = camera.GetOrigin();
	const auto& island = Locator::terrainSystem::value();
	const glm::vec4 islandExtent {island.GetExtent().minimum, island.GetExtent().maximum};
	// It faces the camera and shrinks edge on, lit from straight above as the clouds are
	const glm::mat3 facing {camera.GetRight(), -camera.GetForward(), camera.GetUp()};
	glm::vec3 scale {mist.size};
	scale.y = scale.z = mists::EdgeOnSize(mist.size, mist.shape, mist.position - origin);
	const auto model = glm::translate(mist.position) * glm::mat4(facing) * glm::scale(scale);
	const glm::vec4 u_mist {mists::FrameOffset(mists::Frame(mist.counter), true), 0.0f, 0.0f};
	const glm::vec4 u_mistColour {static_cast<float>((colour >> 16u) & 0xFFu) / 255.0f,
	                              static_cast<float>((colour >> 8u) & 0xFFu) / 255.0f,
	                              static_cast<float>(colour & 0xFFu) / 255.0f, alpha / 255.0f};
	const glm::vec4 skyLight {0.0f, 500000.0f, 0.0f, 210.0f};
	const glm::vec4 u_landLight {0.0f, 1.0f, 0.0f, 0.0f};
	const glm::vec4 noHaze {0.0f};
	const auto texture = textures.Handle(Mist::k_TextureId);
	const auto alphaTexture = textures.Handle(Mist::k_AlphaTextureId);
	const auto viewId = static_cast<bgfx::ViewId>(TranslucentPassOf(desc.viewId));
	for (const auto& subMesh : mesh->GetSubMeshes())
	{
		for (const auto& primitive : subMesh->GetPrimitives())
		{
			bgfx::setTransform(glm::value_ptr(model));
			program->SetTextureSampler("s_diffuse", 0, *texture);
			program->SetTextureSampler("s_alpha", 1, *alphaTexture);
			program->SetTextureSampler("s_landLuminosity", 6, GetLandLuminosity());
			program->SetTextureSampler("s_landLight", 7, GetLandLightTexture());
			program->SetTextureSampler("s_landColour", 8, GetLandColour());
			program->SetUniformValue("u_islandExtent", &islandExtent);
			program->SetUniformValue("u_landLight", &u_landLight);
			program->SetUniformValue("u_haze", &noHaze);
			program->SetUniformValue("u_hazeColour", &_haze[1]);
			program->SetUniformValue("u_modelLight", &skyLight);
			program->SetUniformValue("u_mist", &u_mist);
			program->SetUniformValue("u_mistColour", &u_mistColour);
			if (subMesh->GetMesh().IsIndexed())
			{
				subMesh->GetMesh().GetIndexBuffer().Bind(primitive.indicesCount, primitive.indicesOffset);
			}
			subMesh->GetMesh().GetVertexBuffer().Bind();
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA);
			program->Submit(viewId, depth);
		}
	}
}

void Renderer::DrawParticleMesh(const DrawSceneDesc& desc, const particles::draw::MeshDraw& draw, uint32_t depth) const
{
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(draw.mesh))
	{
		return;
	}
	const auto& creator = static_cast<const particles::MeshCreator&>(*draw.creator);
	const bool reflection = desc.viewId == RenderPass::Reflection;
	const auto axes = creator.faceCamera
	                      ? FacingCamera(draw.axes, draw.position, desc.camera->GetOrigin(), creator.heightStretch)
	                      : draw.axes;
	glm::mat4 model(axes);
	model[3] = glm::vec4(draw.position, 1.0f);
	// A model of several bones has each placed by its own rest matrix, in the particle's frame
	const auto mesh = meshes.Handle(draw.mesh);
	std::vector<glm::mat4> bones;
	if (mesh->IsBoned() && !mesh->GetBoneMatrices().empty())
	{
		bones.reserve(mesh->GetBoneMatrices().size());
		std::ranges::transform(mesh->GetBoneMatrices(), std::back_inserter(bones),
		                       [&model](const glm::mat4& bone) { return model * bone; });
	}

	L3DMeshSubmitDesc submitDesc = {};
	submitDesc.viewId = TranslucentPassOf(desc.viewId);
	submitDesc.program = _shaderManager->GetShader("Object");
	submitDesc.modelMatrices = bones.empty() ? &model : bones.data();
	submitDesc.matrixCount = bones.empty() ? 1 : static_cast<uint8_t>(std::min<size_t>(bones.size(), UINT8_MAX));
	submitDesc.sortDepth = depth;
	submitDesc.uvOffset = draw.uvOffset;
	submitDesc.mirrored = reflection;
	submitDesc.cutBelow = draw.cutBelow;
	if (creator.changeMaterial)
	{
		// The creator's materials in place of the model's own
		const auto mode = creator.additiveMaterial
		                      ? (creator.writeDepthMaterial ? render_modes::Mode::AlphaTexturedAlphaAdditive
		                                                    : render_modes::Mode::AlphaTexturedAlphaAdditiveNz)
		                      : (creator.writeDepthMaterial ? render_modes::Mode::AlphaTexturedAlpha
		                                                    : render_modes::Mode::AlphaTexturedAlphaNz);
		submitDesc.state = BlendState(mode) | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;
		if (!creator.doubleSided)
		{
			// L3D meshes face clockwise; the mirrored reflection sees them from the other side
			submitDesc.state |= reflection ? BGFX_STATE_CULL_CW : BGFX_STATE_CULL_CCW;
		}
	}
	else
	{
		submitDesc.state =
		    BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_MSAA;
		submitDesc.useMaterialBlending = true;
		submitDesc.useMaterialCulling = true;
	}
	// Those drawn in their atom's alpha, or added to what is behind, take their atom's colour unlit; the others are lit
	const bool unlit = creator.useGlobalAlpha || creator.additiveMaterial;
	submitDesc.tint = glm::vec4(glm::vec3(draw.colour), unlit ? std::max(draw.colour.a, 1.0f / 255.0f) : 0.0f);
	DrawMesh(*mesh, submitDesc, std::numeric_limits<uint8_t>::max());
}

void Renderer::DrawParticleFragment(const DrawSceneDesc& desc, const particles::draw::FragmentDraw& fragment,
                                    uint32_t depth) const
{
	const auto& shape = *fragment.shape;
	const auto& meshes = Locator::resources::value().GetMeshes();
	const auto& textures = Locator::resources::value().GetTextures();
	if (!meshes.Contains(shape.mesh) || shape.positions.empty())
	{
		return;
	}
	const auto mesh = meshes.Handle(shape.mesh);
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		/// x: the model light's factor at the corner, of 256
		glm::vec2 shade;
		uint32_t colour;
	};
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord3, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	const auto alpha = static_cast<uint32_t>(std::clamp(fragment.alpha, 0.0f, 1.0f) * 255.0f);
	const uint32_t colour = (alpha << 24u) | (static_cast<uint32_t>(fragment.rgb[2]) << 16u) |
	                        (static_cast<uint32_t>(fragment.rgb[1]) << 8u) | fragment.rgb[0];
	// The model light shades each corner by its normal, as the model has it, against the light's way in the piece's
	// own frame: from the piece's middle, as the game takes it
	const auto light = GetModelLight();
	const auto localLight =
	    model_light::LocalDirection(glm::vec3(light), fragment.axes[0], fragment.axes[1], fragment.axes[2], fragment.position);
	const auto* program = _shaderManager->GetShader("Fragment");
	const auto& island = Locator::terrainSystem::value();
	const glm::vec4 islandExtent {island.GetExtent().minimum, island.GetExtent().maximum};
	const glm::vec4 origin {fragment.position, 0.0f};
	const auto viewId = static_cast<bgfx::ViewId>(TranslucentPassOf(desc.viewId));
	// A draw for each of its skins, its triangles placed by its atom
	const size_t triangles = shape.skins.size();
	std::vector<bool> drawn(triangles, false);
	for (size_t first = 0; first < triangles; ++first)
	{
		if (drawn[first])
		{
			continue;
		}
		const auto skin = shape.skins[first];
		std::vector<Vertex> vertices;
		for (size_t t = first; t < triangles; ++t)
		{
			if (drawn[t] || shape.skins[t] != skin)
			{
				continue;
			}
			drawn[t] = true;
			for (size_t c = 0; c < 3; ++c)
			{
				const size_t corner = t * 3 + c;
				const auto normal = corner < shape.normals.size() ? shape.normals[corner] : glm::vec3(0.0f);
				vertices.push_back({.position = fragment.position + fragment.axes * shape.positions[corner],
				                    .uv = shape.uvs[corner],
				                    .shade = {model_light::Factor(normal, localLight, light.w), 0.0f},
				                    .colour = colour});
			}
		}
		const Texture2D* texture = nullptr;
		if (const auto found = mesh->GetSkins().find(skin); found != mesh->GetSkins().end())
		{
			texture = found->second.get();
		}
		else if (const auto* shared = textures.Find(skin))
		{
			texture = shared;
		}
		const auto count = static_cast<uint32_t>(vertices.size());
		if (texture == nullptr || count == 0 || bgfx::getAvailTransientVertexBuffer(count, layout) < count)
		{
			continue;
		}
		bgfx::TransientVertexBuffer buffer;
		bgfx::allocTransientVertexBuffer(&buffer, count, layout);
		std::memcpy(buffer.data, vertices.data(), count * sizeof(Vertex));
		program->SetTextureSampler("s_diffuse", 0, *texture);
		program->SetTextureSampler("s_landLuminosity", 6, GetLandLuminosity());
		program->SetTextureSampler("s_landLight", 7, GetLandLightTexture());
		program->SetTextureSampler("s_landColour", 8, GetLandColour());
		program->SetUniformValue("u_islandExtent", &islandExtent);
		program->SetUniformValue("u_fragmentOrigin", &origin);
		bgfx::setVertexBuffer(0, &buffer);
		// Drawn from both sides, as a broken piece shows its inside
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_BLEND_ALPHA |
		               BGFX_STATE_MSAA);
		program->Submit(viewId, depth);
	}
}

void Renderer::DrawParticles(const DrawSceneDesc& desc) const
{
	const bool reflection = desc.viewId == RenderPass::Reflection;
	if ((desc.viewId != RenderPass::Main && !reflection) || _particleFrame.items.empty() ||
	    (Locator::temple::has_value() && Locator::temple::value().Active()))
	{
		return;
	}
	auto section = Locator::profiler::value().BeginScoped(reflection ? Profiler::Stage::ReflectionDrawParticles
	                                                                 : Profiler::Stage::MainPassDrawParticles);
	const auto origin = desc.camera->GetOrigin();
	particles::draw::Order(_particleFrame, origin, reflection ? std::nullopt : HandPoint(), _particleCommands,
	                       _particleSpriteOrder);
	const auto& frame = _particleFrame;
	const auto& textures = Locator::resources::value().GetTextures();
	const auto viewId = static_cast<bgfx::ViewId>(TranslucentPassOf(desc.viewId));

	// Every sprite of the pass in one buffer of instances, in the order drawn
	bgfx::InstanceDataBuffer instances {};
	constexpr auto k_Stride = static_cast<uint16_t>(sizeof(particles::sprites::SpriteInstance));
	const auto spriteCount = static_cast<uint32_t>(_particleSpriteOrder.size());
	const bool haveSprites = spriteCount > 0 && bgfx::getAvailInstanceDataBuffer(spriteCount, k_Stride) >= spriteCount;
	if (haveSprites)
	{
		bgfx::allocInstanceDataBuffer(&instances, spriteCount, k_Stride);
		auto* out = reinterpret_cast<particles::sprites::SpriteInstance*>(instances.data);
		for (const auto index : _particleSpriteOrder)
		{
			*out++ = frame.sprites[index];
		}
	}

	// Every ribbon's corners in one buffer, two triangles to a segment
	bgfx::TransientVertexBuffer chainVertices {};
	bgfx::TransientIndexBuffer chainIndices {};
	bool haveChains = false;
	if (!frame.chains.empty())
	{
		const auto layout = ChainLayout();
		const auto vertexCount = static_cast<uint32_t>(frame.chainVertices.size());
		const uint32_t indexCount = vertexCount / k_CornersPerSegment * static_cast<uint32_t>(k_SegmentIndices.size());
		haveChains = bgfx::getAvailTransientVertexBuffer(vertexCount, layout) >= vertexCount &&
		             bgfx::getAvailTransientIndexBuffer(indexCount, true) >= indexCount;
		if (haveChains)
		{
			bgfx::allocTransientVertexBuffer(&chainVertices, vertexCount, layout);
			std::memcpy(chainVertices.data, frame.chainVertices.data(), vertexCount * sizeof(particles::draw::ChainVertex));
			bgfx::allocTransientIndexBuffer(&chainIndices, indexCount, true);
			auto* index = reinterpret_cast<uint32_t*>(chainIndices.data);
			for (uint32_t corner = 0; corner < vertexCount; corner += k_CornersPerSegment)
			{
				for (const auto offset : k_SegmentIndices)
				{
					*index++ = corner + offset;
				}
			}
		}
	}

	glm::vec4 islandExtent(0.0f);
	if (Locator::terrainSystem::has_value())
	{
		const auto extent = Locator::terrainSystem::value().GetExtent();
		islandExtent = glm::vec4(extent.minimum, extent.maximum);
	}
	// Every surface's points in one buffer and their triangles in another. The lit ones are lit as the game lights its
	// models: each point by its normal against the light, seen from the surface's own frame.
	bgfx::TransientVertexBuffer surfaceVertices {};
	bgfx::TransientIndexBuffer surfaceIndices {};
	bool haveSurfaces = false;
	if (!frame.surfaces.empty())
	{
		bgfx::VertexLayout layout;
		layout.begin()
		    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
		    .add(bgfx::Attrib::Color1, 4, bgfx::AttribType::Uint8, true)
		    .end();
		const auto vertexCount = static_cast<uint32_t>(frame.surfaceVertices.size());
		const auto indexCount = static_cast<uint32_t>(frame.surfaceIndices.size());
		haveSurfaces = bgfx::getAvailTransientVertexBuffer(vertexCount, layout) >= vertexCount &&
		               bgfx::getAvailTransientIndexBuffer(indexCount, true) >= indexCount;
		if (haveSurfaces)
		{
			bgfx::allocTransientVertexBuffer(&surfaceVertices, vertexCount, layout);
			auto* out = reinterpret_cast<particles::draw::SurfaceVertex*>(surfaceVertices.data);
			std::memcpy(out, frame.surfaceVertices.data(), vertexCount * sizeof(particles::draw::SurfaceVertex));
			const glm::vec3 light(_modelLight);
			const auto ambient = static_cast<uint32_t>(_modelLight.w);
			for (const auto& surface : frame.surfaces)
			{
				if (!surface.lit)
				{
					continue;
				}
				const auto towards = glm::vec3(surface.worldToAtom * glm::vec4(light, 1.0f));
				const auto towardsLight = glm::length(towards) > 0.0f ? glm::normalize(towards) : towards;
				for (uint32_t i = surface.firstVertex; i < surface.firstVertex + surface.vertexCount; ++i)
				{
					const auto level = particles::surface::LightLevel(frame.surfaceNormals[i], towardsLight, ambient);
					auto& abgr = out[i].abgr;
					const auto channel = [&](uint32_t shift) { return ((((abgr >> shift) & 0xFFu) * level) >> 8u) & 0xFFu; };
					abgr = (abgr & 0xFF000000u) | (channel(16) << 16u) | (channel(8) << 8u) | channel(0);
				}
			}
			bgfx::allocTransientIndexBuffer(&surfaceIndices, indexCount, true);
			std::memcpy(surfaceIndices.data, frame.surfaceIndices.data(), indexCount * sizeof(uint32_t));
		}
	}

	const auto* spriteProgram = _shaderManager->GetShader("ParticleInstanced");
	const auto* chainProgram = _shaderManager->GetShader("ParticleChain");
	const auto bindMaterial = [&](const ShaderProgram& program, uint32_t index) {
		const auto& material = frame.materials[index];
		if (!textures.Contains(material.texture))
		{
			return false;
		}
		const auto texture = textures.Handle(material.texture);
		// A sheet without an alpha of its own takes its alpha from its red
		const auto alpha = textures.Contains(material.alphaTexture) ? textures.Handle(material.alphaTexture) : texture;
		program.SetTextureSampler("s_diffuse", 0, *texture);
		program.SetTextureSampler("s_alpha", 1, *alpha);
		bgfx::setState(BlendState(material.mode));
		return true;
	};
	for (const auto& command : _particleCommands)
	{
		switch (command.kind)
		{
		case ItemKind::Sprite:
			if (haveSprites && bindMaterial(*spriteProgram, command.material))
			{
				// The sprites tinted by the land read its light
				spriteProgram->SetTextureSampler("s_landLuminosity", 6, GetLandLuminosity());
				spriteProgram->SetTextureSampler("s_landLight", 7, GetLandLightTexture());
				spriteProgram->SetUniformValue("u_islandExtent", &islandExtent);
				_plane->GetVertexBuffer().Bind();
				bgfx::setInstanceDataBuffer(&instances, command.first, command.count);
				spriteProgram->Submit(viewId, command.depth);
			}
			break;
		case ItemKind::Chain:
		{
			const auto& chain = frame.chains[command.first];
			if (haveChains && bindMaterial(*chainProgram, command.material))
			{
				bgfx::setVertexBuffer(0, &chainVertices);
				bgfx::setIndexBuffer(&chainIndices,
				                     chain.firstVertex / k_CornersPerSegment * static_cast<uint32_t>(k_SegmentIndices.size()),
				                     chain.segments * static_cast<uint32_t>(k_SegmentIndices.size()));
				chainProgram->Submit(viewId, command.depth);
			}
			break;
		}
		case ItemKind::Mesh:
			DrawParticleMesh(desc, frame.meshes[command.first], command.depth);
			break;
		case ItemKind::Mist:
			DrawParticleMist(desc, frame.mists[command.first], command.depth);
			break;
		case ItemKind::Surface:
		{
			const auto& surface = frame.surfaces[command.first];
			const auto* surfaceProgram = _shaderManager->GetShader("ParticleSurface");
			if (haveSurfaces && bindMaterial(*surfaceProgram, command.material))
			{
				// Its points are in the world; the game's surfaces face clockwise, which the mirrored reflection turns
				bgfx::setTransform(glm::value_ptr(glm::mat4(1.0f)));
				if (!surface.doubleSided)
				{
					bgfx::setState(BlendState(frame.materials[command.material].mode) |
					               (reflection ? BGFX_STATE_CULL_CW : BGFX_STATE_CULL_CCW));
				}
				bgfx::setVertexBuffer(0, &surfaceVertices, surface.firstVertex, surface.vertexCount);
				bgfx::setIndexBuffer(&surfaceIndices, surface.firstIndex, surface.indexCount);
				surfaceProgram->Submit(viewId, command.depth);
			}
			break;
		}
		case ItemKind::Fragment:
			DrawParticleFragment(desc, frame.fragments[command.first], command.depth);
			break;
		}
	}
}
