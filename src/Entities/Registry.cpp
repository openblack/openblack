#include "Registry.h"

#include <3D/Camera.h>
#include <3D/L3DMesh.h>
#include <3D/MeshPack.h>
#include <Entities/Components/AnimatedStatic.h>
#include <Entities/Components/Model.h>
#include <Entities/Components/Transform.h>
#include <Entities/Components/Villager.h>
#include <Game.h>
#include <Graphics/ShaderManager.h>

namespace openblack::Entities
{
void Registry::DebugCreateEntities(float x, float y, float z)
{
}

void Registry::DrawModels(const Camera& camera, graphics::ShaderManager& shaderManager)
{
	graphics::ShaderProgram* objectShader = shaderManager.GetShader("SkinnedMesh");
	objectShader->Bind();
	objectShader->SetUniformValue("u_viewProjection", camera.GetViewProjectionMatrix());

	_registry.view<Model, Transform>().each([objectShader](Model& model, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		objectShader->SetUniformValue("u_modelTransform", modelMatrix);
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(model.meshId));

		for (auto& submeshId : model.submeshIds)
			mesh.Draw(*objectShader, submeshId);
	});

	_registry.view<AnimatedStatic, Transform>().each([objectShader](AnimatedStatic& animated, Transform& transform) {
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, transform.position);
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix           = glm::rotate(modelMatrix, transform.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		modelMatrix           = glm::scale(modelMatrix, transform.scale);

		objectShader->SetUniformValue("u_modelTransform", modelMatrix);

		// temporary-ish:
		MeshId meshID = MeshId::Dummy;
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

		objectShader->SetUniformValue("u_modelTransform", modelMatrix);

		// temporary-ish until we read info.dat:
		MeshId meshID = MeshId::Dummy;
		if (mobile.type == 0)
		{
			const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh((int)MeshId::BuildingCelticFenceShort);
			mesh.Draw(*objectShader, 1);
		}
		else if (mobile.type == 1)
		{
			const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh((int)MeshId::BuildingCelticFenceTall);
			mesh.Draw(*objectShader, 1);
		}
		else
		{
			const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(0);
			mesh.Draw(*objectShader, 0);
		}
	});

	//
}

void Registry::Update()
{
}
} // namespace openblack::Entities
