#include <3D/Sky.h>

#include <Game.h>
#include <3D/Camera.h>

#include <Common/OSFile.h>
#include <Graphics/GameShaders.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace OpenBlack;

Sky::Sky()
{
	// create our shader
	_shader = std::make_unique<ShaderProgram>(
		OpenBlack::Shaders::WorldObject::VertexShader,
		OpenBlack::Shaders::WorldObject::FragmentShader
	);

	// load in the mesh
	size_t skyMeshSize;
	char* skyMesh = OSFile::ReadAll((Game::instance()->GetGamePath() + "/Data/WeatherSystem/sky.l3d").c_str(), &skyMeshSize);
	_model = std::make_unique<L3DModel>();
	_model->LoadFromL3D(skyMesh, skyMeshSize, false);
	free(skyMesh);

	// load some sky bitmaps
	Bitmap16B* bitmap = Bitmap16B::LoadFromFile(Game::instance()->GetGamePath() + "/Data/WeatherSystem/Sky_Ntrl_Day.555");
	_texture = std::make_unique<Texture2D>(bitmap->Width(), bitmap->Height(), GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_1_5_5_5_REV, bitmap->Data());
	delete bitmap;
}

void Sky::Draw()
{
	_shader->Bind();
	_shader->SetUniformValue("viewProj", Game::instance()->GetCamera().GetViewProjectionMatrix());

	glActiveTexture(GL_TEXTURE0);
	_texture->Bind();

	_model->Draw();
}
