/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

namespace openblack::ecs::components
{

/// Meshes with this component will have their vertices match terrain height maps
///
/// They correspond with vanilla Get3DType of MORPHABLE or CITADEL
/// The following Objects should be created with this component:
/// * BigForest
/// * Graveyard
/// * PileFood (not wood)
/// * StoragePit x2 meshes
/// * Wonder (Depends on tribe type but defaults to morphable)
/// * Workshop
/// * CitadelHeart
/// * CitadelPart
/// * Creche
/// * Football
/// * TODO: PhysicalShield (not magic)
/// * TownCentre
/// * Field
struct MorphWithTerrain
{
	int dummy;
};

} // namespace openblack::ecs::components
