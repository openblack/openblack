/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

namespace openblack
{

namespace graphics
{
class L3DMesh;
class Texture2D;
} // namespace graphics

class SkyInterface
{
public:
	[[nodiscard]] virtual float GetCurrentSkyType() const noexcept = 0;
	[[nodiscard]] virtual graphics::L3DMesh& GetMesh() const noexcept = 0;
	[[nodiscard]] virtual graphics::Texture2D& GetTexture() const noexcept = 0;
	virtual void SetTime(float time) noexcept = 0;
};

} // namespace openblack
