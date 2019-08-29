/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <3D/Sky.h>
#include <Game.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace OpenBlack;

Sky::Sky()
{
	// create our shader
	_shader = std::make_unique<ShaderProgram>("shaders/sky.vert", "shaders/sky.frag");

	// load in the mesh
	_model = std::make_unique<SkinnedModel>();
	_model->LoadFromFile(Game::instance()->GetGamePath() + "/Data/WeatherSystem/sky.l3d");

	// load some sky bitmaps
	Bitmap16B* bitmap = Bitmap16B::LoadFromFile(Game::instance()->GetGamePath() + "/Data/WeatherSystem/Sky_Ntrl_Day.555");
	_texture          = std::make_unique<Texture2D>();
	_texture->Create(bitmap->Data(), DataType::UnsignedShort1555Rev, Format::BGRA, bitmap->Width(), bitmap->Height(), InternalFormat::RGB5A1);
	delete bitmap;
}

void Sky::Draw(const Camera& camera)
{
	_shader->Bind();
	_shader->SetUniformValue("viewProj", camera.GetViewProjectionMatrix());

	glActiveTexture(GL_TEXTURE0);
	_texture->Bind();

	_model->Draw(_shader.get());
}
