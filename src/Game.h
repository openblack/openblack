/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <bgfx/bgfx.h>
#include <entt/entity/fwd.hpp>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4701) // func_common.inl glm::any template
#endif
#include <glm/mat4x4.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <spdlog/common.h>

#include "InfoConstants.h"
#include "ReferenceManager.h"
#include "ScriptHeaders/ScriptEnums.h"
#include "Windowing/WindowingInterface.h" // For DisplayMode

union SDL_Event;

namespace openblack
{
class Camera;
class EventManager;
class Profiler;
class Renderer;
class L3DAnim;
class L3DMesh;
class Sky;
class Water;

namespace debug::gui
{
class Gui;
}

namespace graphics
{
class Texture2D;
}

namespace LHVM
{
class LHVM;
class NativeFunction;
enum class ErrorCode : uint8_t;
} // namespace LHVM

namespace lhscriptx
{
class Script;
}

enum class LoggingSubsystem : uint8_t
{
	game,
	input,
	graphics,
	scripting,
	audio,
	pathfinding,
	ai,
	lhvm,

	_count
};

constexpr static std::array<std::string_view, static_cast<size_t>(LoggingSubsystem::_count)> k_LoggingSubsystemStrs {
    "game",        //
    "input",       //
    "graphics",    //
    "scripting",   //
    "audio",       //
    "pathfinding", //
    "ai",          //
    "lhvm",        //
};

struct Arguments
{
	std::string executablePath;
	int windowWidth;
	int windowHeight;
	bool vsync;
	openblack::windowing::DisplayMode displayMode;
	bgfx::RendererType::Enum rendererType;
	std::string gamePath;
	float scale;
	uint32_t numFramesToSimulate;
	std::string logFile;
	std::array<spdlog::level::level_enum, k_LoggingSubsystemStrs.size()> logLevels;
	std::string startLevel;
	std::optional<std::pair</* frame number */ uint32_t, /* output */ std::filesystem::path>> requestScreenshot;
};

class Game
{
public:
	static constexpr auto k_TurnDuration = std::chrono::milliseconds(100);
	static constexpr float k_TurnDurationMultiplierSlow = 2.0f;
	static constexpr float k_TurnDurationMultiplierNormal = 1.0f;
	static constexpr float k_TurnDurationMultiplierFast = 0.5f;

	struct Config
	{
		bool wireframe {false};
		bool showVillagerNames {false};
		bool debugVillagerNames {false};
		bool debugVillagerStates {false};

		bool viewDetailOverlay {false};
		bool drawSky {true};
		bool drawWater {true};
		bool drawIsland {true};
		bool drawEntities {true};
		bool drawSprites {true};
		bool drawTestModel {true};
		bool drawDebugCross {true};
		bool drawBoundingBoxes {false};
		bool drawFootpaths {false};
		bool drawStreams {false};

		float timeOfDay {12.0f};
		float skyAlignment {0.0f};
		float bumpMapStrength {1.0f};
		float smallBumpMapStrength {1.0f};

		float cameraXFov {70.0f};
		float cameraNearClip {1.0f};
		float cameraFarClip {static_cast<float>(0x10000)};

		bool bgfxDebug {false};
		bool bgfxProfile {false};
		bool running {false};

		uint32_t numFramesToSimulate {0};
	};

	explicit Game(Arguments&& args);
	virtual ~Game();

	bool ProcessEvents(const SDL_Event& event);
	bool GameLogicLoop();
	bool Update();
	bool Initialize();
	bool Run();

	void LoadMap(const std::filesystem::path& path);
	void LoadLandscape(const std::filesystem::path& path);

	bool LoadVariables();

	void SetTime(float time);
	void SetGameSpeed(float multiplier) { _gameSpeedMultiplier = multiplier; }
	[[nodiscard]] float GetGameSpeed() const { return _gameSpeedMultiplier; }

	Camera& GetCamera() { return *_camera; }
	[[nodiscard]] Profiler& GetProfiler() const { return *_profiler; }
	[[nodiscard]] Renderer& GetRenderer() const { return *_renderer; }
	[[nodiscard]] Camera& GetCamera() const { return *_camera; }
	[[nodiscard]] Sky& GetSky() const { return *_sky; }
	[[nodiscard]] Water& GetWater() const { return *_water; }
	[[nodiscard]] entt::entity GetHand() const;
	[[nodiscard]] const LHVM::LHVM& GetLhvm() const { return *_lhvm; }
	LHVM::LHVM& GetLhvm() { return *_lhvm; }
	const InfoConstants& GetInfoConstants() { return _infoConstants; } ///< Access should be only read-only
	Config& GetConfig() { return _config; }
	[[nodiscard]] const Config& GetConfig() const { return _config; }
	[[nodiscard]] uint16_t GetTurn() const { return _turnCount; }
	[[nodiscard]] bool IsPaused() const { return _paused; }
	[[nodiscard]] std::chrono::duration<float, std::milli> GetDeltaTime() const { return _turnDeltaTime; }
	[[nodiscard]] const glm::ivec2& GetMousePosition() const { return _mousePosition; }

	void RequestScreenshot(const std::filesystem::path& path);

	void NativeCallEnterCallback(uint32_t func);
	void NativeCallExitCallback(uint32_t func);
	void StopTaskCallback(uint32_t taskNumber);
	void LhvmErrorCallback(LHVM::ErrorCode code, std::string v0, uint32_t v1);
	void AddObjectReference(uint32_t objId);
	void RemoveObjectReference(uint32_t objId);

	[[nodiscard]] glm::vec3 PopVec();
	void PushVec(const glm::vec3& vec);
	[[nodiscard]] std::string PopString();
	[[nodiscard]] std::vector<float> PopVarArg(int32_t argc);
	[[nodiscard]] entt::entity CreateScriptObject(script::ObjectType type, uint32_t subtype, const glm::vec3& position,
	                                              float altitude, float xAngleRadians, float yAngleRadians, float zAngleRadians,
	                                              float scale);

#define CREATE_FUNCTION_BINDING(NAME, STACKIN, STACKOUT, FUNCTION)                       \
	{                                                                                    \
		_functionsTable.emplace_back([this]() { FUNCTION(); }, STACKIN, STACKOUT, NAME); \
	}

	void InitFunctionsTable();
	void InitFunctionsTable0();
	void InitFunctionsTable1();
	void InitFunctionsTable2();
	void InitFunctionsTable3();
	void InitFunctionsTable4();

	void VmNone();                                        // 000 NONE
	void VmSetCameraPosition();                           // 001 SET_CAMERA_POSITION
	void VmSetCameraFocus();                              // 002 SET_CAMERA_FOCUS
	void VmMoveCameraPosition();                          // 003 MOVE_CAMERA_POSITION
	void VmMoveCameraFocus();                             // 004 MOVE_CAMERA_FOCUS
	void VmGetCameraPosition();                           // 005 GET_CAMERA_POSITION
	void VmGetCameraFocus();                              // 006 GET_CAMERA_FOCUS
	void VmSpiritEject();                                 // 007 SPIRIT_EJECT
	void VmSpiritHome();                                  // 008 SPIRIT_HOME
	void VmSpiritPointPos();                              // 009 SPIRIT_POINT_POS
	void VmSpiritPointGameThing();                        // 010 SPIRIT_POINT_GAME_THING
	void VmGameThingFieldOfView();                        // 011 GAME_THING_FIELD_OF_VIEW
	void VmPosFieldOfView();                              // 012 POS_FIELD_OF_VIEW
	void VmRunText();                                     // 013 RUN_TEXT
	void VmTempText();                                    // 014 TEMP_TEXT
	void VmTextRead();                                    // 015 TEXT_READ
	void VmGameThingClicked();                            // 016 GAME_THING_CLICKED
	void VmSetScriptState();                              // 017 SET_SCRIPT_STATE
	void VmSetScriptStatePos();                           // 018 SET_SCRIPT_STATE_POS
	void VmSetScriptFloat();                              // 019 SET_SCRIPT_FLOAT
	void VmSetScriptUlong();                              // 020 SET_SCRIPT_ULONG
	void VmGetProperty();                                 // 021 GET_PROPERTY
	void VmSetProperty();                                 // 022 SET_PROPERTY
	void VmGetPosition();                                 // 023 GET_POSITION
	void VmSetPosition();                                 // 024 SET_POSITION
	void VmGetDistance();                                 // 025 GET_DISTANCE
	void VmCall();                                        // 026 CALL
	void VmCreate();                                      // 027 CREATE
	void VmRandom();                                      // 028 RANDOM
	void VmDllGettime();                                  // 029 DLL_GETTIME
	void VmStartCameraControl();                          // 030 START_CAMERA_CONTROL
	void VmEndCameraControl();                            // 031 END_CAMERA_CONTROL
	void VmSetWidescreen();                               // 032 SET_WIDESCREEN
	void VmMoveGameThing();                               // 033 MOVE_GAME_THING
	void VmSetFocus();                                    // 034 SET_FOCUS
	void VmHasCameraArrived();                            // 035 HAS_CAMERA_ARRIVED
	void VmFlockCreate();                                 // 036 FLOCK_CREATE
	void VmFlockAttach();                                 // 037 FLOCK_ATTACH
	void VmFlockDetach();                                 // 038 FLOCK_DETACH
	void VmFlockDisband();                                // 039 FLOCK_DISBAND
	void VmIdSize();                                      // 040 ID_SIZE
	void VmFlockMember();                                 // 041 FLOCK_MEMBER
	void VmGetHandPosition();                             // 042 GET_HAND_POSITION
	void VmPlaySoundEffect();                             // 043 PLAY_SOUND_EFFECT
	void VmStartMusic();                                  // 044 START_MUSIC
	void VmStopMusic();                                   // 045 STOP_MUSIC
	void VmAttachMusic();                                 // 046 ATTACH_MUSIC
	void VmDetachMusic();                                 // 047 DETACH_MUSIC
	void VmObjectDelete();                                // 048 OBJECT_DELETE
	void VmFocusFollow();                                 // 049 FOCUS_FOLLOW
	void VmPositionFollow();                              // 050 POSITION_FOLLOW
	void VmCallNear();                                    // 051 CALL_NEAR
	void VmSpecialEffectPosition();                       // 052 SPECIAL_EFFECT_POSITION
	void VmSpecialEffectObject();                         // 053 SPECIAL_EFFECT_OBJECT
	void VmDanceCreate();                                 // 054 DANCE_CREATE
	void VmCallIn();                                      // 055 CALL_IN
	void VmChangeInnerOuterProperties();                  // 056 CHANGE_INNER_OUTER_PROPERTIES
	void VmSnapshot();                                    // 057 SNAPSHOT
	void VmGetAlignment();                                // 058 GET_ALIGNMENT
	void VmSetAlignment();                                // 059 SET_ALIGNMENT
	void VmInfluenceObject();                             // 060 INFLUENCE_OBJECT
	void VmInfluencePosition();                           // 061 INFLUENCE_POSITION
	void VmGetInfluence();                                // 062 GET_INFLUENCE
	void VmSetInterfaceInteraction();                     // 063 SET_INTERFACE_INTERACTION
	void VmPlayed();                                      // 064 PLAYED
	void VmRandomUlong();                                 // 065 RANDOM_ULONG
	void VmSetGamespeed();                                // 066 SET_GAMESPEED
	void VmCallInNear();                                  // 067 CALL_IN_NEAR
	void VmOverrideStateAnimation();                      // 068 OVERRIDE_STATE_ANIMATION
	void VmCreatureCreateRelativeToCreature();            // 069 CREATURE_CREATE_RELATIVE_TO_CREATURE
	void VmCreatureLearnEverything();                     // 070 CREATURE_LEARN_EVERYTHING
	void VmCreatureSetKnowsAction();                      // 071 CREATURE_SET_KNOWS_ACTION
	void VmCreatureSetAgendaPriority();                   // 072 CREATURE_SET_AGENDA_PRIORITY
	void VmCreatureTurnOffAllDesires();                   // 073 CREATURE_TURN_OFF_ALL_DESIRES
	void VmCreatureLearnDistinctionAboutActivityObject(); // 074 CREATURE_LEARN_DISTINCTION_ABOUT_ACTIVITY_OBJECT
	void VmCreatureDoAction();                            // 075 CREATURE_DO_ACTION
	void VmInCreatureHand();                              // 076 IN_CREATURE_HAND
	void VmCreatureSetDesireValue();                      // 077 CREATURE_SET_DESIRE_VALUE
	void VmCreatureSetDesireActivated78();                // 078 CREATURE_SET_DESIRE_ACTIVATED
	void VmCreatureSetDesireActivated79();                // 079 CREATURE_SET_DESIRE_ACTIVATED
	void VmCreatureSetDesireMaximum();                    // 080 CREATURE_SET_DESIRE_MAXIMUM
	void VmConvertCameraPosition();                       // 081 CONVERT_CAMERA_POSITION
	void VmConvertCameraFocus();                          // 082 CONVERT_CAMERA_FOCUS
	void VmCreatureSetPlayer();                           // 083 CREATURE_SET_PLAYER
	void VmStartCountdownTimer();                         // 084 START_COUNTDOWN_TIMER
	void VmCreatureInitialiseNumTimesPerformedAction();   // 085 CREATURE_INITIALISE_NUM_TIMES_PERFORMED_ACTION
	void VmCreatureGetNumTimesActionPerformed();          // 086 CREATURE_GET_NUM_TIMES_ACTION_PERFORMED
	void VmRemoveCountdownTimer();                        // 087 REMOVE_COUNTDOWN_TIMER
	void VmGetObjectDropped();                            // 088 GET_OBJECT_DROPPED
	void VmClearDroppedByObject();                        // 089 CLEAR_DROPPED_BY_OBJECT
	void VmCreateReaction();                              // 090 CREATE_REACTION
	void VmRemoveReaction();                              // 091 REMOVE_REACTION
	void VmGetCountdownTimer();                           // 092 GET_COUNTDOWN_TIMER
	void VmStartDualCamera();                             // 093 START_DUAL_CAMERA
	void VmUpdateDualCamera();                            // 094 UPDATE_DUAL_CAMERA
	void VmReleaseDualCamera();                           // 095 RELEASE_DUAL_CAMERA
	void VmSetCreatureHelp();                             // 096 SET_CREATURE_HELP
	void VmGetTargetObject();                             // 097 GET_TARGET_OBJECT
	void VmCreatureDesireIs();                            // 098 CREATURE_DESIRE_IS
	void VmCountdownTimerExists();                        // 099 COUNTDOWN_TIMER_EXISTS
	void VmLookGameThing();                               // 100 LOOK_GAME_THING
	void VmGetObjectDestination();                        // 101 GET_OBJECT_DESTINATION
	void VmCreatureForceFinish();                         // 102 CREATURE_FORCE_FINISH
	void VmHideCountdownTimer();                          // 103 HIDE_COUNTDOWN_TIMER
	void VmGetActionTextForObject();                      // 104 GET_ACTION_TEXT_FOR_OBJECT
	void VmCreateDualCameraWithPoint();                   // 105 CREATE_DUAL_CAMERA_WITH_POINT
	void VmSetCameraToFaceObject();                       // 106 SET_CAMERA_TO_FACE_OBJECT
	void VmMoveCameraToFaceObject();                      // 107 MOVE_CAMERA_TO_FACE_OBJECT
	void VmGetMoonPercentage();                           // 108 GET_MOON_PERCENTAGE
	void VmPopulateContainer();                           // 109 POPULATE_CONTAINER
	void VmAddReference();                                // 110 ADD_REFERENCE
	void VmRemoveReference();                             // 111 REMOVE_REFERENCE
	void VmSetGameTime();                                 // 112 SET_GAME_TIME
	void VmGetGameTime();                                 // 113 GET_GAME_TIME
	void VmGetRealTime();                                 // 114 GET_REAL_TIME
	void VmGetRealDay115();                               // 115 GET_REAL_DAY
	void VmGetRealDay116();                               // 116 GET_REAL_DAY
	void VmGetRealMonth();                                // 117 GET_REAL_MONTH
	void VmGetRealYear();                                 // 118 GET_REAL_YEAR
	void VmRunCameraPath();                               // 119 RUN_CAMERA_PATH
	void VmStartDialogue();                               // 120 START_DIALOGUE
	void VmEndDialogue();                                 // 121 END_DIALOGUE
	void VmIsDialogueReady();                             // 122 IS_DIALOGUE_READY
	void VmChangeWeatherProperties();                     // 123 CHANGE_WEATHER_PROPERTIES
	void VmChangeLightningProperties();                   // 124 CHANGE_LIGHTNING_PROPERTIES
	void VmChangeTimeFadeProperties();                    // 125 CHANGE_TIME_FADE_PROPERTIES
	void VmChangeCloudProperties();                       // 126 CHANGE_CLOUD_PROPERTIES
	void VmSetHeadingAndSpeed();                          // 127 SET_HEADING_AND_SPEED
	void VmStartGameSpeed();                              // 128 START_GAME_SPEED
	void VmEndGameSpeed();                                // 129 END_GAME_SPEED
	void VmBuildBuilding();                               // 130 BUILD_BUILDING
	void VmSetAffectedByWind();                           // 131 SET_AFFECTED_BY_WIND
	void VmWidescreenTransistionFinished();               // 132 WIDESCREEN_TRANSISTION_FINISHED
	void VmGetResource();                                 // 133 GET_RESOURCE
	void VmAddResource();                                 // 134 ADD_RESOURCE
	void VmRemoveResource();                              // 135 REMOVE_RESOURCE
	void VmGetTargetRelativePos();                        // 136 GET_TARGET_RELATIVE_POS
	void VmStopPointing();                                // 137 STOP_POINTING
	void VmStopLooking();                                 // 138 STOP_LOOKING
	void VmLookAtPosition();                              // 139 LOOK_AT_POSITION
	void VmPlaySpiritAnim();                              // 140 PLAY_SPIRIT_ANIM
	void VmCallInNotNear();                               // 141 CALL_IN_NOT_NEAR
	void VmSetCameraZone();                               // 142 SET_CAMERA_ZONE
	void VmGetObjectState();                              // 143 GET_OBJECT_STATE
	void VmRevealCountdownTimer();                        // 144 REVEAL_COUNTDOWN_TIMER
	void VmSetTimerTime();                                // 145 SET_TIMER_TIME
	void VmCreateTimer();                                 // 146 CREATE_TIMER
	void VmGetTimerTimeRemaining();                       // 147 GET_TIMER_TIME_REMAINING
	void VmGetTimerTimeSinceSet();                        // 148 GET_TIMER_TIME_SINCE_SET
	void VmMoveMusic();                                   // 149 MOVE_MUSIC
	void VmGetInclusionDistance();                        // 150 GET_INCLUSION_DISTANCE
	void VmGetLandHeight();                               // 151 GET_LAND_HEIGHT
	void VmLoadMap();                                     // 152 LOAD_MAP
	void VmStopAllScriptsExcluding();                     // 153 STOP_ALL_SCRIPTS_EXCLUDING
	void VmStopAllScriptsInFilesExcluding();              // 154 STOP_ALL_SCRIPTS_IN_FILES_EXCLUDING
	void VmStopScript();                                  // 155 STOP_SCRIPT
	void VmClearClickedObject();                          // 156 CLEAR_CLICKED_OBJECT
	void VmClearClickedPosition();                        // 157 CLEAR_CLICKED_POSITION
	void VmPositionClicked();                             // 158 POSITION_CLICKED
	void VmReleaseFromScript();                           // 159 RELEASE_FROM_SCRIPT
	void VmGetObjectHandIsOver();                         // 160 GET_OBJECT_HAND_IS_OVER
	void VmIdPoisonedSize();                              // 161 ID_POISONED_SIZE
	void VmIsPoisoned();                                  // 162 IS_POISONED
	void VmCallPoisonedIn();                              // 163 CALL_POISONED_IN
	void VmCallNotPoisonedIn();                           // 164 CALL_NOT_POISONED_IN
	void VmSpiritPlayed();                                // 165 SPIRIT_PLAYED
	void VmClingSpirit();                                 // 166 CLING_SPIRIT
	void VmFlySpirit();                                   // 167 FLY_SPIRIT
	void VmSetIdMoveable();                               // 168 SET_ID_MOVEABLE
	void VmSetIdPickupable();                             // 169 SET_ID_PICKUPABLE
	void VmIsOnFire();                                    // 170 IS_ON_FIRE
	void VmIsFireNear();                                  // 171 IS_FIRE_NEAR
	void VmStopScriptsInFiles();                          // 172 STOP_SCRIPTS_IN_FILES
	void VmSetPoisoned();                                 // 173 SET_POISONED
	void VmSetTemperature();                              // 174 SET_TEMPERATURE
	void VmSetOnFire();                                   // 175 SET_ON_FIRE
	void VmSetTarget();                                   // 176 SET_TARGET
	void VmWalkPath();                                    // 177 WALK_PATH
	void VmFocusAndPositionFollow();                      // 178 FOCUS_AND_POSITION_FOLLOW
	void VmGetWalkPathPercentage();                       // 179 GET_WALK_PATH_PERCENTAGE
	void VmCameraProperties();                            // 180 CAMERA_PROPERTIES
	void VmEnableDisableMusic();                          // 181 ENABLE_DISABLE_MUSIC
	void VmGetMusicObjDistance();                         // 182 GET_MUSIC_OBJ_DISTANCE
	void VmGetMusicEnumDistance();                        // 183 GET_MUSIC_ENUM_DISTANCE
	void VmSetMusicPlayPosition();                        // 184 SET_MUSIC_PLAY_POSITION
	void VmAttachObjectLeashToObject();                   // 185 ATTACH_OBJECT_LEASH_TO_OBJECT
	void VmAttachObjectLeashToHand();                     // 186 ATTACH_OBJECT_LEASH_TO_HAND
	void VmDetachObjectLeash();                           // 187 DETACH_OBJECT_LEASH
	void VmSetCreatureOnlyDesire();                       // 188 SET_CREATURE_ONLY_DESIRE
	void VmSetCreatureOnlyDesireOff();                    // 189 SET_CREATURE_ONLY_DESIRE_OFF
	void VmRestartMusic();                                // 190 RESTART_MUSIC
	void VmMusicPlayed191();                              // 191 MUSIC_PLAYED
	void VmIsOfType();                                    // 192 IS_OF_TYPE
	void VmClearHitObject();                              // 193 CLEAR_HIT_OBJECT
	void VmGameThingHit();                                // 194 GAME_THING_HIT
	void VmSpellAtThing();                                // 195 SPELL_AT_THING
	void VmSpellAtPos();                                  // 196 SPELL_AT_POS
	void VmCallPlayerCreature();                          // 197 CALL_PLAYER_CREATURE
	void VmGetSlowestSpeed();                             // 198 GET_SLOWEST_SPEED
	void VmGetObjectHeld199();                            // 199 GET_OBJECT_HELD
	void VmHelpSystemOn();                                // 200 HELP_SYSTEM_ON
	void VmShakeCamera();                                 // 201 SHAKE_CAMERA
	void VmSetAnimationModify();                          // 202 SET_ANIMATION_MODIFY
	void VmSetAviSequence();                              // 203 SET_AVI_SEQUENCE
	void VmPlayGesture();                                 // 204 PLAY_GESTURE
	void VmDevFunction();                                 // 205 DEV_FUNCTION
	void VmHasMouseWheel();                               // 206 HAS_MOUSE_WHEEL
	void VmNumMouseButtons();                             // 207 NUM_MOUSE_BUTTONS
	void VmSetCreatureDevStage();                         // 208 SET_CREATURE_DEV_STAGE
	void VmSetFixedCamRotation();                         // 209 SET_FIXED_CAM_ROTATION
	void VmSwapCreature();                                // 210 SWAP_CREATURE
	void VmGetArena();                                    // 211 GET_ARENA
	void VmGetFootballPitch();                            // 212 GET_FOOTBALL_PITCH
	void VmStopAllGames();                                // 213 STOP_ALL_GAMES
	void VmAttachToGame();                                // 214 ATTACH_TO_GAME
	void VmDetachFromGame();                              // 215 DETACH_FROM_GAME
	void VmDetachUndefinedFromGame();                     // 216 DETACH_UNDEFINED_FROM_GAME
	void VmSetOnlyForScripts();                           // 217 SET_ONLY_FOR_SCRIPTS
	void VmStartMatchWithReferee();                       // 218 START_MATCH_WITH_REFEREE
	void VmGameTeamSize();                                // 219 GAME_TEAM_SIZE
	void VmGameType();                                    // 220 GAME_TYPE
	void VmGameSubType();                                 // 221 GAME_SUB_TYPE
	void VmIsLeashed();                                   // 222 IS_LEASHED
	void VmSetCreatureHome();                             // 223 SET_CREATURE_HOME
	void VmGetHitObject();                                // 224 GET_HIT_OBJECT
	void VmGetObjectWhichHit();                           // 225 GET_OBJECT_WHICH_HIT
	void VmGetNearestTownOfPlayer();                      // 226 GET_NEAREST_TOWN_OF_PLAYER
	void VmSpellAtPoint();                                // 227 SPELL_AT_POINT
	void VmSetAttackOwnTown();                            // 228 SET_ATTACK_OWN_TOWN
	void VmIsFighting();                                  // 229 IS_FIGHTING
	void VmSetMagicRadius();                              // 230 SET_MAGIC_RADIUS
	void VmTempTextWithNumber();                          // 231 TEMP_TEXT_WITH_NUMBER
	void VmRunTextWithNumber();                           // 232 RUN_TEXT_WITH_NUMBER
	void VmCreatureSpellReversion();                      // 233 CREATURE_SPELL_REVERSION
	void VmGetDesire();                                   // 234 GET_DESIRE
	void VmGetEventsPerSecond();                          // 235 GET_EVENTS_PER_SECOND
	void VmGetTimeSince();                                // 236 GET_TIME_SINCE
	void VmGetTotalEvents();                              // 237 GET_TOTAL_EVENTS
	void VmUpdateSnapshot();                              // 238 UPDATE_SNAPSHOT
	void VmCreateReward();                                // 239 CREATE_REWARD
	void VmCreateRewardInTown();                          // 240 CREATE_REWARD_IN_TOWN
	void VmSetFade();                                     // 241 SET_FADE
	void VmSetFadeIn();                                   // 242 SET_FADE_IN
	void VmFadeFinished();                                // 243 FADE_FINISHED
	void VmSetPlayerMagic();                              // 244 SET_PLAYER_MAGIC
	void VmHasPlayerMagic();                              // 245 HAS_PLAYER_MAGIC
	void VmSpiritSpeaks();                                // 246 SPIRIT_SPEAKS
	void VmBeliefForPlayer();                             // 247 BELIEF_FOR_PLAYER
	void VmGetHelp();                                     // 248 GET_HELP
	void VmSetLeashWorks();                               // 249 SET_LEASH_WORKS
	void VmLoadMyCreature();                              // 250 LOAD_MY_CREATURE
	void VmObjectRelativeBelief();                        // 251 OBJECT_RELATIVE_BELIEF
	void VmCreateWithAngleAndScale();                     // 252 CREATE_WITH_ANGLE_AND_SCALE
	void VmSetHelpSystem();                               // 253 SET_HELP_SYSTEM
	void VmSetVirtualInfluence();                         // 254 SET_VIRTUAL_INFLUENCE
	void VmSetActive();                                   // 255 SET_ACTIVE
	void VmThingValid();                                  // 256 THING_VALID
	void VmVortexFadeOut();                               // 257 VORTEX_FADE_OUT
	void VmRemoveReactionOfType();                        // 258 REMOVE_REACTION_OF_TYPE
	void VmCreatureLearnEverythingExcluding();            // 259 CREATURE_LEARN_EVERYTHING_EXCLUDING
	void VmPlayedPercentage();                            // 260 PLAYED_PERCENTAGE
	void VmObjectCastByObject();                          // 261 OBJECT_CAST_BY_OBJECT
	void VmIsWindMagicAtPos();                            // 262 IS_WIND_MAGIC_AT_POS
	void VmCreateMist();                                  // 263 CREATE_MIST
	void VmSetMistFade();                                 // 264 SET_MIST_FADE
	void VmGetObjectFade();                               // 265 GET_OBJECT_FADE
	void VmPlayHandDemo();                                // 266 PLAY_HAND_DEMO
	void VmIsPlayingHandDemo();                           // 267 IS_PLAYING_HAND_DEMO
	void VmGetArsePosition();                             // 268 GET_ARSE_POSITION
	void VmIsLeashedToObject();                           // 269 IS_LEASHED_TO_OBJECT
	void VmGetInteractionMagnitude();                     // 270 GET_INTERACTION_MAGNITUDE
	void VmIsCreatureAvailable();                         // 271 IS_CREATURE_AVAILABLE
	void VmCreateHighlight();                             // 272 CREATE_HIGHLIGHT
	void VmGetObjectHeld273();                            // 273 GET_OBJECT_HELD
	void VmGetActionCount();                              // 274 GET_ACTION_COUNT
	void VmGetObjectLeashType();                          // 275 GET_OBJECT_LEASH_TYPE
	void VmSetFocusFollow();                              // 276 SET_FOCUS_FOLLOW
	void VmSetPositionFollow();                           // 277 SET_POSITION_FOLLOW
	void VmSetFocusAndPositionFollow();                   // 278 SET_FOCUS_AND_POSITION_FOLLOW
	void VmSetCameraLens();                               // 279 SET_CAMERA_LENS
	void VmMoveCameraLens();                              // 280 MOVE_CAMERA_LENS
	void VmCreatureReaction();                            // 281 CREATURE_REACTION
	void VmCreatureInDevScript();                         // 282 CREATURE_IN_DEV_SCRIPT
	void VmStoreCameraDetails();                          // 283 STORE_CAMERA_DETAILS
	void VmRestoreCameraDetails();                        // 284 RESTORE_CAMERA_DETAILS
	void VmStartAngleSound285();                          // 285 START_ANGLE_SOUND
	void VmSetCameraPosFocLens();                         // 286 SET_CAMERA_POS_FOC_LENS
	void VmMoveCameraPosFocLens();                        // 287 MOVE_CAMERA_POS_FOC_LENS
	void VmGameTimeOnOff();                               // 288 GAME_TIME_ON_OFF
	void VmMoveGameTime();                                // 289 MOVE_GAME_TIME
	void VmSetHighGraphicsDetail();                       // 290 SET_HIGH_GRAPHICS_DETAIL
	void VmSetSkeleton();                                 // 291 SET_SKELETON
	void VmIsSkeleton();                                  // 292 IS_SKELETON
	void VmPlayerSpellCastTime();                         // 293 PLAYER_SPELL_CAST_TIME
	void VmPlayerSpellLastCast();                         // 294 PLAYER_SPELL_LAST_CAST
	void VmGetLastSpellCastPos();                         // 295 GET_LAST_SPELL_CAST_POS
	void VmAddSpotVisualTargetPos();                      // 296 ADD_SPOT_VISUAL_TARGET_POS
	void VmAddSpotVisualTargetObject();                   // 297 ADD_SPOT_VISUAL_TARGET_OBJECT
	void VmSetIndestructable();                           // 298 SET_INDESTRUCTABLE
	void VmSetGraphicsClipping();                         // 299 SET_GRAPHICS_CLIPPING
	void VmSpiritAppear();                                // 300 SPIRIT_APPEAR
	void VmSpiritDisappear();                             // 301 SPIRIT_DISAPPEAR
	void VmSetFocusOnObject();                            // 302 SET_FOCUS_ON_OBJECT
	void VmReleaseObjectFocus();                          // 303 RELEASE_OBJECT_FOCUS
	void VmImmersionExists();                             // 304 IMMERSION_EXISTS
	void VmSetDrawLeash();                                // 305 SET_DRAW_LEASH
	void VmSetDrawHighlight();                            // 306 SET_DRAW_HIGHLIGHT
	void VmSetOpenClose();                                // 307 SET_OPEN_CLOSE
	void VmSetIntroBuilding();                            // 308 SET_INTRO_BUILDING
	void VmCreatureForceFriends();                        // 309 CREATURE_FORCE_FRIENDS
	void VmMoveComputerPlayerPosition();                  // 310 MOVE_COMPUTER_PLAYER_POSITION
	void VmEnableDisableComputerPlayer311();              // 311 ENABLE_DISABLE_COMPUTER_PLAYER
	void VmGetComputerPlayerPosition();                   // 312 GET_COMPUTER_PLAYER_POSITION
	void VmSetComputerPlayerPosition();                   // 313 SET_COMPUTER_PLAYER_POSITION
	void VmGetStoredCameraPosition();                     // 314 GET_STORED_CAMERA_POSITION
	void VmGetStoredCameraFocus();                        // 315 GET_STORED_CAMERA_FOCUS
	void VmCallNearInState();                             // 316 CALL_NEAR_IN_STATE
	void VmSetCreatureSound();                            // 317 SET_CREATURE_SOUND
	void VmCreatureInteractingWith();                     // 318 CREATURE_INTERACTING_WITH
	void VmSetSunDraw();                                  // 319 SET_SUN_DRAW
	void VmObjectInfoBits();                              // 320 OBJECT_INFO_BITS
	void VmSetHurtByFire();                               // 321 SET_HURT_BY_FIRE
	void VmConfinedObject();                              // 322 CONFINED_OBJECT
	void VmClearConfinedObject();                         // 323 CLEAR_CONFINED_OBJECT
	void VmGetObjectFlock();                              // 324 GET_OBJECT_FLOCK
	void VmSetPlayerBelief();                             // 325 SET_PLAYER_BELIEF
	void VmPlayJcSpecial();                               // 326 PLAY_JC_SPECIAL
	void VmIsPlayingJcSpecial();                          // 327 IS_PLAYING_JC_SPECIAL
	void VmVortexParameters();                            // 328 VORTEX_PARAMETERS
	void VmLoadCreature();                                // 329 LOAD_CREATURE
	void VmIsSpellCharging();                             // 330 IS_SPELL_CHARGING
	void VmIsThatSpellCharging();                         // 331 IS_THAT_SPELL_CHARGING
	void VmOpposingCreature();                            // 332 OPPOSING_CREATURE
	void VmFlockWithinLimits();                           // 333 FLOCK_WITHIN_LIMITS
	void VmHighlightProperties();                         // 334 HIGHLIGHT_PROPERTIES
	void VmLastMusicLine();                               // 335 LAST_MUSIC_LINE
	void VmHandDemoTrigger();                             // 336 HAND_DEMO_TRIGGER
	void VmGetBellyPosition();                            // 337 GET_BELLY_POSITION
	void VmSetCreatureCreedProperties();                  // 338 SET_CREATURE_CREED_PROPERTIES
	void VmGameThingCanViewCamera();                      // 339 GAME_THING_CAN_VIEW_CAMERA
	void VmGamePlaySaySoundEffect();                      // 340 GAME_PLAY_SAY_SOUND_EFFECT
	void VmSetTownDesireBoost();                          // 341 SET_TOWN_DESIRE_BOOST
	void VmIsLockedInteraction();                         // 342 IS_LOCKED_INTERACTION
	void VmSetCreatureName();                             // 343 SET_CREATURE_NAME
	void VmComputerPlayerReady();                         // 344 COMPUTER_PLAYER_READY
	void VmEnableDisableComputerPlayer345();              // 345 ENABLE_DISABLE_COMPUTER_PLAYER
	void VmClearActorMind();                              // 346 CLEAR_ACTOR_MIND
	void VmEnterExitCitadel();                            // 347 ENTER_EXIT_CITADEL
	void VmStartAngleSound348();                          // 348 START_ANGLE_SOUND
	void VmThingJcSpecial();                              // 349 THING_JC_SPECIAL
	void VmMusicPlayed350();                              // 350 MUSIC_PLAYED
	void VmUpdateSnapshotPicture();                       // 351 UPDATE_SNAPSHOT_PICTURE
	void VmStopScriptsInFilesExcluding();                 // 352 STOP_SCRIPTS_IN_FILES_EXCLUDING
	void VmCreateRandomVillagerOfTribe();                 // 353 CREATE_RANDOM_VILLAGER_OF_TRIBE
	void VmToggleLeash();                                 // 354 TOGGLE_LEASH
	void VmGameSetMana();                                 // 355 GAME_SET_MANA
	void VmSetMagicProperties();                          // 356 SET_MAGIC_PROPERTIES
	void VmSetGameSound();                                // 357 SET_GAME_SOUND
	void VmSexIsMale();                                   // 358 SEX_IS_MALE
	void VmGetFirstHelp();                                // 359 GET_FIRST_HELP
	void VmGetLastHelp();                                 // 360 GET_LAST_HELP
	void VmIsActive();                                    // 361 IS_ACTIVE
	void VmSetBookmarkPosition();                         // 362 SET_BOOKMARK_POSITION
	void VmSetScaffoldProperties();                       // 363 SET_SCAFFOLD_PROPERTIES
	void VmSetComputerPlayerPersonality();                // 364 SET_COMPUTER_PLAYER_PERSONALITY
	void VmSetComputerPlayerSuppression();                // 365 SET_COMPUTER_PLAYER_SUPPRESSION
	void VmForceComputerPlayerAction();                   // 366 FORCE_COMPUTER_PLAYER_ACTION
	void VmQueueComputerPlayerAction();                   // 367 QUEUE_COMPUTER_PLAYER_ACTION
	void VmGetTownWithId();                               // 368 GET_TOWN_WITH_ID
	void VmSetDisciple();                                 // 369 SET_DISCIPLE
	void VmReleaseComputerPlayer();                       // 370 RELEASE_COMPUTER_PLAYER
	void VmSetComputerPlayerSpeed();                      // 371 SET_COMPUTER_PLAYER_SPEED
	void VmSetFocusFollowComputerPlayer();                // 372 SET_FOCUS_FOLLOW_COMPUTER_PLAYER
	void VmSetPositionFollowComputerPlayer();             // 373 SET_POSITION_FOLLOW_COMPUTER_PLAYER
	void VmCallComputerPlayer();                          // 374 CALL_COMPUTER_PLAYER
	void VmCallBuildingInTown();                          // 375 CALL_BUILDING_IN_TOWN
	void VmSetCanBuildWorshipsite();                      // 376 SET_CAN_BUILD_WORSHIPSITE
	void VmGetFacingCameraPosition();                     // 377 GET_FACING_CAMERA_POSITION
	void VmSetComputerPlayerAttitude();                   // 378 SET_COMPUTER_PLAYER_ATTITUDE
	void VmGetComputerPlayerAttitude();                   // 379 GET_COMPUTER_PLAYER_ATTITUDE
	void VmLoadComputerPlayerPersonality();               // 380 LOAD_COMPUTER_PLAYER_PERSONALITY
	void VmSaveComputerPlayerPersonality();               // 381 SAVE_COMPUTER_PLAYER_PERSONALITY
	void VmSetPlayerAlly();                               // 382 SET_PLAYER_ALLY
	void VmCallFlying();                                  // 383 CALL_FLYING
	void VmSetObjectFadeIn();                             // 384 SET_OBJECT_FADE_IN
	void VmIsAffectedBySpell();                           // 385 IS_AFFECTED_BY_SPELL
	void VmSetMagicInObject();                            // 386 SET_MAGIC_IN_OBJECT
	void VmIdAdultSize();                                 // 387 ID_ADULT_SIZE
	void VmObjectCapacity();                              // 388 OBJECT_CAPACITY
	void VmObjectAdultCapacity();                         // 389 OBJECT_ADULT_CAPACITY
	void VmSetCreatureAutoFighting();                     // 390 SET_CREATURE_AUTO_FIGHTING
	void VmIsAutoFighting();                              // 391 IS_AUTO_FIGHTING
	void VmSetCreatureQueueFightMove();                   // 392 SET_CREATURE_QUEUE_FIGHT_MOVE
	void VmSetCreatureQueueFightSpell();                  // 393 SET_CREATURE_QUEUE_FIGHT_SPELL
	void VmSetCreatureQueueFightStep();                   // 394 SET_CREATURE_QUEUE_FIGHT_STEP
	void VmGetCreatureFightAction();                      // 395 GET_CREATURE_FIGHT_ACTION
	void VmCreatureFightQueueHits();                      // 396 CREATURE_FIGHT_QUEUE_HITS
	void VmSquareRoot();                                  // 397 SQUARE_ROOT
	void VmGetPlayerAlly();                               // 398 GET_PLAYER_ALLY
	void VmSetPlayerWindResistance();                     // 399 SET_PLAYER_WIND_RESISTANCE
	void VmGetPlayerWindResistance();                     // 400 GET_PLAYER_WIND_RESISTANCE
	void VmPauseUnpauseClimateSystem();                   // 401 PAUSE_UNPAUSE_CLIMATE_SYSTEM
	void VmPauseUnpauseStormCreationInClimateSystem();    // 402 PAUSE_UNPAUSE_STORM_CREATION_IN_CLIMATE_SYSTEM
	void VmGetManaForSpell();                             // 403 GET_MANA_FOR_SPELL
	void VmKillStormsInArea();                            // 404 KILL_STORMS_IN_AREA
	void VmInsideTemple();                                // 405 INSIDE_TEMPLE
	void VmRestartObject();                               // 406 RESTART_OBJECT
	void VmSetGameTimeProperties();                       // 407 SET_GAME_TIME_PROPERTIES
	void VmResetGameTimeProperties();                     // 408 RESET_GAME_TIME_PROPERTIES
	void VmSoundExists();                                 // 409 SOUND_EXISTS
	void VmGetTownWorshipDeaths();                        // 410 GET_TOWN_WORSHIP_DEATHS
	void VmGameClearDialogue();                           // 411 GAME_CLEAR_DIALOGUE
	void VmGameCloseDialogue();                           // 412 GAME_CLOSE_DIALOGUE
	void VmGetHandState();                                // 413 GET_HAND_STATE
	void VmSetInterfaceCitadel();                         // 414 SET_INTERFACE_CITADEL
	void VmMapScriptFunction();                           // 415 MAP_SCRIPT_FUNCTION
	void VmWithinRotation();                              // 416 WITHIN_ROTATION
	void VmGetPlayerTownTotal();                          // 417 GET_PLAYER_TOWN_TOTAL
	void VmSpiritScreenPoint();                           // 418 SPIRIT_SCREEN_POINT
	void VmKeyDown();                                     // 419 KEY_DOWN
	void VmSetFightExit();                                // 420 SET_FIGHT_EXIT
	void VmGetObjectClicked();                            // 421 GET_OBJECT_CLICKED
	void VmGetMana();                                     // 422 GET_MANA
	void VmClearPlayerSpellCharging();                    // 423 CLEAR_PLAYER_SPELL_CHARGING
	void VmStopSoundEffect();                             // 424 STOP_SOUND_EFFECT
	void VmGetTotemStatue();                              // 425 GET_TOTEM_STATUE
	void VmSetSetOnFire();                                // 426 SET_SET_ON_FIRE
	void VmSetLandBalance();                              // 427 SET_LAND_BALANCE
	void VmSetObjectBeliefScale();                        // 428 SET_OBJECT_BELIEF_SCALE
	void VmStartImmersion();                              // 429 START_IMMERSION
	void VmStopImmersion();                               // 430 STOP_IMMERSION
	void VmStopAllImmersion();                            // 431 STOP_ALL_IMMERSION
	void VmSetCreatureInTemple();                         // 432 SET_CREATURE_IN_TEMPLE
	void VmGameDrawText();                                // 433 GAME_DRAW_TEXT
	void VmGameDrawTempText();                            // 434 GAME_DRAW_TEMP_TEXT
	void VmFadeAllDrawText();                             // 435 FADE_ALL_DRAW_TEXT
	void VmSetDrawTextColour();                           // 436 SET_DRAW_TEXT_COLOUR
	void VmSetClippingWindow();                           // 437 SET_CLIPPING_WINDOW
	void VmClearClippingWindow();                         // 438 CLEAR_CLIPPING_WINDOW
	void VmSaveGameInSlot();                              // 439 SAVE_GAME_IN_SLOT
	void VmSetObjectCarrying();                           // 440 SET_OBJECT_CARRYING
	void VmPosValidForCreature();                         // 441 POS_VALID_FOR_CREATURE
	void VmGetTimeSinceObjectAttacked();                  // 442 GET_TIME_SINCE_OBJECT_ATTACKED
	void VmGetTownAndVillagerHealthTotal();               // 443 GET_TOWN_AND_VILLAGER_HEALTH_TOTAL
	void VmGameAddForBuilding();                          // 444 GAME_ADD_FOR_BUILDING
	void VmEnableDisableAlignmentMusic();                 // 445 ENABLE_DISABLE_ALIGNMENT_MUSIC
	void VmGetDeadLiving();                               // 446 GET_DEAD_LIVING
	void VmAttachSoundTag();                              // 447 ATTACH_SOUND_TAG
	void VmDetachSoundTag();                              // 448 DETACH_SOUND_TAG
	void VmGetSacrificeTotal();                           // 449 GET_SACRIFICE_TOTAL
	void VmGameSoundPlaying();                            // 450 GAME_SOUND_PLAYING
	void VmGetTemplePosition();                           // 451 GET_TEMPLE_POSITION
	void VmCreatureAutoscale();                           // 452 CREATURE_AUTOSCALE
	void VmGetSpellIconInTemple();                        // 453 GET_SPELL_ICON_IN_TEMPLE
	void VmGameClearComputerPlayerActions();              // 454 GAME_CLEAR_COMPUTER_PLAYER_ACTIONS
	void VmGetFirstInContainer();                         // 455 GET_FIRST_IN_CONTAINER
	void VmGetNextInContainer();                          // 456 GET_NEXT_IN_CONTAINER
	void VmGetTempleEntrancePosition();                   // 457 GET_TEMPLE_ENTRANCE_POSITION
	void VmSaySoundEffectPlaying();                       // 458 SAY_SOUND_EFFECT_PLAYING
	void VmSetHandDemoKeys();                             // 459 SET_HAND_DEMO_KEYS
	void VmCanSkipTutorial();                             // 460 CAN_SKIP_TUTORIAL
	void VmCanSkipCreatureTraining();                     // 461 CAN_SKIP_CREATURE_TRAINING
	void VmIsKeepingOldCreature();                        // 462 IS_KEEPING_OLD_CREATURE
	void VmCurrentProfileHasCreature();                   // 463 CURRENT_PROFILE_HAS_CREATURE

	static Game* Instance() { return sInstance; }

private:
	static Game* sInstance;

	/// path to Lionhead Studios Ltd/Black & White folder
	const std::filesystem::path _gamePath;

	std::unique_ptr<Renderer> _renderer;
	std::unique_ptr<debug::gui::Gui> _gui;
	std::unique_ptr<Camera> _camera;
	std::unique_ptr<Profiler> _profiler;
	std::unique_ptr<EventManager> _eventManager;

	// std::unique_ptr<L3DMesh> _testModel;
	std::unique_ptr<L3DMesh> _testModel;
	std::unique_ptr<L3DAnim> _testAnimation;
	std::unique_ptr<L3DMesh> _handModel;
	std::unique_ptr<Sky> _sky;
	std::unique_ptr<Water> _water;
	std::unique_ptr<lhscriptx::Script> _scriptx;

	std::unique_ptr<LHVM::LHVM> _lhvm;
	ReferenceManager<entt::entity> _refManager;
	std::vector<LHVM::NativeFunction> _functionsTable;
	std::function<void(const uint32_t func)> _nativeCallEnterCallback;
	std::function<void(const uint32_t func)> _nativeCallExitCallback;
	std::function<void(const uint32_t taskNumber)> _stopTaskCallback;
	std::function<void(const LHVM::ErrorCode code, const std::string v0, const uint32_t v1)> _lhvmErrorCallback;
	std::function<void(const uint32_t objId)> _addObjectReference;
	std::function<void(const uint32_t objId)> _removeObjectReference;
	std::unordered_set<int32_t> _keysdown;

	InfoConstants _infoConstants;
	Config _config;
	std::filesystem::path _startMap;

	std::chrono::steady_clock::time_point _lastGameLoopTime;
	std::chrono::steady_clock::duration _turnDeltaTime;
	float _gameSpeedMultiplier {1.0f};
	uint32_t _frameCount {0};
	uint16_t _turnCount {0};
	bool _paused {true};

	glm::ivec2 _mousePosition;
	glm::mat4 _handPose;

	entt::entity _handEntity;
	bool _handGripping;

	std::optional<std::pair</* frame number */ uint32_t, /* output */ std::filesystem::path>> _requestScreenshot;
};
} // namespace openblack
