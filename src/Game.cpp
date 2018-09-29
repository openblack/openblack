#include "Game.h"

#include <iostream>
#include <sstream>

#include <SDL.h>
#define GLEW_STATIC
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <3D/Camera.h>
#include <3D/LandIsland.h>
#include <3D/oldLH3DIsland.h>

#include <Common/OSFile.h>

#include <Graphics/GameShaders.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

#include <3D/Mesh.h>
#include <3D/MeshPack.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>

using namespace OpenBlack;
using namespace OpenBlack::Graphics;

void GLAPIENTRY OpenGLMsgCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

void checkSDLError(int ret)
{
    if (ret != 0)
        std::cerr << "SDL error: " << SDL_GetError() << std::endl;
}

Game::Game(int argc, char **argv)
    : mWindow(NULL),
    mGLContext(NULL)
{
    uint32_t flags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER;
    if (SDL_WasInit(flags) == 0)
    {
        SDL_SetMainReady();
        if (SDL_Init(flags) != 0)
            throw std::runtime_error("Could not initialize SDL! " + std::string(SDL_GetError()));
    }

    createWindow(1280, 720);
    createContext();

    std::cout << "Game Path: " << GetGamePath() << std::endl;
}

Game::~Game()
{
    SDL_GL_DeleteContext(mGLContext);

    if (mWindow)
    {
        SDL_DestroyWindow(mWindow);
        mWindow = NULL;
    }

    SDL_Quit();
}

Texture2D* CreateCountryTexture()
{
    uint8_t* data = new uint8_t[256 * 256 * 3];

    for (uint32_t x = 0; x < LH3DIsland::g_country_count; x++)
    {
        LNDCountry country = LH3DIsland::g_countries[x];

        for (int y = 0; y < 256; y++)
        {
            data[(y * 256 * 3) + (x * 3) + 0] = (uint8_t)country.MapMaterial[y].FirstMaterialIndex;
            data[(y * 256 * 3) + (x * 3) + 1] = (uint8_t)country.MapMaterial[y].SecondMaterialIndex;
            data[(y * 256 * 3) + (x * 3) + 2] = (uint8_t)country.MapMaterial[y].Coeficient;
        }
    }

    TextureDef2D tex;
    tex.width = 256;
    tex.height = 256;
    tex.format = GL_RGB_INTEGER;
    tex.internalFormat = GL_RGB8UI;
    tex.type = GL_UNSIGNED_BYTE;
    tex.data = data;

    //FILE* file = fopen("country.raw", "wb");
    //fwrite(data, 1, 256 * 256 * 3, file);
    //fclose(file);

    Texture2D* texture = new Texture2D;
    texture->Create(tex);
    texture->Bind();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texture;
}

#define printOpenGLError() printOglError(__FILE__, __LINE__)

int printOglError(char *file, int line)
{

	GLenum glErr;
	int    retCode = 0;

	glErr = glGetError();
	if (glErr != GL_NO_ERROR)
	{
		printf("glError in file %s @ line %d: %s\n",
			file, line, gluErrorString(glErr));
		retCode = 1;
	}
	return retCode;
}

char* gG3DStringList[626 + 1] =
{
	"MSH_DUMMY",
	"MSH_A_BAT_1",
	"MSH_A_BAT_2",
	"MSH_A_CROW_1",
	"MSH_A_CROW_2",
	"MSH_A_DOVE_1",
	"MSH_A_DOVE_2",
	"MSH_A_PIGEON_1",
	"MSH_A_PIGEON_2",
	"MSH_A_SEAGULL_1",
	"MSH_A_SEAGULL_2",
	"MSH_A_SPELL_DOVE",
	"MSH_A_SWALLOW_1",
	"MSH_A_SWALLOW_2",
	"MSH_A_VULTURE_1",
	"MSH_A_VULTURE_2",
	"MSH_A_COW_1",
	"MSH_A_COW_2",
	"MSH_A_COW_3",
	"MSH_A_HORSE_1",
	"MSH_A_HORSE_2",
	"MSH_A_HORSE_3",
	"MSH_A_LEOPARD_1",
	"MSH_A_LEOPARD_2",
	"MSH_A_LEOPARD_3",
	"MSH_A_LION_1",
	"MSH_A_LION_2",
	"MSH_A_LION_3",
	"MSH_A_PIG_1",
	"MSH_A_PIG_2",
	"MSH_A_PIG_3",
	"MSH_SHARK_BONED",
	"MSH_A_SHEEP_1",
	"MSH_A_SHEEP_2",
	"MSH_A_SHEEP_3",
	"MSH_A_TIGER_1",
	"MSH_A_TIGER_2",
	"MSH_A_TIGER_3",
	"MSH_A_TORTOISE_1",
	"MSH_A_TORTOISE_2",
	"MSH_A_TORTOISE_3",
	"MSH_A_WOLF_1",
	"MSH_A_WOLF_2",
	"MSH_A_WOLF_3",
	"MSH_A_ZEBRA_1",
	"MSH_A_ZEBRA_2",
	"MSH_A_ZEBRA_3",
	"MSH_B_AMCN_1",
	"MSH_B_AMCN_1_A",
	"MSH_B_AMCN_2",
	"MSH_B_AMCN_2_A",
	"MSH_B_AMCN_3",
	"MSH_B_AMCN_3_A",
	"MSH_B_AMCN_4",
	"MSH_B_AMCN_5",
	"MSH_B_AMCN_CRECHE",
	"MSH_B_AMCN_FENCE",
	"MSH_B_AMCN_GRAVEYARD",
	"MSH_B_AMCN_TOTEM",
	"MSH_B_AMCN_WONDER",
	"MSH_B_AMCN_WORKSHOP",
	"MSH_B_AZTC_1",
	"MSH_B_AZTC_2",
	"MSH_B_AZTC_3",
	"MSH_B_AZTC_4",
	"MSH_B_AZTC_5",
	"MSH_B_AZTC_CRECHE",
	"MSH_B_AZTC_GRAVEYARD",
	"MSH_B_AZTC_OLMECHEAD",
	"MSH_B_AZTC_STATUE",
	"MSH_B_AZTC_TEMPLE",
	"MSH_B_AZTC_TOTEM",
	"MSH_B_AZTC_WONDER",
	"MSH_B_AZTC_WORKSHOP",
	"MSH_B_CELT_1",
	"MSH_B_CELT_2",
	"MSH_B_CELT_3",
	"MSH_B_CELT_4",
	"MSH_B_CELT_5",
	"MSH_B_CELT_5A",
	"MSH_B_CELT_CRECHE",
	"MSH_B_CELT_FENCE_SHORT",
	"MSH_B_CELT_FENCE_TALL",
	"MSH_B_CELT_GRAVEYARD",
	"MSH_B_CELT_GRAVE_01",
	"MSH_B_CELT_GRAVE_02",
	"MSH_B_CELT_GRAVE_03",
	"MSH_B_CELT_TOTEM",
	"MSH_B_CELT_WALLGATE_SPELL",
	"MSH_B_CELT_WALLTOWER_SPELL",
	"MSH_B_CELT_WALL_SPELL",
	"MSH_B_CELT_WONDER",
	"MSH_B_CELT_WORKSHOP",
	"MSH_B_CITADEL_AMCN_ALTAR",
	"MSH_B_CITADEL_AZTC_ALTAR",
	"MSH_B_CITADEL_CELT_ALTAR",
	"MSH_B_CITADEL_DESIRE_HUNGRY",
	"MSH_B_CITADEL_DESIRE_SLEEP",
	"MSH_B_CITADEL_EGPT_ALTAR",
	"MSH_B_CITADEL_GREK_ALTAR",
	"MSH_B_CITADEL_JAPN_ALTAR",
	"MSH_B_CITADEL_NORS_ALTAR",
	"MSH_B_CITADEL_POP_COUNTER_BEAD",
	"MSH_B_CITADEL_TIBT_ALTAR",
	"MSH_B_EGPT_1",
	"MSH_B_EGPT_1_A",
	"MSH_B_EGPT_2",
	"MSH_B_EGPT_2_A",
	"MSH_B_EGPT_3",
	"MSH_B_EGPT_4",
	"MSH_B_EGPT_5",
	"MSH_B_EGPT_5_A",
	"MSH_B_EGPT_BARREL",
	"MSH_B_EGPT_CART",
	"MSH_B_EGPT_CRECHE",
	"MSH_B_EGPT_GRAVEYARD",
	"MSH_B_EGPT_NEEDLE",
	"MSH_B_EGPT_POT_A",
	"MSH_B_EGPT_POT_B",
	"MSH_B_EGPT_TOTEM",
	"MSH_B_EGPT_WELL",
	"MSH_B_EGPT_WINDMILL",
	"MSH_B_EGPT_WONDER",
	"MSH_B_EGPT_WORKSHOP",
	"MSH_B_GREK_1",
	"MSH_B_GREK_2",
	"MSH_B_GREK_2_A",
	"MSH_B_GREK_3",
	"MSH_B_GREK_4",
	"MSH_B_GREK_5",
	"MSH_B_GREK_5A",
	"MSH_B_GREK_CRECHE",
	"MSH_B_GREK_GRAVEYARD",
	"MSH_B_GREK_SAIL",
	"MSH_B_GREK_STATUE",
	"MSH_B_GREK_TOTEM",
	"MSH_B_GREK_WONDER",
	"MSH_B_GREK_WORKSHOP",

	"MSH_B_JAPN_1",
	"MSH_B_JAPN_2",
	"MSH_B_JAPN_3",
	"MSH_B_JAPN_4",
	"MSH_B_JAPN_5",
	"MSH_B_JAPN_CRECHE",
	"MSH_B_JAPN_GRAVEYARD",
	"MSH_B_JAPN_TOTEM",
	"MSH_B_JAPN_WONDER",
	"MSH_B_JAPN_WORKSHOP",

	"MSH_B_CAMPFIRE",
	"MSH_B_CREATURE_PEN_LEASH_POLE",

	"MSH_B_FOOTBALL_CENTRECIRCLE",
	"MSH_B_FOOTBALL_CORNERFLAG",
	"MSH_B_FOOTBALL_GOAL",
	"MSH_B_FOOTBALL_PITCH",
	"MSH_B_FOOTBALL_PITCH_BIG",

	"MSH_B_MINE_ENTRANCE",
	"MSH_B_PIER",
	"MSH_B_SCAFFOLD_01",
	"MSH_B_SCAFFOLD_02",
	"MSH_B_SCAFFOLD_03",
	"MSH_B_XANDO_BASE",
	"MSH_B_XANDO_O",
	"MSH_B_XANDO_X",
	"MSH_PIPER_ENTRANCE_PHYS_1",

	"MSH_B_AMCN_VILLAGECENTRE",
	"MSH_B_AZTC_VILLAGECENTRE",
	"MSH_B_CELT_VILLAGECENTRE",

	"MSH_B_DESIRE_FLAG_ABODE",
	"MSH_B_DESIRE_FLAG_CHILDREN",
	"MSH_B_DESIRE_FLAG_CIVIC_BUILDING",
	"MSH_B_DESIRE_FLAG_FOOD",
	"MSH_B_DESIRE_FLAG_PLAYTIME",
	"MSH_B_DESIRE_FLAG_PROTECTION",
	"MSH_B_DESIRE_FLAG_RAIN",
	"MSH_B_DESIRE_FLAG_SUN",
	"MSH_B_DESIRE_FLAG_WOOD",

	"MSH_B_EGPT_VILLAGECENTRE",
	"MSH_B_GREK_VILLAGECENTRE",
	"MSH_B_JAPN_VILLAGECENTRE",
	"MSH_B_NORS_VILLAGECENTRE",

	"MSH_B_PLAYER_ICON01",
	"MSH_B_PLAYER_ICON01_PLINTH",
	"MSH_B_PLAYER_ICON_APE",
	"MSH_B_PLAYER_ICON_BEAR",
	"MSH_B_PLAYER_ICON_COW",
	"MSH_B_PLAYER_ICON_HORSE",
	"MSH_B_PLAYER_ICON_LEOPARD",
	"MSH_B_PLAYER_ICON_LION",
	"MSH_B_PLAYER_ICON_PLINTH_AMCN",
	"MSH_B_PLAYER_ICON_PLINTH_AZTC",
	"MSH_B_PLAYER_ICON_PLINTH_CELT",
	"MSH_B_PLAYER_ICON_PLINTH_EGPT",
	"MSH_B_PLAYER_ICON_PLINTH_GREK",
	"MSH_B_PLAYER_ICON_PLINTH_JAPN",
	"MSH_B_PLAYER_ICON_PLINTH_NORS",
	"MSH_B_PLAYER_ICON_PLINTH_TIBT",
	"MSH_B_PLAYER_ICON_POLARBEAR",
	"MSH_B_PLAYER_ICON_SHEEP",
	"MSH_B_PLAYER_ICON_TIGER",
	"MSH_B_PLAYER_ICON_TORTOISE",
	"MSH_B_PLAYER_ICON_WOLF",
	"MSH_B_PLAYER_ICON_ZEBRA",

	"MSH_B_TIBT_VILLAGECENTRE",
	"MSH_B_VILLAGECENTRE_SPELLHAND",
	"MSH_B_NORS_1",
	"MSH_B_NORS_2",
	"MSH_B_NORS_2_A",
	"MSH_B_NORS_3",
	"MSH_B_NORS_3_A",
	"MSH_B_NORS_4",
	"MSH_B_NORS_5",
	"MSH_B_NORS_CRECHE",
	"MSH_B_NORS_GATE",
	"MSH_B_NORS_GRAVEYARD",
	"MSH_B_NORS_PIGPEN",
	"MSH_B_NORS_TEMPLE",
	"MSH_B_NORS_TOTEM",
	"MSH_B_NORS_WONDER",
	"MSH_B_NORS_WORKSHOP",
	"MSH_NORS_GATE_PHYS_1",
	"MSH_NORS_GATE_PHYS_2",
	"MSH_B_AMCN_STORAGE_PIT",
	"MSH_B_AZTC_STORAGE_PIT",
	"MSH_B_CELT_STORAGE_PIT",
	"MSH_B_EGPT_STORAGE_PIT",
	"MSH_B_GRAIN",
	"MSH_B_GREK_STORAGE_PIT",
	"MSH_B_JAPN_STORAGE_PIT",
	"MSH_B_NORS_STORAGE_PIT",
	"MSH_B_TIBT_STORAGE_PIT",
	"MSH_B_WOOD_00",
	"MSH_B_WOOD_01",
	"MSH_B_WOOD_02",
	"MSH_B_WOOD_03",
	"MSH_B_WOOD_04",
	"MSH_B_WOOD_OUTSIDE",
	"MSH_B_TIBT_1",
	"MSH_B_TIBT_2",
	"MSH_B_TIBT_3",
	"MSH_B_TIBT_3_A",
	"MSH_B_TIBT_4",
	"MSH_B_TIBT_5",
	"MSH_B_TIBT_5_A",
	"MSH_B_TIBT_CRECHE",
	"MSH_B_TIBT_GRAVEYARD",
	"MSH_B_TIBT_LARGE_PILLER",
	"MSH_B_TIBT_MINI_TEMPLE",
	"MSH_B_TIBT_TOTEM",
	"MSH_B_TIBT_WONDER",
	"MSH_B_TIBT_WORKSHOP",
	"MSH_B_SPELLHAND",
	"MSH_B_WORSHIPGRAIN",
	"MSH_B_WORSHIPSITE_01_NEUTRAL",
	"MSH_BOULDER1_CHALK",
	"MSH_BOULDER1_LIME",
	"MSH_BOULDER1_SAND",
	"MSH_BOULDER1_VOLCANIC",
	"MSH_BOULDER2_CHALK",
	"MSH_BOULDER2_LIME",
	"MSH_BOULDER2_SAND",
	"MSH_BOULDER2_VOLCANIC",
	"MSH_BOULDER3_CHALK",
	"MSH_BOULDER3_LIME",
	"MSH_BOULDER3_SAND",
	"MSH_BOULDER3_VOLCANIC",
	"MSH_FAT_PILAR_CHALK",
	"MSH_FAT_PILAR_LIME",
	"MSH_FAT_PILAR_SAND",
	"MSH_FAT_PILAR_VOLCANIC",
	"MSH_F_CREATURE_CAVE",
	"MSH_F_FOREST",
	"MSH_F_FOREST01",
	"MSH_F_FOREST02",
	"MSH_F_FOREST03",
	"MSH_F_FOREST_BOOK01",
	"MSH_F_GIANTSKELETON",
	"MSH_F_GIANTSKULL",
	"MSH_F_IDOL",
	"MSH_F_PIPER_CAVE",
	"MSH_F_RIB1",
	"MSH_F_RIB2",
	"MSH_F_SPOUTHOLE",
	"MSH_F_VERTABRAE",
	"MSH_PILAR2_CHALK",
	"MSH_PILAR2_LIME",
	"MSH_PILAR2_SAND",
	"MSH_PILAR2_VOLCANIC",
	"MSH_PILAR3_CHALK",
	"MSH_PILAR3_LIME",
	"MSH_PILAR3_SAND",
	"MSH_PILAR3_VOLCANIC",
	"MSH_PILAR_CHALK",
	"MSH_PILAR_LIME",
	"MSH_PILAR_SAND",
	"MSH_PILAR_VOLCANIC",
	"MSH_PUZZLE_1",
	"MSH_PUZZLE_2",
	"MSH_PUZZLE_3",
	"MSH_PUZZLE_4",
	"MSH_PUZZLE_5",
	"MSH_SPIKEY_PILAR_CHALK",
	"MSH_SPIKEY_PILAR_LIME",
	"MSH_SPIKEY_PILAR_SAND",
	"MSH_SPIKEY_PILAR_VOLCANIC",
	"MSH_Z_PHONEBOX",
	"MSH_I_BLAST",
	"MSH_I_FIREBALL",
	"MSH_I_FOOD",
	"MSH_I_FOOTBALL",
	"MSH_I_FOREST",
	"MSH_I_HEAL",
	"MSH_I_LIGHTNING",
	"MSH_I_LIGHTNING2",
	"MSH_I_MINISCROLL",
	"MSH_I_MINISCROLL_ACTIVE",
	"MSH_I_MINISCROLL_BRONZE",
	"MSH_I_MINISCROLL_BRONZE_ACTIVE",
	"MSH_I_MINISCROLL_GOLD",
	"MSH_I_MINISCROLL_GOLD_ACTIVE",
	"MSH_I_MINISCROLL_SILVER",
	"MSH_I_MINISCROLL_SILVER_ACTIVE",
	"MSH_I_OFFERING_BEDROLL",
	"MSH_I_OFFERING_FERTILITY",
	"MSH_I_OFFERING_FOOD",
	"MSH_I_OFFERING_PLAY",
	"MSH_I_OFFERING_PROTECTION",
	"MSH_I_OFFERING_SKULL",
	"MSH_I_OFFERING_VILLAGE_IMPROVE",
	"MSH_I_OFFERING_WATER",
	"MSH_I_OFFERING_WOOD",
	"MSH_I_PRIEST",
	"MSH_I_SHIELD",
	"MSH_I_SKELETON",
	"MSH_I_SPELLGEM",
	"MSH_I_TREES",
	"MSH_I_WALL",
	"MSH_GATE_TOTEM_PLINTHE_PHYS_1",
	"MSH_GATE_TOTEM_PLINTHE_PHYS_2",
	"MSH_GATE_TOTEM_PLINTHE_PHYS_3",
	"MSH_O_ARK",
	"MSH_O_ARK_DRY_DOCK",
	"MSH_O_ARK_WRECK",
	"MSH_O_AXE",
	"MSH_O_BAG",
	"MSH_O_BALL",
	"MSH_O_BELLTOWER",
	"MSH_O_BOX_FRAME",
	"MSH_O_BRANCH_EVERGREEN_INHAND",
	"MSH_O_BRANCH_FRUIT_INHAND",
	"MSH_O_BRANCH_HARDWOOD_INHAND",
	"MSH_O_BURNT_TREE",
	"MSH_O_CAULDRON",
	"MSH_O_CHAMPI",
	"MSH_O_CREATURE_TURD",
	"MSH_O_CROOK",
	"MSH_O_FISHINGROD",
	"MSH_O_FLATROCK_CHALK",
	"MSH_O_FLATROCK_LIMESTONE",
	"MSH_O_FLATROCK_SANDSTONE",
	"MSH_O_FLATROCK_VOLCANIC",
	"MSH_O_FLOWER_BED_1",
	"MSH_O_FLOWER_BED_2",
	"MSH_O_GATE_TOTEM_APE",
	"MSH_O_GATE_TOTEM_BLANK",
	"MSH_O_GATE_TOTEM_COW",
	"MSH_O_GATE_TOTEM_PLINTHE",
	"MSH_O_GATE_TOTEM_TIGER",
	"MSH_O_HAMMER",
	"MSH_O_INFO_SIGN",
	"MSH_O_JAPN_STONETHING",
	"MSH_O_LARGE_FISH_DEAD",
	"MSH_O_LIGHT_BULB",
	"MSH_O_LOGS_INHAND",
	"MSH_O_LONGROCK_CHALK",
	"MSH_O_LONGROCK_LIMESTONE",
	"MSH_O_LONGROCK_SANDSTONE",
	"MSH_O_LONGROCK_VOLCANIC",
	"MSH_O_MAGIC_MUSHROOM",
	"MSH_O_MALLET_HEAVY",
	"MSH_O_ROCK_CHALK",
	"MSH_O_ROCK_LIMESTONE",
	"MSH_O_ROCK_SANDSTONE",
	"MSH_O_ROCK_VOLCANIC",
	"MSH_O_SAW",
	"MSH_O_SCYTHE",
	"MSH_O_SHARPROCK_CHALK",
	"MSH_O_SHARPROCK_LIMESTONE",
	"MSH_O_SHARPROCK_SANDSTONE",
	"MSH_O_SHARPROCK_VOLCANIC",
	"MSH_O_SKULL_TOTEM",
	"MSH_O_SPADE",
	"MSH_O_SQUAREROCK_CHALK",
	"MSH_O_SQUAREROCK_LIMESTONE",
	"MSH_O_SQUAREROCK_SANDSTONE",
	"MSH_O_SQUAREROCK_VOLCANIC",
	"MSH_O_TICKERTAPE",
	"MSH_O_TOADSTOOL",
	"MSH_O_TOMBSTONE",
	"MSH_O_TOWNLIGHT",
	"MSH_O_TOY_BALL",
	"MSH_O_TOY_BOWLING_BALL",
	"MSH_O_TOY_CUDDLY",
	"MSH_O_TOY_DIE",
	"MSH_O_TOY_SKITTLE",
	"MSH_O_WHEAT_ENCLOSURE",
	"MSH_O_WHEAT_INHAND",
	"MSH_O_WOOD_INHAND",
	"MSH_REWARD_CHEST_EXPLODE",
	"MSH_SCULPTURE_IN_PROGRESS",
	"MSH_SINGING_STONE_CENTER",
	"MSH_SINGING_STONE",
	"MSH_IDOL_BUILDER",
	"MSH_NEW_SHAOLIN",
	"MSH_P_AMCN_F_A_1",
	"MSH_P_AMCN_F_A_2",
	"MSH_P_AMCN_F_A_3",
	"MSH_P_AMCN_M_A_1",
	"MSH_P_AMCN_M_A_2",
	"MSH_P_AMCN_M_A_3",
	"MSH_P_ANIMAL_BREEDER",
	"MSH_P_ANIMAL_TRAINER",
	"MSH_P_AZTC_F_A_1",
	"MSH_P_AZTC_F_A_2",
	"MSH_P_AZTC_F_A_3",
	"MSH_P_AZTC_LEADER",
	"MSH_P_AZTC_M_A_1",
	"MSH_P_AZTC_M_A_2",
	"MSH_P_AZTC_M_A_3",
	"MSH_P_BANDIT",
	"MSH_P_BOY_BLACK_1",
	"MSH_P_BOY_BLACK_2",
	"MSH_P_BOY_BLACK_3",
	"MSH_P_BOY_ORIENTAL_1",
	"MSH_P_BOY_ORIENTAL_2",
	"MSH_P_BOY_ORIENTAL_3",
	"MSH_P_BOY_TAN_1",
	"MSH_P_BOY_TAN_2",
	"MSH_P_BOY_TAN_3",
	"MSH_P_BOY_WHITE",
	"MSH_P_BOY_WHITE_1",
	"MSH_P_BOY_WHITE_2",
	"MSH_P_BOY_WHITE_3",
	"MSH_P_CELT_FOOTBALLER",
	"MSH_P_CELT_F_A_1",
	"MSH_P_CELT_F_A_2",
	"MSH_P_CELT_F_A_3",
	"MSH_P_CELT_M_A_1",
	"MSH_P_CELT_M_A_2",
	"MSH_P_CELT_M_A_3",
	"MSH_P_CRUSADER",
	"MSH_P_CULT_PRIEST",
	"MSH_P_CULT_PRIESTESS",
	"MSH_P_EGPT_F_A_1",
	"MSH_P_EGPT_F_A_2",
	"MSH_P_EGPT_F_A_3",
	"MSH_P_EGPT_M_A_1",
	"MSH_P_EGPT_M_A_2",
	"MSH_P_EGPT_M_A_3",
	"MSH_P_EGPT_M_B_1",
	"MSH_P_EGPT_M_B_2",
	"MSH_P_ENGINEER",
	"MSH_P_GIRL_BLACK_1",
	"MSH_P_GIRL_BLACK_2",
	"MSH_P_GIRL_BLACK_3",
	"MSH_P_GIRL_ORIENTAL_1",
	"MSH_P_GIRL_ORIENTAL_2",
	"MSH_P_GIRL_ORIENTAL_3",
	"MSH_P_GIRL_TAN_1",
	"MSH_P_GIRL_TAN_2",
	"MSH_P_GIRL_TAN_3",
	"MSH_P_GIRL_WHITE",
	"MSH_P_GIRL_WHITE_1",
	"MSH_P_GIRL_WHITE_2",
	"MSH_P_GIRL_WHITE_3",
	"MSH_P_GREK_F_A_1",
	"MSH_P_GREK_F_A_2",
	"MSH_P_GREK_F_A_3",
	"MSH_P_GREK_M_A_1",
	"MSH_P_GREK_M_A_2",
	"MSH_P_GREK_M_A_3",
	"MSH_P_HERMIT",
	"MSH_P_HIPPY",
	"MSH_P_IDOL_BUILDER",
	"MSH_P_INTRO_F",
	"MSH_P_INTRO_M",
	"MSH_P_JAPANESE_F_A_2",
	"MSH_P_JAPANESE_F_A_3",
	"MSH_P_JAPANESE_M_A_1",
	"MSH_P_JAPANESE_M_A_2",
	"MSH_P_JAPANESE_M_A_3",
	"MSH_P_JAPN_F_A_1",
	"MSH_P_JAPN_F_A_2",
	"MSH_P_JAPN_F_A_3",
	"MSH_P_JAPN_M_A_1",
	"MSH_P_JAPN_M_A_2",
	"MSH_P_JAPN_M_A_3",
	"MSH_P_NOMAD",
	"MSH_P_NORS_FOOTBALLER",
	"MSH_P_NORS_F_A_1",
	"MSH_P_NORS_F_A_2",
	"MSH_P_NORS_F_A_3",
	"MSH_P_NORS_M_A_1",
	"MSH_P_NORS_M_A_2",
	"MSH_P_NORS_M_A_3",
	"MSH_P_NORS_SAILOR",
	"MSH_P_PIED_PIPER",
	"MSH_P_SAILOR_ACCORDIAN",
	"MSH_P_SCULPTOR",
	"MSH_P_SHAOLIN_MONK",
	"MSH_P_SHEPERD",
	"MSH_P_SKELETON_F",
	"MSH_P_SKELETON_M",
	"MSH_P_TIBETAN_F_A_2",
	"MSH_P_TIBETAN_F_A_3",
	"MSH_P_TIBETAN_M_A_1",
	"MSH_P_TIBETAN_M_A_2",
	"MSH_P_TIBETAN_M_A_3",
	"MSH_P_TIBT_F_A_1",
	"MSH_P_TIBT_F_A_2",
	"MSH_P_TIBT_F_A_3",
	"MSH_P_TIBT_M_A_1",
	"MSH_P_TIBT_M_A_2",
	"MSH_P_TIBT_M_A_3",
	"MSH_P_TIBT_M_B_1",
	"MSH_P_TIBT_M_B_2",
	"MSH_CREED",
	"MSH_S_BLAST_CONE",
	"MSH_S_BLAST_CENTRE",
	"MSH_S_BLAST_INNER",
	"MSH_S_BLAST_OUTER",
	"MSH_S_COLUMN",
	"MSH_S_FIREBALL_TAIL",
	"MSH_S_HEAL_MESH",
	"MSH_S_GRAIN_PILE",
	"MSH_S_HORN_OF_PLENTY",
	"MSH_S_PHILE",
	"MSH_S_PHILE_ANGRY",
	"MSH_S_PHILE_BIG",
	"MSH_S_PHILE_FAT",
	"MSH_S_PHILE_FREEZE",
	"MSH_S_PHILE_FRIGHTEN",
	"MSH_S_PHILE_HUNGRY",
	"MSH_S_PHILE_ILL",
	"MSH_S_PHILE_INVISIBLE",
	"MSH_S_PHILE_ITCHY",
	"MSH_S_PHILE_LOVE",
	"MSH_S_PHILE_SMALL",
	"MSH_S_PHILE_STRONG",
	"MSH_S_PHILE_THIN",
	"MSH_S_PHILE_THIRSTY",
	"MSH_S_PHILE_TIRED",
	"MSH_S_PHILE_WEAK",
	"MSH_S_RAIN_CONE",
	"MSH_S_SHIELDMAGICLOWPOLY",
	"MSH_S_SOLID_SHIELD",
	"MSH_S_SPELLBALLSURFACE02",
	"MSH_S_SKULL_ICON",
	"MSH_S_SPELL_CREATOR",
	"MSH_S_SPELL_DISPENSER",
	"MSH_S_VORTEXCYLINDER",
	"MSH_S_ZCHEATBOX",
	"MSH_S_PULSE_IN",
	"MSH_S_PULSE_OUT",
	"MSH_A_WORM",
	"MSH_B_DUMMY_CITADEL",
	"MSH_Z_APPLE_BARREL",
	"MSH_Z_ARROW",
	"MSH_Z_SPELLROCK01",
	"MSH_Z_SPELLROCK02",
	"MSH_T_COPSE",
	"MSH_T_COPSE_A",
	"MSH_T_BEECH",
	"MSH_T_BIRCH",
	"MSH_T_BUSH",
	"MSH_T_BUSH_A",
	"MSH_T_BUSH_B",
	"MSH_T_CEDAR",
	"MSH_T_CONIFER",
	"MSH_T_CONIFER_A",
	"MSH_T_CYPRESS",
	"MSH_T_CYPRESS_A",
	"MSH_T_HEDGE",
	"MSH_T_HEDGE_A",
	"MSH_T_OAK",
	"MSH_T_OAK_A",
	"MSH_T_OLIVE",
	"MSH_T_PALM",
	"MSH_T_PALM_A",
	"MSH_T_PALM_B",
	"MSH_T_PALM_C",
	"MSH_T_PINE",
	"MSH_T_RICE",
	"MSH_T_ROOTS",
	"MSH_T_ROOTS_PILE",
	"MSH_T_WHEAT",
	"MSH_T_WHEAT_INFIELD",
	"MSH_U_AXE",
	"MSH_U_BAG",
	"MSH_U_BALL",
	"MSH_U_BENCH",
	"MSH_U_BUCKET",
	"MSH_U_BUCKET02",
	"MSH_U_CART",
	"MSH_U_CHEST",
	"MSH_U_CHEST_TOP",
	"MSH_U_CROOK",
	"MSH_U_FISHING_ROD",
	"MSH_U_FRAME",
	"MSH_U_FRAME_SKIN",
	"MSH_U_HAMMER",
	"MSH_U_MALLET_HEAVY",
	"MSH_U_SAW",
	"MSH_U_SCYTHE",
	"MSH_U_SPADE",
	"MSH_U_STOOL",
	"MSH_U_STOOL01",
	"MSH_U_TABLE",
	"MSH_U_TROUGH",
	"MSH_U_WASHING_LINE_AMCN",
	"MSH_U_WASHING_LINE_AZTC",
	"MSH_U_WASHING_LINE_CELT",
	"MSH_U_WASHING_LINE_EGPT",
	"MSH_U_WASHING_LINE_GREK",
	"MSH_U_WASHING_LINE_JAPN",
	"MSH_U_WASHING_LINE_NORS",
	"MSH_U_WASHING_LINE_TIBT",
	"MAX_COUNT_3D_MESHES"
};

void Game::Run()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	printf("display size: %.2f %.2f\n", io.DisplaySize.x, io.DisplaySize.y);

	ImGui_ImplSDL2_InitForOpenGL(mWindow, mGLContext);
	ImGui_ImplOpenGL3_Init("#version 130");

	ImGui::StyleColorsDark();

    mCamera = new Camera;
    mCamera->SetProjectionMatrixPerspective(60.0f, (float)1280 / (float)720, 0.1f, 8192.f);
	//mCamera->SetPosition(glm::vec3(2500.0f, 240.0f, 1600.0f));
	//mCamera->SetPosition(glm::vec3(1441.56f, 240.0f, 2081.76));
	mCamera->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));

    mCamera->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));

	std::string allMeshesFilePath = GetGamePath() + "\\Data\\AllMeshes.g3d";

	OSFile* allMeshesFile = new OSFile();
	allMeshesFile->Open(allMeshesFilePath.c_str(), LH_FILE_MODE::Read);

	MeshPack* pack = new MeshPack(allMeshesFile);

	allMeshesFile->Close();

    std::string filePath = GetGamePath() + "\\Data\\Landscape\\Land1.lnd";

	// replace static LH3DIsland with LandIsland*.
    LandIsland* island = new LandIsland();
    island->LoadFromDisk(filePath);

    LH3DIsland::LoadFromDisk(filePath.c_str());
    std::vector<LH3DVertex> vertexArray = LH3DIsland::GetVerticies();
    mIndicesArray = LH3DIsland::GetIndices();

    printf("Blocks: %d\n", LH3DIsland::g_block_count);

    Texture2D* countryTexture = CreateCountryTexture();

    // Create Vertex Array Object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

	std::string modelPath = GetGamePath() + "\\Data\\cone.l3d";

	size_t fileSize;
	char* modelData = OSFile::ReadAll(modelPath.c_str(), &fileSize);

	Mesh* mesh = new Mesh();
	mesh->LoadFromL3D(modelData, fileSize);

	Shader* modelShader = new Shader();
	modelShader->Create(OpenBlack::Shaders::WorldObject::VertexShader, OpenBlack::Shaders::WorldObject::FragmentShader);

	GLuint shaderProgram = modelShader->GetHandle();

	glUseProgram(shaderProgram);

	GLint uniView = glGetUniformLocation(shaderProgram, "viewProj");
	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(mCamera->GetViewProjectionMatrix()));

	SDL_Event event;
	bool running = true;

	glEnable(GL_DEPTH_TEST);
	glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1);

	int currentMesh = 0;

	bool show_demo_window = true;

	while (running)
	{
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				running = false;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(mWindow))
				running = false;

			switch (event.type) {
			case SDL_KEYUP:
				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE: running = false; break;
				case SDLK_LEFT: currentMesh--; break;
				case SDLK_RIGHT: currentMesh++; break;
				}
				break;
			}
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(mWindow);
		ImGui::NewFrame();

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				ImGui::MenuItem("(dummy menu)", NULL, false, false);
				if (ImGui::MenuItem("New")) {}
				if (ImGui::MenuItem("Open", "Ctrl+O")) {}
				ImGui::Separator();
				if (ImGui::MenuItem("Quit", "Alt+F4")) {}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}

		{
			static float f = 0.0f;
			static glm::vec3 v = glm::vec3(0.0f, 5.0f, 5.0f);

			ImGui::Begin("Mesh Viewer");                          // Create a window called "Hello, world!" and append into it.

			ImGui::SliderFloat3("cam pos", glm::value_ptr(v), -50.0f, 50.0f);

			mCamera->SetPosition(v);

			ImGui::Separator();
			ImGui::PushItemWidth(-1);
			ImGui::ListBox("", &currentMesh, gG3DStringList, IM_ARRAYSIZE(gG3DStringList), 8);
			ImGui::Separator();

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::End();
		}

		SDL_GL_MakeCurrent(mWindow, mGLContext);
		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(mCamera->GetViewProjectionMatrix()));

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		pack->Meshes[currentMesh]->Draw();
		printOpenGLError();

		glUseProgram(0);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(mWindow);
	}

	/*VertexBuffer* vertexBuffer = new VertexBuffer();
	vertexBuffer->Create(vertexArray.data(), vertexArray.size() * sizeof(LH3DVertex));

	IndexBuffer* indexBuffer = new IndexBuffer();
	indexBuffer->Create(mIndicesArray.data(), mIndicesArray.size());

    char* terrainVertSource = OSFile::ReadAll("shaders/terrain.vert.glsl", nullptr);
    char* terrainFragSource = OSFile::ReadAll("shaders/terrain.frag.glsl", nullptr);

    Shader* terrainShader = new Shader();

    terrainShader->Create(terrainVertSource, terrainFragSource);

    GLuint shaderProgram = terrainShader->GetHandle();

    glUseProgram(shaderProgram);

    // Specify the layout of the vertex data
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
    GLint countryAttrib = glGetAttribLocation(shaderProgram, "country");
    GLint altAttrib = glGetAttribLocation(shaderProgram, "altitude");
    GLint uniView = glGetUniformLocation(shaderProgram, "viewProj");
    GLint sMatAttrib = glGetUniformLocation(shaderProgram, "sMaterials");
    GLint sCountryLookupAttrib = glGetUniformLocation(shaderProgram, "sCountryLookup");

    glEnableVertexAttribArray(posAttrib);
    glEnableVertexAttribArray(colAttrib);
    glEnableVertexAttribArray(texAttrib);
    glEnableVertexAttribArray(countryAttrib);
    glEnableVertexAttribArray(altAttrib);

    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(LH3DVertex), (GLvoid*)offsetof(LH3DVertex, position));
    glVertexAttribPointer(colAttrib, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(LH3DVertex), (GLvoid*)offsetof(LH3DVertex, color));
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(LH3DVertex), (GLvoid*)offsetof(LH3DVertex, uv));
    glVertexAttribPointer(countryAttrib, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(LH3DVertex), (GLvoid*)offsetof(LH3DVertex, country));
    glVertexAttribPointer(altAttrib, 1, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(LH3DVertex), (GLvoid*)offsetof(LH3DVertex, altitude));

    glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(mCamera->GetViewProjectionMatrix()));

    glActiveTexture(GL_TEXTURE0);
    LH3DIsland::g_materialarray->Bind();
    glUniform1i(sMatAttrib, 0);

    glActiveTexture(GL_TEXTURE1);
    countryTexture->Bind();
    glUniform1i(sCountryLookupAttrib, 1);

    SDL_Event event;
    bool running = true;

    glEnable(GL_DEPTH_TEST);
    glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1);

    while (running)
    {
        while (SDL_PollEvent(&event)) {
            running = event.type != SDL_QUIT;

            switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                case SDLK_w: mCamera->SetPosition(mCamera->GetPosition() + mCamera->GetForward() * 2.0f); break;
                case SDLK_s: mCamera->SetPosition(mCamera->GetPosition() + mCamera->GetForward() * -2.0f); break;
                case SDLK_a: mCamera->SetPosition(mCamera->GetPosition() + mCamera->GetRight() * -2.0f); break;
                case SDLK_d: mCamera->SetPosition(mCamera->GetPosition() + mCamera->GetRight() * 2.0f); break;
                }
                break;
            }
        }

        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(mCamera->GetViewProjectionMatrix()));

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		vertexBuffer->Bind();
		indexBuffer->Bind();
        glDrawElements(GL_TRIANGLES, mIndicesArray.size(), GL_UNSIGNED_INT, 0);

        SDL_GL_SwapWindow(mWindow);
    }

	//delete buffer;
    //glDeleteBuffers(1, &vbo);*/
    glDeleteVertexArrays(1, &vao);

    //delete countryTexture;

    LH3DIsland::Release();
}

void Game::loop()
{

}

void Game::createWindow(int width, int height)
{
    int pos_x = SDL_WINDOWPOS_CENTERED,
        pos_y = SDL_WINDOWPOS_CENTERED;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

    mWindow = SDL_CreateWindow("OpenBlack", pos_x, pos_y, width, height, flags);
    if (!mWindow)
    {
        std::stringstream error;
        error << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        throw std::runtime_error(error.str());
    }

	mGLContext = SDL_GL_CreateContext(mWindow);
	if (!mGLContext)
		throw std::runtime_error("Failed to create a GL context\n");

	SDL_GL_SetSwapInterval(1);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		std::stringstream error;
		error << "glewInit failed: " << glewGetErrorString(err) << std::endl;
		throw std::runtime_error(error.str());
	}

	//glEnable(GL_DEBUG_OUTPUT);
	//glDebugMessageCallback(OpenGLMsgCallback, 0);

    setWindowIcon();
}

void Game::setWindowIcon()
{
    // auto surface = SDLUtil::imageToSurface(image, true);
    // SDL_SetWindowIcon(mWindow, surface.get());
}

void Game::createContext()
{

}

void Game::LoadMap(std::string name)
{
	// GGame::ClearMap()
	// LH3DLandscape::Release()
	// LH3DIsland::Release()
	// GSetup::LoadMapFeatures(name)
	// Town::AsssignTownFeature()
}

std::string Game::GetGamePath()
{
    return "C:\\Users\\Matt\\Development\\openblack\\GameData";
}
