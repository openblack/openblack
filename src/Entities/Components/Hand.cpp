/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Hand.h"

using openblack::entities::components::Hand;
using namespace openblack;

void Hand::Init()
{
	// Data/CreatureMesh/Hand_Boned_Base2.l3d
	// Data/CreatureMesh/Hand_Boned_Good2.l3d
	// Data/CreatureMesh/Hand_Boned_Evil2.l3d
	// load data\\hand.raw into a memory
}

const std::vector<std::string> Hand::_grabLandSounds = {
    "G_HandGrabLand_01.wav", "G_HandGrabLand_02.wav", "G_HandGrabLand_03.wav",
    "G_HandGrabLand_04.wav", "G_HandGrabLand_05.wav", "G_HandGrabLand_06.wav",
};