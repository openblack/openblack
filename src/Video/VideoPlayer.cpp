/* OpenBlack - A reimplementation of Lionheads's Black & White engine
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

#include "VideoPlayer.h"

using namespace OpenBlack::Graphics;
using namespace OpenBlack::Video;

#include <iostream>

VideoPlayer::VideoPlayer(const std::string& file)
{
	_file = std::make_unique<OSFile>();
	_file->Open(file.c_str(), LH_FILE_MODE::Read);

	std::cout << "Opening Video: " << file << std::endl;

	/*_bink = std::make_unique<Bink>(_file.get());

	std::cout << "Dimensions: " << _bink->GetWidth() << "x" << _bink->GetHeight() << std::endl;
	std::cout << "Frame Count: " << _bink->GetFrameCount() << " @ " << _bink->GetFPS() << "FPS" << std::endl;

	_bink->DoFrame();*/
}

VideoPlayer::~VideoPlayer()
{
	if (_file != nullptr)
		_file->Close();
}

void VideoPlayer::createTexture()
{
	if (_texture != nullptr)
		_texture.reset();

	/*TextureDef2D tex;
	tex.width = 256;
	tex.height = 256;
	tex.format = GL_RGBA;
	tex.internalFormat = GL_RGBA;
	tex.type = GL_UNSIGNED_INT_8_8_8_8;
	tex.data = new uint8_t[256 * 256 * 4];
	memset((void*)tex.data, 100, 256 * 256 * 4);

	_texture = std::make_shared<Texture2D>();
	_texture->Create(tex);*/
}
