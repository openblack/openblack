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

#pragma once
#ifndef OPENBLACK_3D_SKY_H
#define OPENBLACK_3D_SKY_H

#include <memory>
#include <Graphics/Shader.h>
#include <3D/L3DModel.h>
#include <Common/Bitmap16B.h>
#include <Graphics/Texture2D.h>

using OpenBlack::Graphics::Shader;

namespace OpenBlack {

class Sky
{
public:
	Sky();
	~Sky() = default;

	void Draw();

private:
	std::unique_ptr<Shader> _shader;
	std::unique_ptr<L3DModel> _model;
	std::unique_ptr<Bitmap16B> _bitmap;
	std::unique_ptr<Texture2D> _texture;
};

}

#endif
