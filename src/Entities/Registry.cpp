#include "Registry.h"

#include <3D/Camera.h>
#include <3D/L3DMesh.h>
#include <3D/MeshLookup.h>
#include <3D/MeshPack.h>
#include <AllMeshes.h>
#include <Entities/Components/Abode.h>
#include <Entities/Components/AnimatedStatic.h>
#include <Entities/Components/Transform.h>
#include <Entities/Components/Tree.h>
#include <Game.h>
#include <Graphics/ShaderManager.h>
#include <spdlog/spdlog.h>

namespace openblack::Entities
{

void Registry::DrawModels(graphics::ShaderManager& shaderManager)
{
	graphics::ShaderProgram* objectShader = shaderManager.GetShader("Object");

	_registry.view<Tree, Transform>().each([objectShader](Tree& tree, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);
		auto meshId           = treeMeshLookup[tree.treeInfo];
		objectShader->SetUniformValue("u_model", modelMatrix);
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));

//		objectShader->SetUniformValue("u_model", modelMatrix);
		bgfx::setTransform(&modelMatrix);

		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(model.meshId));

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
			mesh.Draw(viewId, *objectShader, submeshId, state);
	});

	_registry.view<AnimatedStatic, Transform>().each([objectShader](AnimatedStatic& animated, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

//		objectShader->SetUniformValue("u_model", modelMatrix);
		bgfx::setTransform(&modelMatrix);

		// temporary-ish:
		if (animated.type == "Norse Gate")
		{
			const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh((int)MeshId::BuildingNorseGate);
			mesh.Draw(*objectShader, 0);
		}
		else if (animated.type == "Gate Stone Plinth")
		{
			const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh((int)MeshId::ObjectGateTotemPlinthe);
			mesh.Draw(*objectShader, 0);
		}
		else if (animated.type == "Piper Cave Entrance")
		{
			const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh((int)MeshId::BuildingMineEntrance);
			mesh.Draw(*objectShader, 0);
		}
		else
		{
			const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(0);
			mesh.Draw(*objectShader, 0);
		}
	});

	_registry.view<MobileStatic, Transform>().each([objectShader](MobileStatic& mobile, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);
		objectShader->SetUniformValue("u_model", modelMatrix);

//		objectShader->SetUniformValue("u_model", modelMatrix);
		bgfx::setTransform(&modelMatrix);

		auto meshId = mobileStaticMeshLookup[mobile.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Draw(*objectShader, 1);
	});

	_registry.view<Feature, Transform>().each([objectShader](Feature& feature, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);
		objectShader->SetUniformValue("u_model", modelMatrix);

		auto meshId         = featureMeshLookup[feature.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Draw(*objectShader, 1);
	});

	_registry.view<Field, Transform>().each([objectShader](Field& feature, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);
		objectShader->SetUniformValue("u_model", modelMatrix);


		auto meshId         = MeshId::TreeWheat;
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Draw(*objectShader, 0);
	});

	_registry.view<Forest, Transform>().each([objectShader](Forest& forest, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);
		objectShader->SetUniformValue("u_model", modelMatrix);

		auto meshId         = MeshId::FeatureForest;
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Draw(*objectShader, 1);
	});

	_registry.view<MobileObject, Transform>().each([objectShader](MobileObject& mobileObject, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);
		objectShader->SetUniformValue("u_model", modelMatrix);

		auto meshId         = mobileObjectMeshLookup[mobileObject.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Draw(*objectShader, 1);
	});
}
} // namespace openblack::Entities
