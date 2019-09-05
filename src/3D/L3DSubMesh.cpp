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

#include <3D/L3DSubMesh.h>
#include <Common/IStream.h>
#include <Game.h>
#include <spdlog/spdlog.h>

namespace OpenBlack
{

void L3DSubMesh::Load(IStream& stream)
{
	struct {
		uint32_t flags; // nodraw, transparent
		uint32_t numPrimitives;
		uint32_t primitivesOffset;
		uint32_t numBones;
		uint32_t bonesOffset;
	} header;

	stream.Read(&header);

	// offsets are local to stream :D

	//spdlog::debug("position: {}", stream.Position());
	//spdlog::debug("# prims: {} @ {}", header.numPrimitives, header.primitivesOffset);
	//spdlog::debug("# bones: {} @ {}", header.numBones, header.bonesOffset);
}

} // namespace OpenBlack
