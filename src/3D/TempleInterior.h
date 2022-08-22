/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <map>

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

#include "3D/CameraPath.h"
#include "3D/TempleInteriorInterface.h"
#include "ECS/Components/Temple.h"

namespace openblack
{
class TempleInterior final: public TempleInteriorInterface
{
public:
	enum class TempleIndoorPart
	{
		Challenge,
		ChallengeLO,
		ChallengeDome,
		ChallengeFloor,
		ChallengeFloorLO,
		CreatureCave,
		CreatureCaveLO,
		CreatureCaveWater,
		CreatureCaveWaterLO,
		Credits,
		CreditsLO,
		CreditsDome,
		CreditsFloor,
		CreditsFloorLO,
		Main,
		MainLO,
		MainFloor,
		MainFloorLO,
		MainWater,
		MainWaterLO,
		Movement,
		Multi,
		MultiLO,
		MultiDome,
		MultiFloor,
		MultiFloorLO,
		Options,
		OptionsLO,
		OptionsDome,
		OptionsFloor,
		OptionsFloorLO,
		SaveGame,
		SaveGameLO,
		SaveGameDome,
		SaveGameFloor,
		SaveGameFloorLO,
	};

	[[nodiscard]] bool Active() const override { return _active; }
	[[nodiscard]] glm::vec3 GetPosition() const override { return _templePosition; }
	void Activate() override;
	void Deactivate() override;

private:
	bool _active;
	const glm::vec3 _templePosition;
	glm::vec3 _playerPositionOutside;
	glm::vec3 _playerRotationOutside;
};
} // namespace openblack
