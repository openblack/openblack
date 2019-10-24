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
#include <spdlog/spdlog.h>

namespace openblack::Entities
{

void Registry::DrawModels(uint8_t viewId, graphics::ShaderManager &shaderManager)
{
	graphics::ShaderProgram* debugShader            = shaderManager.GetShader("DebugLine");
	graphics::ShaderProgram* objectShader = shaderManager.GetShader("Object");

	uint64_t state = 0u
		| BGFX_STATE_WRITE_MASK
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_MSAA
	;

	_registry.view<Tree, Transform>().each([viewId, objectShader, state](Tree& tree, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		auto meshId           = treeMeshLookup[tree.treeInfo];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));

		mesh.Submit(viewId, modelMatrix, *objectShader, state);
	});

	_registry.view<Stream>().each([viewId, debugShader](Stream& stream) {
		auto nodes       = stream.streamNodes;
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

	_registry.view<Abode, Transform>().each([viewId, state, objectShader](Abode& abode, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);
		auto abodeId          = abode.abodeInfo;

		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(abodeMeshLookup[abodeId]));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);
	});

	_registry.view<AnimatedStatic, Transform>().each([viewId, state, objectShader](AnimatedStatic& animated, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		// temporary-ish:
		MeshId meshId = MeshId::Dummy;
		if (animated.type == "Norse Gate")
		{
			meshId = MeshId::BuildingNorseGate;
		}
		else if (animated.type == "Gate Stone Plinth")
		{
			meshId = MeshId::ObjectGateTotemPlinthe;
		}
		else if (animated.type == "Piper Cave Entrance")
		{
			meshId = MeshId::BuildingMineEntrance;
		}

		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<int>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);
	});

	_registry.view<MobileStatic, Transform>().each([viewId, state, objectShader](MobileStatic& mobile, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		auto meshId         = mobileStaticMeshLookup[mobile.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));

		mesh.Submit(viewId, modelMatrix, *objectShader, state);
	});

	_registry.view<Feature, Transform>().each([viewId, state, objectShader](Feature& feature, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		auto meshId         = featureMeshLookup[feature.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);
	});

	_registry.view<Field, Transform>().each([viewId, state, objectShader](Field& feature, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		auto meshId         = MeshId::TreeWheat;
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);
	});

	_registry.view<Forest, Transform>().each([viewId, state, objectShader](Forest& forest, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		auto meshId         = MeshId::FeatureForest;
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);
	});

	_registry.view<MobileObject, Transform>().each([viewId, state, objectShader](MobileObject& mobileObject, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		auto meshId         = mobileObjectMeshLookup[mobileObject.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);
	});
}
} // namespace openblack::Entities
