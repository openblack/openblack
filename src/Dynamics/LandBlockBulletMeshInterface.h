/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cassert>
#include <cstdint>

#include <array>
#include <span>
#include <vector>

#ifdef _MSC_VER
// Disable warning about conditional expression not being is constant
#pragma warning(push)
#pragma warning(disable : 4127)
#endif
#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "3D/LandBlock.h"

namespace openblack::dynamics
{

class LandBlockBulletMeshInterface: public btStridingMeshInterface
{
	std::vector<std::array<float, 3>> _vertices;
	std::vector<uint16_t> _indices;

public:
	explicit LandBlockBulletMeshInterface(std::span<const LandVertex> vertices)
	    : _vertices(vertices.size())
	    , _indices(vertices.size())
	{
		// TODO (#749) use std::views::enumerate
		for (uint16_t i = 0; auto& v : _vertices)
		{
			const auto& vertex = vertices[i];
			v[0] = vertex.position.x;
			v[1] = vertex.position.y;
			v[2] = vertex.position.z;
			_indices[i] = i;
			++i;
		}
	}

	/// get read and write access to a subpart of a triangle mesh
	/// this subpart has a continuous array of vertices and indices
	/// in this way the mesh can be handled as chunks of memory with striding
	/// very similar to OpenGL vertexarray support
	/// make a call to unLockVertexBase when the read and write access is finished
	void getLockedVertexIndexBase([[maybe_unused]] unsigned char** vertexbase, [[maybe_unused]] int& numverts,
	                              [[maybe_unused]] PHY_ScalarType& type, [[maybe_unused]] int& stride,
	                              [[maybe_unused]] unsigned char** indexbase, [[maybe_unused]] int& indexstride,
	                              [[maybe_unused]] int& numfaces, [[maybe_unused]] PHY_ScalarType& indicestype,
	                              [[maybe_unused]] int subpart) override
	{
	}

	void getLockedReadOnlyVertexIndexBase(const unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride,
	                                      const unsigned char** indexbase, int& indexstride, int& numfaces,
	                                      PHY_ScalarType& indicestype, [[maybe_unused]] int subpart) const override
	{
		assert(subpart == 0);
		*vertexbase = reinterpret_cast<const unsigned char*>(_vertices.data());
		numverts = static_cast<int>(_vertices.size());
		type = PHY_ScalarType::PHY_FLOAT;
		stride = sizeof(_vertices[0]);
		*indexbase = reinterpret_cast<const unsigned char*>(_indices.data());
		indexstride = 3 * sizeof(_indices[0]);
		numfaces = static_cast<int>(_indices.size() / 3);
		indicestype = PHY_ScalarType::PHY_SHORT;
	}

	/// unLockVertexBase finishes the access to a subpart of the triangle mesh
	/// make a call to unLockVertexBase when the read and write access (using getLockedVertexIndexBase) is finished
	void unLockVertexBase([[maybe_unused]] int subpart) override {}

	void unLockReadOnlyVertexBase([[maybe_unused]] int subpart) const override {}

	/// getNumSubParts returns the number of separate subparts
	/// each subpart has a continuous array of vertices and indices
	[[nodiscard]] int getNumSubParts() const override { return 1; }

	void preallocateVertices([[maybe_unused]] int numverts) override {}

	void preallocateIndices([[maybe_unused]] int numindices) override {}
};
} // namespace openblack::dynamics
