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

namespace OpenBlack
{
class GameWindow;

class Renderer {
  public:
	enum class Api {
		OpenGl,
	};
	struct RequiredAttribute {
		Api api;
		uint32_t name;
		int value;
	};
	static std::vector<RequiredAttribute> GetRequiredAttributes();
	static uint32_t GetRequiredFlags();

	Renderer() = delete;
	explicit Renderer(std::unique_ptr<GameWindow>& window);

	~Renderer() = default;
};
} // namespace OpenBlack
