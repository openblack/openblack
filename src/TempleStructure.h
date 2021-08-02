/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "3D/CameraPath.h"
#include "Common/MeshLookup.h"
#include "Entities/Components/Temple.h"

#include <glm/vec3.hpp>
#include <SDL.h>

#include <map>

namespace openblack
{
class TempleStructure
{
public:
	enum class TempleOutdoorPart
	{
		EvilTemple,
		WorshipSite,
		Entrance,
		TempleState1,
		// TODO(raffclar): Further research needed into temple states (exterior meshes)
//		TempleState2,
//		TempleState3,
//		TempleState4,
//		TempleState5,
//		TempleState6,
//		TempleState7,
//		TempleState8,
//		TempleState9,
//		TempleState10,
//		TempleState11,
//		TempleState12,
//		TempleState13
	};

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

	int GetNumberOfRooms() const { return _templeRoomCount; }
	bool LoadOutsideTempleMeshes();
	bool LoadInteriorTempleAssets();
	MeshId GetTempleMeshId();
	MeshId GetTempleWorshipSiteMeshId();
	std::vector<MeshId> GetInteriorTempleMeshIds();
	void EnterTemple();
	void ExitTemple();
	void MoveToRoom(TempleRoom room, bool teleport);
	static const std::string& GetRoomName(TempleRoom room);
	bool IsInsideTemple() const { return _insideTemple; }
	TempleRoom ActiveRoom() const { return _activeRoom; }
	glm::vec3 GetTemplePosition() const { return _templePosition; }
	bool IsFreeMovementEnabled() const { return _freeMovement; }
	void SetFreeMovementEnabled(bool freeMovement) { _freeMovement = freeMovement; }
	bool IsTeleportEnabled() const { return _teleport; }
	void SetTeleportEnabled(bool teleport) { _teleport = teleport; }
	void ProcessSDLEvent(const SDL_Event& e);
private:
	bool _freeMovement = true;
	bool _teleport = false;
	const int _templeRoomCount = 7;
	TempleRoom _activeRoom = TempleRoom::Unknown;
	bool _insideTemple;
	glm::vec3 _lastPlayerPosition;
	glm::vec3 _lastPlayerRotation;
	glm::vec3 _templePosition;
	std::map<TempleOutdoorPart, MeshId> _outsideMeshes;
	std::map<TempleIndoorPart, MeshId> _insideMeshes;
	std::unordered_map<TempleRoom, std::unique_ptr<CameraPath>> _insideCameraPaths;
};
}
