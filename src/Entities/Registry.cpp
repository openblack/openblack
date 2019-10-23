#include "Registry.h"

#include <3D/Camera.h>
#include <3D/L3DMesh.h>
#include <3D/MeshLookup.h>
#include <3D/MeshPack.h>
#include <AllMeshes.h>
#include <Entities/Components/Abode.h>
#include <Entities/Components/AnimatedStatic.h>
#include <Entities/Components/Stream.h>
#include <Entities/Components/Transform.h>
#include <Entities/Components/Tree.h>
#include <Game.h>
#include <Graphics/DebugLines.h>
#include <Graphics/ShaderManager.h>

namespace openblack::Entities
{

Registry::Registry()
	: _instanceUniformBuffer(BGFX_INVALID_HANDLE)
	, _dirty(true)
	, _hasBoundingBoxes(false)
{
	_registry.set<RegistryContext>();
}

Registry::~Registry()
{
	if (bgfx::isValid(_instanceUniformBuffer))
		bgfx::destroy(_instanceUniformBuffer);
}

void Registry::PrepareDrawDescs(bool drawBoundingBox)
{
	// Count number of instances
	uint32_t instanceCount = 0;
	std::map<MeshId, uint32_t> meshIds;

	// Trees
	_registry.view<const Tree, const Transform>().each([&meshIds, &instanceCount](const Tree& entity, const Transform& transform) {
		const auto meshId = treeMeshLookup[entity.treeInfo];
		auto count = meshIds.insert(std::make_pair(meshId, 0));
		count.first->second++;
		instanceCount++;
	});

	// Abodes
	_registry.view<const Abode, const Transform>().each([&meshIds, &instanceCount](const Abode& entity, const Transform& transform) {
		const auto meshId = abodeMeshLookup[entity.abodeInfo];
		auto count = meshIds.insert(std::make_pair(meshId, 0));
		count.first->second++;
		instanceCount++;
	});

	// Animated Statics
	_registry.view<const AnimatedStatic, const Transform>().each([&meshIds, &instanceCount](const AnimatedStatic& entity, const Transform& transform) {
		// temporary-ish:
		MeshId meshId = MeshId::Dummy;
		if (entity.type == "Norse Gate")
		{
			meshId = MeshId::BuildingNorseGate;
		}
		else if (entity.type == "Gate Stone Plinth")
		{
			meshId = MeshId::ObjectGateTotemPlinthe;
		}
		else if (entity.type == "Piper Cave Entrance")
		{
			meshId = MeshId::BuildingMineEntrance;
		}
		auto count = meshIds.insert(std::make_pair(meshId, 0));
		count.first->second++;
		instanceCount++;
	});

	// Mobile Statics
	_registry.view<const MobileStatic, const Transform>().each([&meshIds, &instanceCount](const MobileStatic& entity, const Transform& transform) {
		const auto meshId = mobileStaticMeshLookup[entity.type];
		auto count = meshIds.insert(std::make_pair(meshId, 0));
		count.first->second++;
		instanceCount++;
	});

	// Features
	_registry.view<const Feature, const Transform>().each([&meshIds, &instanceCount](const Feature& entity, const Transform& transform) {
		const auto meshId = featureMeshLookup[entity.type];
		auto count = meshIds.insert(std::make_pair(meshId, 0));
		count.first->second++;
		instanceCount++;
	});

	// Fields
	_registry.view<const Field, const Transform>().each([&meshIds, &instanceCount](const Field& entity, const Transform& transform) {
		const auto meshId = MeshId::TreeWheat;
		auto count = meshIds.insert(std::make_pair(meshId, 0));
		count.first->second++;
		instanceCount++;
	});

	// Forests
	_registry.view<const Forest, const Transform>().each([&meshIds, &instanceCount](const Forest& entity, const Transform& transform) {
		const auto meshId = MeshId::FeatureForest;
		auto count = meshIds.insert(std::make_pair(meshId, 0));
		count.first->second++;
		instanceCount++;
	});

	// Mobile Objects
	_registry.view<const MobileObject, const Transform>().each([&meshIds, &instanceCount](const MobileObject& entity, const Transform& transform) {
		const auto meshId = mobileObjectMeshLookup[entity.type];
		auto count = meshIds.insert(std::make_pair(meshId, 0));
		count.first->second++;
		instanceCount++;
	});

	if (drawBoundingBox)
	{
		instanceCount *= 2;
	}

	// Recreate instancing uniform buffer if it is too small
	if (_instanceUniforms.size() < instanceCount)
	{
		if (bgfx::isValid(_instanceUniformBuffer))
		{
			bgfx::destroy(_instanceUniformBuffer);
		}
		bgfx::VertexLayout layout;
		layout.begin()
		      .add(bgfx::Attrib::TexCoord7, 4, bgfx::AttribType::Float)
		      .add(bgfx::Attrib::TexCoord6, 4, bgfx::AttribType::Float)
		      .add(bgfx::Attrib::TexCoord5, 4, bgfx::AttribType::Float)
		      .add(bgfx::Attrib::TexCoord4, 4, bgfx::AttribType::Float)
		      .end();
		_instanceUniformBuffer = bgfx::createDynamicVertexBuffer(instanceCount, layout);
		_instanceUniforms.resize(instanceCount);
	}

	// Determine uniform buffer offsets and instance count for draw
	uint32_t offset = 0;
	_instancedDrawDescs.clear();
	for (const auto& [meshId, count] : meshIds)
	{
		_instancedDrawDescs.emplace(std::piecewise_construct, std::forward_as_tuple(meshId), std::forward_as_tuple(offset, count));
		offset += count;
	}

}

void Registry::PrepareDrawUploadUniforms(bool drawBoundingBox)
{
	// Set transforms for instanced draw at offsets
	auto prepareDrawBoundingBox = [drawBoundingBox, this](uint32_t idx, const Transform& transform, MeshId meshId, int8_t submeshId) {
		if (drawBoundingBox)
		{
			const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
			auto box = mesh.GetSubMeshes()[(mesh.GetNumSubMeshes() + submeshId) % mesh.GetNumSubMeshes()]->GetBoundingBox();

			glm::mat4 boxMatrix = glm::mat4(1.0f);
			boxMatrix           = glm::translate(boxMatrix, transform.position + box.center());
//			boxMatrix           = glm::rotate(boxMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
//			boxMatrix           = glm::rotate(boxMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
//			boxMatrix           = glm::rotate(boxMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
			boxMatrix           = glm::scale(boxMatrix, transform.scale * box.size());

			_instanceUniforms[idx + _instanceUniforms.size() / 2] = boxMatrix;
		}
	};
	// Store offsets of uniforms for descs
	std::map<MeshId, uint32_t> uniformOffsets;

	// Trees
	_registry.view<const Tree, const Transform>().each([this, &uniformOffsets, prepareDrawBoundingBox](const Tree& entity, const Transform& transform) {
		const auto meshId = treeMeshLookup[entity.treeInfo];
		auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		auto desc = _instancedDrawDescs.find(meshId);
		_instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, -1);
		offset.first->second++;
	});

	// Abodes
	_registry.view<const Abode, const Transform>().each([this, &uniformOffsets, prepareDrawBoundingBox](const Abode& entity, const Transform& transform) {
		const auto meshId = abodeMeshLookup[entity.abodeInfo];
		auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		auto desc = _instancedDrawDescs.find(meshId);
		_instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 0);
		offset.first->second++;
	});

	// Animated Statics
	_registry.view<const AnimatedStatic, const Transform>().each([this, &uniformOffsets, prepareDrawBoundingBox](const AnimatedStatic& entity, const Transform& transform) {
		// temporary-ish:
		MeshId meshId = MeshId::Dummy;
		if (entity.type == "Norse Gate")
		{
			meshId = MeshId::BuildingNorseGate;
		}
		else if (entity.type == "Gate Stone Plinth")
		{
			meshId = MeshId::ObjectGateTotemPlinthe;
		}
		else if (entity.type == "Piper Cave Entrance")
		{
			meshId = MeshId::BuildingMineEntrance;
		}
		auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		auto desc = _instancedDrawDescs.find(meshId);
		_instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 0);
		offset.first->second++;
	});

	// Mobile Statics
	_registry.view<const MobileStatic, const Transform>().each([this, &uniformOffsets, prepareDrawBoundingBox](const MobileStatic& entity, const Transform& transform) {
		const auto meshId = mobileStaticMeshLookup[entity.type];
		auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		auto desc = _instancedDrawDescs.find(meshId);
		_instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 1);
		offset.first->second++;
	});

	// Features
	_registry.view<const Feature, const Transform>().each([this, &uniformOffsets, prepareDrawBoundingBox](const Feature& entity, const Transform& transform) {
		const auto meshId = featureMeshLookup[entity.type];
		auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		auto desc = _instancedDrawDescs.find(meshId);
		_instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 1);
		offset.first->second++;
	});

	// Fields
	_registry.view<const Field, const Transform>().each([this, &uniformOffsets, prepareDrawBoundingBox](const Field& entity, const Transform& transform) {
		const auto meshId = MeshId::TreeWheat;
		auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		auto desc = _instancedDrawDescs.find(meshId);
		_instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 0);
		offset.first->second++;
	});

	// Forests
	_registry.view<const Forest, const Transform>().each([this, &uniformOffsets, prepareDrawBoundingBox](const Forest& entity, const Transform& transform) {
		const auto meshId = MeshId::FeatureForest;
		auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		auto desc = _instancedDrawDescs.find(meshId);
		_instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 0);
		offset.first->second++;
	});

	// Mobile Objects
	_registry.view<const MobileObject, const Transform>().each([this, &uniformOffsets, prepareDrawBoundingBox](const MobileObject& entity, const Transform& transform) {
		const auto meshId = mobileObjectMeshLookup[entity.type];
		auto offset = uniformOffsets.insert(std::make_pair(meshId, 0));
		auto desc = _instancedDrawDescs.find(meshId);
		_instanceUniforms[desc->second.offset + offset.first->second] = static_cast<glm::mat4>(transform);
		prepareDrawBoundingBox(desc->second.offset + offset.first->second, transform, meshId, 1);
		offset.first->second++;
	});

	if (!_instanceUniforms.empty())
	{
		bgfx::update(_instanceUniformBuffer, 0, bgfx::makeRef(_instanceUniforms.data(), _instanceUniforms.size()*sizeof(glm::mat4)));
	}
}

void Registry::PrepareDraw(bool drawBoundingBox)
{
	if (_dirty || _hasBoundingBoxes != drawBoundingBox)
	{
		PrepareDrawDescs(drawBoundingBox);
		PrepareDrawUploadUniforms(drawBoundingBox);

		_dirty = false;
		_hasBoundingBoxes = drawBoundingBox;
	}
}

void Registry::DrawModels(uint8_t viewId, graphics::ShaderManager &shaderManager, graphics::DebugLines* boundingBox) const
{
	graphics::ShaderProgram* debugShader = shaderManager.GetShader("DebugLine");
	graphics::ShaderProgram* debugShaderInstanced = shaderManager.GetShader("DebugLineInstanced");
	graphics::ShaderProgram* objectShaderInstanced = shaderManager.GetShader("ObjectInstanced");

	uint64_t state = 0u
		| BGFX_STATE_WRITE_MASK
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_MSAA
	;

	for (const auto& [meshId, placers]: _instancedDrawDescs)
	{
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, _instanceUniformBuffer, placers.offset, placers.count, *objectShaderInstanced, state);
	}

	if (boundingBox)
	{
		auto boundBoxOffset = _instanceUniforms.size() / 2;
		auto boundBoxCount  = _instanceUniforms.size() / 2;
		boundingBox->Draw(viewId, _instanceUniformBuffer, boundBoxOffset, boundBoxCount, *debugShaderInstanced);
	}

	// FIXME(bwrsandman): Add unique_ptr<DebugLine> _streamLine on class
	//                    Move CreateLine to PrepareDraw when null and keep result
	//                    Don't use offset, just create a line from 0, 1 in x
	//                    Build matrix for position, rotation and scale
	//                    Add to instanced drawing desc in PrepareDraw and draw one line instanced
	_registry.view<const Stream>().each([viewId, debugShader](const Stream& entity) {
		auto nodes = entity.streamNodes;
		const auto color = glm::vec4(1, 0, 0, 1);

		for (const auto& from : nodes)
		{
			for (const auto& to : from.edges)
			{
				auto startPos = from.position;
				auto startOffset = glm::vec4(0, 0, 0, 0.0f);
				auto endOffset = glm::vec4(to.position - startPos, 0.0f);
				// auto line = DebugLines::CreateLine(startOffset, endOffset, color);
				// line->SetPose(startPos, 1);
				// line->Draw(viewId, *debugShader);
			}
		}
	});
}
} // namespace openblack::Entities
