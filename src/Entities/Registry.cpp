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

void Registry::DrawModels(uint8_t viewId, graphics::ShaderManager &shaderManager, graphics::DebugLines* boundingBox) const
{
	graphics::ShaderProgram* debugShader = shaderManager.GetShader("DebugLine");
	graphics::ShaderProgram* debugShaderInstanced = shaderManager.GetShader("DebugLineInstanced");
	graphics::ShaderProgram* objectShader = shaderManager.GetShader("Object");
	graphics::ShaderProgram* objectShaderInstanced = shaderManager.GetShader("ObjectInstanced");

	uint64_t state = 0u
		| BGFX_STATE_WRITE_MASK
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_MSAA
	;

	_registry.view<const Tree, const Transform>().each([viewId, debugShader, objectShader, state, &boundingBox](const Tree& tree, const Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		auto meshId           = treeMeshLookup[tree.treeInfo];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));

		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[mesh.GetNumSubMeshes() - 1]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const Stream>().each([viewId, debugShader](const Stream& stream) {
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

	_registry.view<const Abode, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const Abode& abode, const Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);
		auto abodeId          = abode.abodeInfo;

		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(abodeMeshLookup[abodeId]));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[0]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const AnimatedStatic, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const AnimatedStatic& animated, const Transform& transform) {
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

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[0]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const MobileStatic, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const MobileStatic& mobile, const Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		auto meshId         = mobileStaticMeshLookup[mobile.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[1]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const Feature, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const Feature& feature, const Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		auto meshId         = featureMeshLookup[feature.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[1]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const Field, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const Field& feature, const Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		auto meshId         = MeshId::TreeWheat;
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[0]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const Forest, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const Forest& forest, const Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		auto meshId         = MeshId::FeatureForest;
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[0]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const MobileObject, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const MobileObject& mobileObject, const Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		auto meshId         = mobileObjectMeshLookup[mobileObject.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[1]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});
}
} // namespace openblack::Entities
