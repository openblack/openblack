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
struct NativeFunction;
enum class ErrorCode : uint32_t;
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

	void NativeCallEnterCallback(const uint32_t func);
	void NativeCallExitCallback(const uint32_t func);
	void StopTaskCallback(const uint32_t taskNumber);
	void LhvmErrorCallback(const LHVM::ErrorCode code, const std::string v0, const uint32_t v1);
	void AddObjectReference(const uint32_t objId);
	void RemoveObjectReference(const uint32_t objId);

	[[nodiscard]] glm::vec3 PopVec();
	void PushVec(const glm::vec3& vec);
	[[nodiscard]] std::string PopString();
	[[nodiscard]] std::vector<float_t> PopVarArg(const int32_t argc);
	[[nodiscard]] entt::entity CreateScriptObject(const script::ObjectType type, uint32_t subtype, const glm::vec3& position,
	                                              float altitude, float xAngleRadians, float yAngleRadians,
	                                              const float zAngleRadians, const float scale);

#define CREATE_FUNCTION_BINDING(NAME, STACKIN, STACKOUT, FUNCTION)                                              \
	{                                                                                                           \
		_functionsTable.emplace_back(LHVM::NativeFunction {                                                     \
		    .impl = std::bind(&Game::FUNCTION, this), .stackIn = STACKIN, .stackOut = STACKOUT, .name = NAME}); \
	}

	void InitFunctionsTable();

	void Native000_NONE();
	void Native001_SET_CAMERA_POSITION();
	void Native002_SET_CAMERA_FOCUS();
	void Native003_MOVE_CAMERA_POSITION();
	void Native004_MOVE_CAMERA_FOCUS();
	void Native005_GET_CAMERA_POSITION();
	void Native006_GET_CAMERA_FOCUS();
	void Native007_SPIRIT_EJECT();
	void Native008_SPIRIT_HOME();
	void Native009_SPIRIT_POINT_POS();
	void Native010_SPIRIT_POINT_GAME_THING();
	void Native011_GAME_THING_FIELD_OF_VIEW();
	void Native012_POS_FIELD_OF_VIEW();
	void Native013_RUN_TEXT();
	void Native014_TEMP_TEXT();
	void Native015_TEXT_READ();
	void Native016_GAME_THING_CLICKED();
	void Native017_SET_SCRIPT_STATE();
	void Native018_SET_SCRIPT_STATE_POS();
	void Native019_SET_SCRIPT_FLOAT();
	void Native020_SET_SCRIPT_ULONG();
	void Native021_GET_PROPERTY();
	void Native022_SET_PROPERTY();
	void Native023_GET_POSITION();
	void Native024_SET_POSITION();
	void Native025_GET_DISTANCE();
	void Native026_CALL();
	void Native027_CREATE();
	void Native028_RANDOM();
	void Native029_DLL_GETTIME();
	void Native030_START_CAMERA_CONTROL();
	void Native031_END_CAMERA_CONTROL();
	void Native032_SET_WIDESCREEN();
	void Native033_MOVE_GAME_THING();
	void Native034_SET_FOCUS();
	void Native035_HAS_CAMERA_ARRIVED();
	void Native036_FLOCK_CREATE();
	void Native037_FLOCK_ATTACH();
	void Native038_FLOCK_DETACH();
	void Native039_FLOCK_DISBAND();
	void Native040_ID_SIZE();
	void Native041_FLOCK_MEMBER();
	void Native042_GET_HAND_POSITION();
	void Native043_PLAY_SOUND_EFFECT();
	void Native044_START_MUSIC();
	void Native045_STOP_MUSIC();
	void Native046_ATTACH_MUSIC();
	void Native047_DETACH_MUSIC();
	void Native048_OBJECT_DELETE();
	void Native049_FOCUS_FOLLOW();
	void Native050_POSITION_FOLLOW();
	void Native051_CALL_NEAR();
	void Native052_SPECIAL_EFFECT_POSITION();
	void Native053_SPECIAL_EFFECT_OBJECT();
	void Native054_DANCE_CREATE();
	void Native055_CALL_IN();
	void Native056_CHANGE_INNER_OUTER_PROPERTIES();
	void Native057_SNAPSHOT();
	void Native058_GET_ALIGNMENT();
	void Native059_SET_ALIGNMENT();
	void Native060_INFLUENCE_OBJECT();
	void Native061_INFLUENCE_POSITION();
	void Native062_GET_INFLUENCE();
	void Native063_SET_INTERFACE_INTERACTION();
	void Native064_PLAYED();
	void Native065_RANDOM_ULONG();
	void Native066_SET_GAMESPEED();
	void Native067_CALL_IN_NEAR();
	void Native068_OVERRIDE_STATE_ANIMATION();
	void Native069_CREATURE_CREATE_RELATIVE_TO_CREATURE();
	void Native070_CREATURE_LEARN_EVERYTHING();
	void Native071_CREATURE_SET_KNOWS_ACTION();
	void Native072_CREATURE_SET_AGENDA_PRIORITY();
	void Native073_CREATURE_TURN_OFF_ALL_DESIRES();
	void Native074_CREATURE_LEARN_DISTINCTION_ABOUT_ACTIVITY_OBJECT();
	void Native075_CREATURE_DO_ACTION();
	void Native076_IN_CREATURE_HAND();
	void Native077_CREATURE_SET_DESIRE_VALUE();
	void Native078_CREATURE_SET_DESIRE_ACTIVATED();
	void Native079_CREATURE_SET_DESIRE_ACTIVATED();
	void Native080_CREATURE_SET_DESIRE_MAXIMUM();
	void Native081_CONVERT_CAMERA_POSITION();
	void Native082_CONVERT_CAMERA_FOCUS();
	void Native083_CREATURE_SET_PLAYER();
	void Native084_START_COUNTDOWN_TIMER();
	void Native085_CREATURE_INITIALISE_NUM_TIMES_PERFORMED_ACTION();
	void Native086_CREATURE_GET_NUM_TIMES_ACTION_PERFORMED();
	void Native087_REMOVE_COUNTDOWN_TIMER();
	void Native088_GET_OBJECT_DROPPED();
	void Native089_CLEAR_DROPPED_BY_OBJECT();
	void Native090_CREATE_REACTION();
	void Native091_REMOVE_REACTION();
	void Native092_GET_COUNTDOWN_TIMER();
	void Native093_START_DUAL_CAMERA();
	void Native094_UPDATE_DUAL_CAMERA();
	void Native095_RELEASE_DUAL_CAMERA();
	void Native096_SET_CREATURE_HELP();
	void Native097_GET_TARGET_OBJECT();
	void Native098_CREATURE_DESIRE_IS();
	void Native099_COUNTDOWN_TIMER_EXISTS();
	void Native100_LOOK_GAME_THING();
	void Native101_GET_OBJECT_DESTINATION();
	void Native102_CREATURE_FORCE_FINISH();
	void Native103_HIDE_COUNTDOWN_TIMER();
	void Native104_GET_ACTION_TEXT_FOR_OBJECT();
	void Native105_CREATE_DUAL_CAMERA_WITH_POINT();
	void Native106_SET_CAMERA_TO_FACE_OBJECT();
	void Native107_MOVE_CAMERA_TO_FACE_OBJECT();
	void Native108_GET_MOON_PERCENTAGE();
	void Native109_POPULATE_CONTAINER();
	void Native110_ADD_REFERENCE();
	void Native111_REMOVE_REFERENCE();
	void Native112_SET_GAME_TIME();
	void Native113_GET_GAME_TIME();
	void Native114_GET_REAL_TIME();
	void Native115_GET_REAL_DAY();
	void Native116_GET_REAL_DAY();
	void Native117_GET_REAL_MONTH();
	void Native118_GET_REAL_YEAR();
	void Native119_RUN_CAMERA_PATH();
	void Native120_START_DIALOGUE();
	void Native121_END_DIALOGUE();
	void Native122_IS_DIALOGUE_READY();
	void Native123_CHANGE_WEATHER_PROPERTIES();
	void Native124_CHANGE_LIGHTNING_PROPERTIES();
	void Native125_CHANGE_TIME_FADE_PROPERTIES();
	void Native126_CHANGE_CLOUD_PROPERTIES();
	void Native127_SET_HEADING_AND_SPEED();
	void Native128_START_GAME_SPEED();
	void Native129_END_GAME_SPEED();
	void Native130_BUILD_BUILDING();
	void Native131_SET_AFFECTED_BY_WIND();
	void Native132_WIDESCREEN_TRANSISTION_FINISHED();
	void Native133_GET_RESOURCE();
	void Native134_ADD_RESOURCE();
	void Native135_REMOVE_RESOURCE();
	void Native136_GET_TARGET_RELATIVE_POS();
	void Native137_STOP_POINTING();
	void Native138_STOP_LOOKING();
	void Native139_LOOK_AT_POSITION();
	void Native140_PLAY_SPIRIT_ANIM();
	void Native141_CALL_IN_NOT_NEAR();
	void Native142_SET_CAMERA_ZONE();
	void Native143_GET_OBJECT_STATE();
	void Native144_REVEAL_COUNTDOWN_TIMER();
	void Native145_SET_TIMER_TIME();
	void Native146_CREATE_TIMER();
	void Native147_GET_TIMER_TIME_REMAINING();
	void Native148_GET_TIMER_TIME_SINCE_SET();
	void Native149_MOVE_MUSIC();
	void Native150_GET_INCLUSION_DISTANCE();
	void Native151_GET_LAND_HEIGHT();
	void Native152_LOAD_MAP();
	void Native153_STOP_ALL_SCRIPTS_EXCLUDING();
	void Native154_STOP_ALL_SCRIPTS_IN_FILES_EXCLUDING();
	void Native155_STOP_SCRIPT();
	void Native156_CLEAR_CLICKED_OBJECT();
	void Native157_CLEAR_CLICKED_POSITION();
	void Native158_POSITION_CLICKED();
	void Native159_RELEASE_FROM_SCRIPT();
	void Native160_GET_OBJECT_HAND_IS_OVER();
	void Native161_ID_POISONED_SIZE();
	void Native162_IS_POISONED();
	void Native163_CALL_POISONED_IN();
	void Native164_CALL_NOT_POISONED_IN();
	void Native165_SPIRIT_PLAYED();
	void Native166_CLING_SPIRIT();
	void Native167_FLY_SPIRIT();
	void Native168_SET_ID_MOVEABLE();
	void Native169_SET_ID_PICKUPABLE();
	void Native170_IS_ON_FIRE();
	void Native171_IS_FIRE_NEAR();
	void Native172_STOP_SCRIPTS_IN_FILES();
	void Native173_SET_POISONED();
	void Native174_SET_TEMPERATURE();
	void Native175_SET_ON_FIRE();
	void Native176_SET_TARGET();
	void Native177_WALK_PATH();
	void Native178_FOCUS_AND_POSITION_FOLLOW();
	void Native179_GET_WALK_PATH_PERCENTAGE();
	void Native180_CAMERA_PROPERTIES();
	void Native181_ENABLE_DISABLE_MUSIC();
	void Native182_GET_MUSIC_OBJ_DISTANCE();
	void Native183_GET_MUSIC_ENUM_DISTANCE();
	void Native184_SET_MUSIC_PLAY_POSITION();
	void Native185_ATTACH_OBJECT_LEASH_TO_OBJECT();
	void Native186_ATTACH_OBJECT_LEASH_TO_HAND();
	void Native187_DETACH_OBJECT_LEASH();
	void Native188_SET_CREATURE_ONLY_DESIRE();
	void Native189_SET_CREATURE_ONLY_DESIRE_OFF();
	void Native190_RESTART_MUSIC();
	void Native191_MUSIC_PLAYED();
	void Native192_IS_OF_TYPE();
	void Native193_CLEAR_HIT_OBJECT();
	void Native194_GAME_THING_HIT();
	void Native195_SPELL_AT_THING();
	void Native196_SPELL_AT_POS();
	void Native197_CALL_PLAYER_CREATURE();
	void Native198_GET_SLOWEST_SPEED();
	void Native199_GET_OBJECT_HELD();
	void Native200_HELP_SYSTEM_ON();
	void Native201_SHAKE_CAMERA();
	void Native202_SET_ANIMATION_MODIFY();
	void Native203_SET_AVI_SEQUENCE();
	void Native204_PLAY_GESTURE();
	void Native205_DEV_FUNCTION();
	void Native206_HAS_MOUSE_WHEEL();
	void Native207_NUM_MOUSE_BUTTONS();
	void Native208_SET_CREATURE_DEV_STAGE();
	void Native209_SET_FIXED_CAM_ROTATION();
	void Native210_SWAP_CREATURE();
	void Native211_GET_ARENA();
	void Native212_GET_FOOTBALL_PITCH();
	void Native213_STOP_ALL_GAMES();
	void Native214_ATTACH_TO_GAME();
	void Native215_DETACH_FROM_GAME();
	void Native216_DETACH_UNDEFINED_FROM_GAME();
	void Native217_SET_ONLY_FOR_SCRIPTS();
	void Native218_START_MATCH_WITH_REFEREE();
	void Native219_GAME_TEAM_SIZE();
	void Native220_GAME_TYPE();
	void Native221_GAME_SUB_TYPE();
	void Native222_IS_LEASHED();
	void Native223_SET_CREATURE_HOME();
	void Native224_GET_HIT_OBJECT();
	void Native225_GET_OBJECT_WHICH_HIT();
	void Native226_GET_NEAREST_TOWN_OF_PLAYER();
	void Native227_SPELL_AT_POINT();
	void Native228_SET_ATTACK_OWN_TOWN();
	void Native229_IS_FIGHTING();
	void Native230_SET_MAGIC_RADIUS();
	void Native231_TEMP_TEXT_WITH_NUMBER();
	void Native232_RUN_TEXT_WITH_NUMBER();
	void Native233_CREATURE_SPELL_REVERSION();
	void Native234_GET_DESIRE();
	void Native235_GET_EVENTS_PER_SECOND();
	void Native236_GET_TIME_SINCE();
	void Native237_GET_TOTAL_EVENTS();
	void Native238_UPDATE_SNAPSHOT();
	void Native239_CREATE_REWARD();
	void Native240_CREATE_REWARD_IN_TOWN();
	void Native241_SET_FADE();
	void Native242_SET_FADE_IN();
	void Native243_FADE_FINISHED();
	void Native244_SET_PLAYER_MAGIC();
	void Native245_HAS_PLAYER_MAGIC();
	void Native246_SPIRIT_SPEAKS();
	void Native247_BELIEF_FOR_PLAYER();
	void Native248_GET_HELP();
	void Native249_SET_LEASH_WORKS();
	void Native250_LOAD_MY_CREATURE();
	void Native251_OBJECT_RELATIVE_BELIEF();
	void Native252_CREATE_WITH_ANGLE_AND_SCALE();
	void Native253_SET_HELP_SYSTEM();
	void Native254_SET_VIRTUAL_INFLUENCE();
	void Native255_SET_ACTIVE();
	void Native256_THING_VALID();
	void Native257_VORTEX_FADE_OUT();
	void Native258_REMOVE_REACTION_OF_TYPE();
	void Native259_CREATURE_LEARN_EVERYTHING_EXCLUDING();
	void Native260_PLAYED_PERCENTAGE();
	void Native261_OBJECT_CAST_BY_OBJECT();
	void Native262_IS_WIND_MAGIC_AT_POS();
	void Native263_CREATE_MIST();
	void Native264_SET_MIST_FADE();
	void Native265_GET_OBJECT_FADE();
	void Native266_PLAY_HAND_DEMO();
	void Native267_IS_PLAYING_HAND_DEMO();
	void Native268_GET_ARSE_POSITION();
	void Native269_IS_LEASHED_TO_OBJECT();
	void Native270_GET_INTERACTION_MAGNITUDE();
	void Native271_IS_CREATURE_AVAILABLE();
	void Native272_CREATE_HIGHLIGHT();
	void Native273_GET_OBJECT_HELD();
	void Native274_GET_ACTION_COUNT();
	void Native275_GET_OBJECT_LEASH_TYPE();
	void Native276_SET_FOCUS_FOLLOW();
	void Native277_SET_POSITION_FOLLOW();
	void Native278_SET_FOCUS_AND_POSITION_FOLLOW();
	void Native279_SET_CAMERA_LENS();
	void Native280_MOVE_CAMERA_LENS();
	void Native281_CREATURE_REACTION();
	void Native282_CREATURE_IN_DEV_SCRIPT();
	void Native283_STORE_CAMERA_DETAILS();
	void Native284_RESTORE_CAMERA_DETAILS();
	void Native285_START_ANGLE_SOUND();
	void Native286_SET_CAMERA_POS_FOC_LENS();
	void Native287_MOVE_CAMERA_POS_FOC_LENS();
	void Native288_GAME_TIME_ON_OFF();
	void Native289_MOVE_GAME_TIME();
	void Native290_SET_HIGH_GRAPHICS_DETAIL();
	void Native291_SET_SKELETON();
	void Native292_IS_SKELETON();
	void Native293_PLAYER_SPELL_CAST_TIME();
	void Native294_PLAYER_SPELL_LAST_CAST();
	void Native295_GET_LAST_SPELL_CAST_POS();
	void Native296_ADD_SPOT_VISUAL_TARGET_POS();
	void Native297_ADD_SPOT_VISUAL_TARGET_OBJECT();
	void Native298_SET_INDESTRUCTABLE();
	void Native299_SET_GRAPHICS_CLIPPING();
	void Native300_SPIRIT_APPEAR();
	void Native301_SPIRIT_DISAPPEAR();
	void Native302_SET_FOCUS_ON_OBJECT();
	void Native303_RELEASE_OBJECT_FOCUS();
	void Native304_IMMERSION_EXISTS();
	void Native305_SET_DRAW_LEASH();
	void Native306_SET_DRAW_HIGHLIGHT();
	void Native307_SET_OPEN_CLOSE();
	void Native308_SET_INTRO_BUILDING();
	void Native309_CREATURE_FORCE_FRIENDS();
	void Native310_MOVE_COMPUTER_PLAYER_POSITION();
	void Native311_ENABLE_DISABLE_COMPUTER_PLAYER();
	void Native312_GET_COMPUTER_PLAYER_POSITION();
	void Native313_SET_COMPUTER_PLAYER_POSITION();
	void Native314_GET_STORED_CAMERA_POSITION();
	void Native315_GET_STORED_CAMERA_FOCUS();
	void Native316_CALL_NEAR_IN_STATE();
	void Native317_SET_CREATURE_SOUND();
	void Native318_CREATURE_INTERACTING_WITH();
	void Native319_SET_SUN_DRAW();
	void Native320_OBJECT_INFO_BITS();
	void Native321_SET_HURT_BY_FIRE();
	void Native322_CONFINED_OBJECT();
	void Native323_CLEAR_CONFINED_OBJECT();
	void Native324_GET_OBJECT_FLOCK();
	void Native325_SET_PLAYER_BELIEF();
	void Native326_PLAY_JC_SPECIAL();
	void Native327_IS_PLAYING_JC_SPECIAL();
	void Native328_VORTEX_PARAMETERS();
	void Native329_LOAD_CREATURE();
	void Native330_IS_SPELL_CHARGING();
	void Native331_IS_THAT_SPELL_CHARGING();
	void Native332_OPPOSING_CREATURE();
	void Native333_FLOCK_WITHIN_LIMITS();
	void Native334_HIGHLIGHT_PROPERTIES();
	void Native335_LAST_MUSIC_LINE();
	void Native336_HAND_DEMO_TRIGGER();
	void Native337_GET_BELLY_POSITION();
	void Native338_SET_CREATURE_CREED_PROPERTIES();
	void Native339_GAME_THING_CAN_VIEW_CAMERA();
	void Native340_GAME_PLAY_SAY_SOUND_EFFECT();
	void Native341_SET_TOWN_DESIRE_BOOST();
	void Native342_IS_LOCKED_INTERACTION();
	void Native343_SET_CREATURE_NAME();
	void Native344_COMPUTER_PLAYER_READY();
	void Native345_ENABLE_DISABLE_COMPUTER_PLAYER();
	void Native346_CLEAR_ACTOR_MIND();
	void Native347_ENTER_EXIT_CITADEL();
	void Native348_START_ANGLE_SOUND();
	void Native349_THING_JC_SPECIAL();
	void Native350_MUSIC_PLAYED();
	void Native351_UPDATE_SNAPSHOT_PICTURE();
	void Native352_STOP_SCRIPTS_IN_FILES_EXCLUDING();
	void Native353_CREATE_RANDOM_VILLAGER_OF_TRIBE();
	void Native354_TOGGLE_LEASH();
	void Native355_GAME_SET_MANA();
	void Native356_SET_MAGIC_PROPERTIES();
	void Native357_SET_GAME_SOUND();
	void Native358_SEX_IS_MALE();
	void Native359_GET_FIRST_HELP();
	void Native360_GET_LAST_HELP();
	void Native361_IS_ACTIVE();
	void Native362_SET_BOOKMARK_POSITION();
	void Native363_SET_SCAFFOLD_PROPERTIES();
	void Native364_SET_COMPUTER_PLAYER_PERSONALITY();
	void Native365_SET_COMPUTER_PLAYER_SUPPRESSION();
	void Native366_FORCE_COMPUTER_PLAYER_ACTION();
	void Native367_QUEUE_COMPUTER_PLAYER_ACTION();
	void Native368_GET_TOWN_WITH_ID();
	void Native369_SET_DISCIPLE();
	void Native370_RELEASE_COMPUTER_PLAYER();
	void Native371_SET_COMPUTER_PLAYER_SPEED();
	void Native372_SET_FOCUS_FOLLOW_COMPUTER_PLAYER();
	void Native373_SET_POSITION_FOLLOW_COMPUTER_PLAYER();
	void Native374_CALL_COMPUTER_PLAYER();
	void Native375_CALL_BUILDING_IN_TOWN();
	void Native376_SET_CAN_BUILD_WORSHIPSITE();
	void Native377_GET_FACING_CAMERA_POSITION();
	void Native378_SET_COMPUTER_PLAYER_ATTITUDE();
	void Native379_GET_COMPUTER_PLAYER_ATTITUDE();
	void Native380_LOAD_COMPUTER_PLAYER_PERSONALITY();
	void Native381_SAVE_COMPUTER_PLAYER_PERSONALITY();
	void Native382_SET_PLAYER_ALLY();
	void Native383_CALL_FLYING();
	void Native384_SET_OBJECT_FADE_IN();
	void Native385_IS_AFFECTED_BY_SPELL();
	void Native386_SET_MAGIC_IN_OBJECT();
	void Native387_ID_ADULT_SIZE();
	void Native388_OBJECT_CAPACITY();
	void Native389_OBJECT_ADULT_CAPACITY();
	void Native390_SET_CREATURE_AUTO_FIGHTING();
	void Native391_IS_AUTO_FIGHTING();
	void Native392_SET_CREATURE_QUEUE_FIGHT_MOVE();
	void Native393_SET_CREATURE_QUEUE_FIGHT_SPELL();
	void Native394_SET_CREATURE_QUEUE_FIGHT_STEP();
	void Native395_GET_CREATURE_FIGHT_ACTION();
	void Native396_CREATURE_FIGHT_QUEUE_HITS();
	void Native397_SQUARE_ROOT();
	void Native398_GET_PLAYER_ALLY();
	void Native399_SET_PLAYER_WIND_RESISTANCE();
	void Native400_GET_PLAYER_WIND_RESISTANCE();
	void Native401_PAUSE_UNPAUSE_CLIMATE_SYSTEM();
	void Native402_PAUSE_UNPAUSE_STORM_CREATION_IN_CLIMATE_SYSTEM();
	void Native403_GET_MANA_FOR_SPELL();
	void Native404_KILL_STORMS_IN_AREA();
	void Native405_INSIDE_TEMPLE();
	void Native406_RESTART_OBJECT();
	void Native407_SET_GAME_TIME_PROPERTIES();
	void Native408_RESET_GAME_TIME_PROPERTIES();
	void Native409_SOUND_EXISTS();
	void Native410_GET_TOWN_WORSHIP_DEATHS();
	void Native411_GAME_CLEAR_DIALOGUE();
	void Native412_GAME_CLOSE_DIALOGUE();
	void Native413_GET_HAND_STATE();
	void Native414_SET_INTERFACE_CITADEL();
	void Native415_MAP_SCRIPT_FUNCTION();
	void Native416_WITHIN_ROTATION();
	void Native417_GET_PLAYER_TOWN_TOTAL();
	void Native418_SPIRIT_SCREEN_POINT();
	void Native419_KEY_DOWN();
	void Native420_SET_FIGHT_EXIT();
	void Native421_GET_OBJECT_CLICKED();
	void Native422_GET_MANA();
	void Native423_CLEAR_PLAYER_SPELL_CHARGING();
	void Native424_STOP_SOUND_EFFECT();
	void Native425_GET_TOTEM_STATUE();
	void Native426_SET_SET_ON_FIRE();
	void Native427_SET_LAND_BALANCE();
	void Native428_SET_OBJECT_BELIEF_SCALE();
	void Native429_START_IMMERSION();
	void Native430_STOP_IMMERSION();
	void Native431_STOP_ALL_IMMERSION();
	void Native432_SET_CREATURE_IN_TEMPLE();
	void Native433_GAME_DRAW_TEXT();
	void Native434_GAME_DRAW_TEMP_TEXT();
	void Native435_FADE_ALL_DRAW_TEXT();
	void Native436_SET_DRAW_TEXT_COLOUR();
	void Native437_SET_CLIPPING_WINDOW();
	void Native438_CLEAR_CLIPPING_WINDOW();
	void Native439_SAVE_GAME_IN_SLOT();
	void Native440_SET_OBJECT_CARRYING();
	void Native441_POS_VALID_FOR_CREATURE();
	void Native442_GET_TIME_SINCE_OBJECT_ATTACKED();
	void Native443_GET_TOWN_AND_VILLAGER_HEALTH_TOTAL();
	void Native444_GAME_ADD_FOR_BUILDING();
	void Native445_ENABLE_DISABLE_ALIGNMENT_MUSIC();
	void Native446_GET_DEAD_LIVING();
	void Native447_ATTACH_SOUND_TAG();
	void Native448_DETACH_SOUND_TAG();
	void Native449_GET_SACRIFICE_TOTAL();
	void Native450_GAME_SOUND_PLAYING();
	void Native451_GET_TEMPLE_POSITION();
	void Native452_CREATURE_AUTOSCALE();
	void Native453_GET_SPELL_ICON_IN_TEMPLE();
	void Native454_GAME_CLEAR_COMPUTER_PLAYER_ACTIONS();
	void Native455_GET_FIRST_IN_CONTAINER();
	void Native456_GET_NEXT_IN_CONTAINER();
	void Native457_GET_TEMPLE_ENTRANCE_POSITION();
	void Native458_SAY_SOUND_EFFECT_PLAYING();
	void Native459_SET_HAND_DEMO_KEYS();
	void Native460_CAN_SKIP_TUTORIAL();
	void Native461_CAN_SKIP_CREATURE_TRAINING();
	void Native462_IS_KEEPING_OLD_CREATURE();
	void Native463_CURRENT_PROFILE_HAS_CREATURE();

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
