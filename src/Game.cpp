#include "Game.h"

#include <iostream>
#include <sstream>

#include <SDL.h>
#define GLEW_STATIC
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <3D/Camera.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <LH3D/LandIsland.h>
#include <LH3D/oldLH3DIsland.h>
#include <LH3D/LHOSFile.h>

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
    uint32_t flags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;
    if (SDL_WasInit(flags) == 0)
    {
        SDL_SetMainReady();
        if (SDL_Init(flags) != 0)
            throw std::runtime_error("Could not initialize SDL! " + std::string(SDL_GetError()));
    }

    createWindow(1600, 1200);
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

void Game::Run()
{
    mCamera = new Camera;
    mCamera->SetProjectionMatrixPerspective(60.0f, (float)800 / (float)600, 0.1f, 8192.f);
    mCamera->SetPosition(glm::vec3(2500.0f, 240.0f, 1600.0f));
    mCamera->SetRotation(glm::vec3(35.0f, 140.0f, 0.0f));

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

    // Create a Vertex Buffer Object and copy the vertex data to it
    GLuint vbo;
    glGenBuffers(1, &vbo); // Generate 1 buffer
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexArray.size() * sizeof(LH3DVertex), vertexArray.data(), GL_STATIC_DRAW);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesArray.size() * sizeof(uint32_t), mIndicesArray.data(), GL_STATIC_DRAW);

    char* terrainVertSource = LHOSFile::ReadAll("shaders/terrain.vert.glsl", nullptr);
    char* terrainFragSource = LHOSFile::ReadAll("shaders/terrain.frag.glsl", nullptr);

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
        glDrawElements(GL_TRIANGLES, mIndicesArray.size(), GL_UNSIGNED_INT, 0);

        SDL_GL_SwapWindow(mWindow);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    delete countryTexture;

    LH3DIsland::Release();
}

void Game::loop()
{

}

void Game::createWindow(int width, int height)
{
    int pos_x = SDL_WINDOWPOS_CENTERED,
        pos_y = SDL_WINDOWPOS_CENTERED;

    uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

    mWindow = SDL_CreateWindow("OpenBlack", pos_x, pos_y, width, height, flags);
    if (!mWindow)
    {
        std::stringstream error;
        error << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        throw std::runtime_error(error.str());
    }

    setWindowIcon();
}

void Game::setWindowIcon()
{
    // auto surface = SDLUtil::imageToSurface(image, true);
    // SDL_SetWindowIcon(mWindow, surface.get());
}

void Game::createContext()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    mGLContext = SDL_GL_CreateContext(mWindow);
    if (!mGLContext)
        throw std::runtime_error("Failed to create a GL context\n");

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
    return "H:\\Program Files (x86)\\Lionhead Studios Ltd\\Black & White";
}
