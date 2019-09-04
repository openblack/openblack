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

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <SDL.h>

namespace OpenBlack
{
class GameWindow;

class Renderer {
  struct SDLDestroyer
  {
	void operator()(SDL_GLContext* glcontext) const { SDL_GL_DeleteContext(*glcontext); }
  };

  public:
	enum class Api {
		OpenGl,
	};
	struct RequiredAttribute {
		Api api;
		uint32_t name;
		int value;
	};
	static std::vector<RequiredAttribute> GetRequiredWindowingAttributes();
	static uint32_t GetRequiredFlags();

	Renderer() = delete;
	explicit Renderer(std::unique_ptr<GameWindow>& window);

	~Renderer() = default;

	[[nodiscard]] SDL_GLContext& GetGLContext() const;
	void MessageCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int32_t length, const std::string& message) const;

  private:
  static std::vector<RequiredAttribute> GetRequiredContextAttributes();

  std::unique_ptr<SDL_GLContext, SDLDestroyer> _glcontext;
};
} // namespace OpenBlack
