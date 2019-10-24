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
{
	_registry.set<RegistryContext>();
}

Registry::~Registry()
{
}

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

	_registry.view<const Tree, const Transform>().each([viewId, debugShader, objectShader, state, &boundingBox](const Tree& entity, const Transform& transform) {
		auto modelMatrix = static_cast<const glm::mat4>(transform);

		auto meshId = treeMeshLookup[entity.treeInfo];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[mesh.GetNumSubMeshes() - 1]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const Abode, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const Abode& entity, const Transform& transform) {
		auto modelMatrix = static_cast<const glm::mat4>(transform);

		auto meshId = abodeMeshLookup[entity.abodeInfo];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[0]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const AnimatedStatic, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const AnimatedStatic& entity, const Transform& transform) {
		auto modelMatrix = static_cast<const glm::mat4>(transform);

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

		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<int>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[0]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const MobileStatic, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const MobileStatic& entity, const Transform& transform) {
		auto modelMatrix = static_cast<const glm::mat4>(transform);

		auto meshId = mobileStaticMeshLookup[entity.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[1]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const Feature, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const Feature& entity, const Transform& transform) {
		auto modelMatrix = static_cast<const glm::mat4>(transform);

		auto meshId = featureMeshLookup[entity.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[1]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const Field, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const Field& entity, const Transform& transform) {
		auto modelMatrix = static_cast<const glm::mat4>(transform);

		auto meshId = MeshId::TreeWheat;
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[0]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const Forest, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const Forest& entity, const Transform& transform) {
		auto modelMatrix = static_cast<const glm::mat4>(transform);

		auto meshId = MeshId::FeatureForest;
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[0]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

	_registry.view<const MobileObject, const Transform>().each([viewId, state, objectShader, debugShader, boundingBox](const MobileObject& entity, const Transform& transform) {
		auto modelMatrix = static_cast<const glm::mat4>(transform);

		auto meshId = mobileObjectMeshLookup[entity.type];
		const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
		mesh.Submit(viewId, modelMatrix, *objectShader, state);

		if (boundingBox)
		{
			auto box = mesh.GetSubMeshes()[1]->GetBoundingBox();
			boundingBox->SetPose(box.center() + transform.position, box.size() * transform.scale);
			boundingBox->Draw(viewId, *debugShader);
		}
	});

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
