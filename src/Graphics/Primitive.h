/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <memory>

namespace openblack::graphics
{
class Mesh;

class Primitive
{
public:
	static std::unique_ptr<Mesh> CreatePlane();

protected:
	Primitive() = delete;
};

} // namespace openblack::graphics
