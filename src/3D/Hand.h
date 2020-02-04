/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Graphics/Mesh.h"
#include "Graphics/Texture2D.h"
#include "L3DMesh.h"

#include <memory>

namespace openblack
{
class Hand
{
public:
	Hand();
	~Hand();

	void Init();
	void Draw();

private:
	std::unique_ptr<L3DMesh> m_HandModel;
};
} // namespace openblack
