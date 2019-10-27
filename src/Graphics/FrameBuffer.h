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

#include <cstdint>
#include <memory>
#include <optional>

#include <Graphics/Texture2D.h>

namespace openblack::graphics
{

class FrameBuffer {
public:
	FrameBuffer() = delete;
	FrameBuffer(std::string name, uint16_t width, uint16_t height, Format colorFormat, std::optional<Format> depthStencilFormat = {});
	~FrameBuffer();

	void Bind(uint8_t viewId) const;

	Texture2D& GetColorAttachment() { return _colorAttachment; }
	void GetSize(uint16_t& width, uint16_t& height) const { width = _width, height = _height; }

private:
	std::string _name;
	bgfx::FrameBufferHandle _handle;

	uint16_t _width;
	uint16_t _height;
	Format _colorFormat;
	std::optional<Format> _depthStencilFormat;

	Texture2D _colorAttachment;
	Texture2D _depthStencilAttachment;
};

}  // namespace openblack::graphics
