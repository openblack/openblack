/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <chrono>
#include <optional>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "3D/TempleLight.h"
#include "3D/TempleMap.h"
#include "Graphics/GraphicsHandle.h"

namespace openblack
{
namespace graphics
{
class Texture2D;
}
namespace gui
{
class GameInterface;
} // namespace gui
struct OrientedTextVertex;
class TempleDoors;

enum class TempleRoom
{
	Challenge,
	CreatureCave,
	Credits,
	Main,
	Multi,
	Options,
	SaveGame,
	Unknown
};

/// A belt or medal of the creature's room: its icon's mesh, where it is, and its colour
struct TempleCaveTrophy
{
	uint32_t mesh;
	glm::mat4 model;
	uint32_t colour;
	/// Whether it is drawn with an environment map added
	bool environmentMapped;
};

/// Where the cursor meets the temple's room, and the way the surface there faces
struct TempleCursorHit
{
	glm::vec3 point;
	glm::vec3 normal;
	/// The room whose mesh the cursor is over, and the submesh of it, when it is over one
	TempleRoom room {TempleRoom::Unknown};
	std::optional<uint32_t> subMesh;
};

/// A texture a room's mesh draws one of its submeshes with, in place of its skin
struct TempleSubMeshTexture
{
	uint32_t subMesh;
	graphics::TextureHandle texture;
};

/// A colour added to one of a room's submeshes, from 0 to 1, as a control glows under the cursor
struct TempleSubMeshGlow
{
	uint32_t subMesh;
	glm::vec3 colour;
};

class TempleInteriorInterface
{
public:
	virtual ~TempleInteriorInterface() = default;

	[[nodiscard]] virtual bool Active() const = 0;
	[[nodiscard]] virtual glm::vec3 GetPosition() const = 0;
	virtual void Activate() = 0;
	virtual void Deactivate() = 0;
};
} // namespace openblack
