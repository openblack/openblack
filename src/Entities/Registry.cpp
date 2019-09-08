#include "Registry.h"

#include <Game.h>
#include <3D/L3DMesh.h>
#include <3D/MeshPack.h>
#include <3D/Camera.h>
#include <Graphics/ShaderManager.h>
#include <Entities/Components/Transform.h>
#include <Entities/Components/Model.h>
#include <Entities/Components/Villager.h>

namespace openblack::Entities
{
void Registry::DebugCreateEntities(float x, float y, float z)
{
	auto entity = _registry.create();
	_registry.assign<Transform>(entity, x, y, z, 2.5f, 180.0f, 111.0f, 0.0f);
	MeshId meshId = MeshId::Dummy;
	_registry.assign<Model>(entity, meshId);
	uint32_t health = 100;
	uint32_t age = 18;
	uint32_t hunger = 100;
	_registry.assign<Villager>(entity, health, age, hunger);
}

void Registry::DrawModels(const Camera& camera, Graphics::ShaderManager& shaderManager)
{
	const auto& meshPack = Game::instance()->GetMeshPack();
	const auto& view     = _registry.view<Model, Transform>();
	const auto projectionMatrix = camera.GetViewProjectionMatrix();

	Graphics::ShaderProgram* objectShader = shaderManager.GetShader("SkinnedMesh");
	objectShader->Bind();
	objectShader->SetUniformValue("u_viewProjection", projectionMatrix);

	for (auto const& entity : view)
	{
		const auto& model = view.get<Model>(entity);
		const auto& position = view.get<Transform>(entity);

		const auto _modelPosition = glm::vec3(position.x + model.xOffset, position.y + model.yOffset, position.z + model.zOffset);
		const auto _modelRotation = glm::vec3(position.rotX, position.rotY, position.rotZ);
		const auto _modelScale    = glm::vec3(position.scale);

		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix           = glm::translate(modelMatrix, _modelPosition);

		modelMatrix = glm::rotate(modelMatrix, _modelRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, _modelRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, _modelRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

		modelMatrix = glm::scale(modelMatrix, _modelScale);

		objectShader->SetUniformValue("u_modelTransform", modelMatrix);

		const L3DMesh& mesh = meshPack.GetMesh(static_cast<uint32_t>(model.meshId));
		mesh.Draw(*objectShader, model.submeshId);
	}
}

void Registry::Update()
{
}
} // namespace openblack::Entities
