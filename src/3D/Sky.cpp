#include <3D/Sky.h>

#include <Game.h>
#include <3D/Camera.h>

#include <Common/OSFile.h>
#include <Graphics/GameShaders.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using OpenBlack::Sky;

Sky::Sky()
{
	// create our shader
	_shader = std::make_unique<Shader>();
	_shader->Create(OpenBlack::Shaders::WorldObject::VertexShader, OpenBlack::Shaders::WorldObject::FragmentShader);

	// load in the mesh
	size_t skyMeshSize;
	char* skyMesh = OSFile::ReadAll((Game::instance()->GetGamePath() + "/Data/WeatherSystem/sky.l3d").c_str(), &skyMeshSize);
	_model = std::make_unique<L3DModel>();
	_model->LoadFromL3D(skyMesh, skyMeshSize, false);
	free(skyMesh);
}

void Sky::Draw()
{
	glUseProgram(_shader->GetHandle());
	GLint viewProj = glGetUniformLocation(_shader->GetHandle(), "viewProj");
	glUniformMatrix4fv(viewProj, 1, GL_FALSE, glm::value_ptr(Game::instance()->GetCamera().GetViewProjectionMatrix()));

	_model->Draw();
}
