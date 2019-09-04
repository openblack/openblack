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

#include "Renderer.h"

#include <sstream>

#include <GL/glew.h>
#include <SDL_video.h>
#include <spdlog/spdlog.h>

#include <GameWindow.h>

using namespace OpenBlack;

std::vector<Renderer::RequiredAttribute> Renderer::GetRequiredAttributes() {
	return std::vector<RequiredAttribute> {
		{Api::OpenGl, SDL_GL_RED_SIZE, 8},
		{Api::OpenGl, SDL_GL_GREEN_SIZE, 8},
		{Api::OpenGl, SDL_GL_BLUE_SIZE, 8},
		{Api::OpenGl, SDL_GL_ALPHA_SIZE, 8},
		{Api::OpenGl, SDL_GL_DOUBLEBUFFER, 1},

		{Api::OpenGl, SDL_GL_MULTISAMPLEBUFFERS, 1},
		{Api::OpenGl, SDL_GL_MULTISAMPLESAMPLES, 4},

		{Api::OpenGl, SDL_GL_CONTEXT_MAJOR_VERSION, 3},
		{Api::OpenGl, SDL_GL_CONTEXT_MINOR_VERSION, 3},
		{Api::OpenGl, SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE},
	};
}

uint32_t Renderer::GetRequiredFlags()
{
	return SDL_WINDOW_OPENGL;
}

Renderer::Renderer(std::unique_ptr<GameWindow>& window)
{
	// Create a debug context?
	bool useDebug = true;
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, useDebug ? SDL_GL_CONTEXT_DEBUG_FLAG : 0);

	auto context = SDL_GL_CreateContext(window->GetHandle());

	if (context == nullptr)
	{
		throw std::runtime_error("Could not create OpenGL context: " + std::string(SDL_GetError()));
	}
	else
	{
		_glcontext = std::unique_ptr<SDL_GLContext, SDLDestroyer>(&context);
	}

	spdlog::info("OpenGL context successfully created.");

	int majorVersion, minorVersion;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &majorVersion);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minorVersion);

	spdlog::info("OpenGL version: {}.{}", majorVersion, minorVersion);

	// initalize glew
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
#ifdef GLEW_ERROR_NO_GLX_DISPLAY
	if (GLEW_ERROR_NO_GLX_DISPLAY == err)
	{
		spdlog::warn("GLEW couldn't open GLX display");
	}
	else
#endif
	if (GLEW_OK != err)
	{
		std::stringstream error;
		error << "glewInit failed: " << glewGetErrorString(err) << std::endl;
		throw std::runtime_error(error.str());
	}

	spdlog::info("Using GLEW {0}", glewGetString(GLEW_VERSION));
}

SDL_GLContext& Renderer::GetGLContext() const
{
	return *_glcontext;
}
