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

#include <SDL_video.h>

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
}
