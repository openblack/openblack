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

	TextureDef2D tex;
	tex.width = 256;
	tex.height = 256;
	tex.format = GL_RGBA;
	tex.internalFormat = GL_RGBA;
	tex.type = GL_UNSIGNED_INT_8_8_8_8;
	tex.data = new uint8_t[256 * 256 * 4];
	memset((void*)tex.data, 100, 256 * 256 * 4);

	_texture = std::make_shared<Texture2D>();
	_texture->Create(tex);
}
