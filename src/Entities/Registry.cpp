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
#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace openblack::Entities
{

void Registry::DrawModels(uint8_t viewId, graphics::ShaderManager &shaderManager)
{
	graphics::ShaderProgram* objectShader = shaderManager.GetShader("SkinnedMesh");
	graphics::ShaderProgram* debugShader            = shaderManager.GetShader("DebugLine");
	graphics::ShaderProgram* objectShader = shaderManager.GetShader("Object");

	uint64_t state = 0u
		| BGFX_STATE_WRITE_MASK
		| BGFX_STATE_DEPTH_TEST_LESS
		// | BGFX_STATE_CULL_CCW  // TODO(bwrsandman): Some meshes wind one way and some others (i.e. rocks, gate)
		| BGFX_STATE_MSAA
		| BGFX_STATE_BLEND_ALPHA
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

		bgfx::setTransform(&modelMatrix);

		mesh.Draw(viewId, *objectShader, 2, state);
		mesh.Draw(viewId, *objectShader, 3, state);
	});

	_registry.view<Stream>().each([debugShader](Stream& stream) {
		auto nodes       = stream.streamNodes;
		const auto color = glm::vec3(1, 0, 0);

		for (const auto& from : nodes)
		{
			for (const auto& to : from.edges)
			{
				auto startPos = from.position;
				auto startOffset = glm::vec3(0, 0, 0);
				auto endOffset = to.position - startPos;
				auto line = DebugLines::CreateLine(startOffset, endOffset, color);
				line->SetPose(startPos, 1);
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_BLEND);
				line->Draw(*debugShader);
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_BLEND);
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

		bgfx::setTransform(&modelMatrix);

		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(abodeMeshLookup[abodeId]));

		std::vector<int> submeshIds = { 2 };

		// Temporary
		switch (abodeId)
		{
		case AbodeInfo::AfricanTownCentre:
		case AbodeInfo::AztecTownCentre:
		case AbodeInfo::CelticTownCentre:
		case AbodeInfo::EgyptianTownCentre:
		case AbodeInfo::AfricanCreche:
		case AbodeInfo::GreekTownCentre:
		case AbodeInfo::NorseTownCentre:
		case AbodeInfo::JapaneseTownCentre:
		case AbodeInfo::TibetanTownCentre:
			submeshIds = { 0, 3 };
			break;
		case AbodeInfo::IndianTownCentre:
			submeshIds = { 1, 2 };
		}

		for (auto& submeshId : submeshIds)
		{
			mesh.Draw(viewId, *objectShader, submeshId, state);
		}
	});

	_registry.view<AnimatedStatic, Transform>().each([viewId, state, objectShader](AnimatedStatic& animated, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		bgfx::setTransform(&modelMatrix);

		// temporary-ish:
		if (animated.type == "Norse Gate")
		{
			const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh((int)MeshId::BuildingNorseGate);
			mesh.Draw(viewId, *objectShader, 0, state);
		}
		else if (animated.type == "Gate Stone Plinth")
		{
			const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh((int)MeshId::ObjectGateTotemPlinthe);
			mesh.Draw(viewId, *objectShader, 0, state);
		}
		else if (animated.type == "Piper Cave Entrance")
		{
			const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh((int)MeshId::BuildingMineEntrance);
			mesh.Draw(viewId, *objectShader, 0, state);
		}
		else
		{
			const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(0);
			mesh.Draw(viewId, *objectShader, 0, state);
		}
	});

	_registry.view<MobileStatic, Transform>().each([viewId, state, objectShader](MobileStatic& mobile, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		bgfx::setTransform(&modelMatrix);

		auto meshId = mobileStaticMeshLookup[mobile.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Draw(viewId, *objectShader, 1, state);
	});

	_registry.view<Feature, Transform>().each([viewId, state, objectShader](Feature& feature, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		bgfx::setTransform(&modelMatrix);

		auto meshId         = featureMeshLookup[feature.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Draw(viewId, *objectShader, 1, state);
	});

	_registry.view<Field, Transform>().each([viewId, state, objectShader](Field& feature, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		bgfx::setTransform(&modelMatrix);

		auto meshId         = MeshId::TreeWheat;
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Draw(viewId, *objectShader, 0, state);
	});

	_registry.view<Forest, Transform>().each([viewId, state, objectShader](Forest& forest, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		bgfx::setTransform(&modelMatrix);

		auto meshId         = MeshId::FeatureForest;
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Draw(viewId, *objectShader, 1, state);
	});

	_registry.view<MobileObject, Transform>().each([viewId, state, objectShader](MobileObject& mobileObject, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		bgfx::setTransform(&modelMatrix);

		auto meshId         = mobileObjectMeshLookup[mobileObject.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Draw(viewId, *objectShader, 1, state);
	});
}
} // namespace openblack::Entities
