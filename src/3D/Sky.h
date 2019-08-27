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
#ifndef OPENBLACK_3D_SKY_H
#define OPENBLACK_3D_SKY_H

#include <3D/Camera.h>
#include <3D/SkinnedModel.h>
#include <Common/Bitmap16B.h>
#include <Graphics/ShaderProgram.h>
#include <Graphics/Texture2D.h>
#include <memory>

using OpenBlack::Graphics::ShaderProgram;

namespace OpenBlack
{

class Sky
{
  public:
	Sky();
	~Sky() = default;

	void Draw(const Camera&);

  private:
	std::unique_ptr<ShaderProgram> _shader;
	std::unique_ptr<SkinnedModel> _model;
	std::unique_ptr<Texture2D> _texture;
};

} // namespace OpenBlack

#endif
