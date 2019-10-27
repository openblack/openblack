/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <array>
#include <memory>

namespace openblack
{

class L3DMesh;

namespace graphics
{
class ShaderProgram;
class Texture2D;
}

class Sky
{
  public:
	Sky();
	~Sky() = default;

	void SetDayNightTimes(float nightFull, float duskStart, float duskEnd, float dayFull);

	void Interpolate555Texture(uint16_t* bitmap, uint16_t*, uint16_t*, float);

	void CalculateTextures();
	void Draw(graphics::RenderPass viewId, const glm::mat4& modelMatrix, const graphics::ShaderProgram &program, bool cullBack) const;
	void SetTime(float time);

	float TimeOfDay;

  private:
	std::unique_ptr<L3DMesh> _model;
	std::unique_ptr<graphics::Texture2D> _texture;

	std::array<std::array<uint16_t, 256 * 256>, 9> _bitmaps;

	float _timeOfDay;
	float _nightFullTime;
	float _duskStartTime;
	float _duskEndTime;
	float _dayFullTime;
};

} // namespace openblack
