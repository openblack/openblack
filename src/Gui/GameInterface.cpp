/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GameInterface.h"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <SDL.h>
#include <bgfx/bgfx.h>
#include <spdlog/spdlog.h>

#include "Audio/AudioManagerInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"

using namespace openblack::gui;
using openblack::Locator;
using openblack::filesystem::Path;

namespace
{
/// The scripts the game's text is in, in the order the game reads them
constexpr std::array k_TextScripts = {"InfoScript2.txt", "InfoScriptPatch2.txt", "InfoScriptMultiplayer2.txt"};
/// SetupThing's font, the first of GatheringText::SetupGameFonts
constexpr std::string_view k_Font = "j0";
constexpr uint16_t k_AtlasSize = 256;

std::vector<uint8_t> ReadIfExists(const std::filesystem::path& path)
{
	auto& fileSystem = Locator::filesystem::value();
	if (!fileSystem.Exists(path))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Missing {} for the interface", path.generic_string());
		return {};
	}
	return fileSystem.ReadAll(path);
}
} // namespace

namespace
{
/// A texture of the game made of a colour file and an alpha file, 256 pixels square. Null when they are missing.
std::unique_ptr<openblack::graphics::Texture2D> LoadTexture(const std::string& name)
{
	auto& fileSystem = Locator::filesystem::value();
	const auto colour = ReadIfExists(fileSystem.GetPath<Path::Textures>() / (name + ".raw"));
	const auto alpha = ReadIfExists(fileSystem.GetPath<Path::Textures>() / (name + "a.raw"));
	constexpr size_t k_Pixels = static_cast<size_t>(k_AtlasSize) * k_AtlasSize;
	if (colour.size() != k_Pixels * 3 || alpha.size() != k_Pixels)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Unable to read the texture {}", name);
		return nullptr;
	}
	const auto* data = bgfx::alloc(static_cast<uint32_t>(k_Pixels * 4));
	for (size_t i = 0; i < k_Pixels; ++i)
	{
		data->data[(i * 4) + 0] = colour[(i * 3) + 0];
		data->data[(i * 4) + 1] = colour[(i * 3) + 1];
		data->data[(i * 4) + 2] = colour[(i * 3) + 2];
		data->data[(i * 4) + 3] = alpha[i];
	}
	auto texture = std::make_unique<openblack::graphics::Texture2D>(name);
	texture->Create(k_AtlasSize, k_AtlasSize, 1, openblack::graphics::TextureFormat::RGBA8,
	                openblack::graphics::Wrapping::ClampEdge, openblack::graphics::Filter::Linear, data);
	return texture;
}
} // namespace

std::unique_ptr<GameInterface> GameInterface::Create(std::u16string_view playerName, MenuSettings settings)
{
	auto& fileSystem = Locator::filesystem::value();

	TextDatabase texts;
	for (const auto* script : k_TextScripts)
	{
		const auto data = ReadIfExists(fileSystem.GetPath<Path::Scripts>() / script);
		texts.AddScript(data);
	}

	const auto met = ReadIfExists(fileSystem.GetPath<Path::Data>() / (std::string(k_Font) + ".met"));
	const auto fnt = ReadIfExists(fileSystem.GetPath<Path::Data>() / (std::string(k_Font) + ".fnt"));
	auto font = GameFont::Load(met, fnt);
	if (!font)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Unable to read the font {}", k_Font);
		return nullptr;
	}

	// The front end atlas, and the pictures of the player's symbols (FrontEnd::Init) and of the mice (fn_00447450)
	auto atlas = LoadTexture("Front_end_buttons");
	if (!atlas)
	{
		return nullptr;
	}
	auto symbols = LoadTexture("ChooseSymbol");
	auto mice = LoadTexture("mousehelp");

	// White glyphs, their coverage in alpha
	const auto& coverage = font->GetAtlas();
	const auto* fontData = bgfx::alloc(static_cast<uint32_t>(coverage.size() * 4));
	for (size_t i = 0; i < coverage.size(); ++i)
	{
		fontData->data[(i * 4) + 0] = 0xFF;
		fontData->data[(i * 4) + 1] = 0xFF;
		fontData->data[(i * 4) + 2] = 0xFF;
		fontData->data[(i * 4) + 3] = coverage[i];
	}
	auto fontTexture = std::make_unique<graphics::Texture2D>(std::string("Font ") + std::string(k_Font));
	fontTexture->Create(font->GetAtlasSize().x, font->GetAtlasSize().y, 1, graphics::TextureFormat::RGBA8,
	                    graphics::Wrapping::ClampEdge, graphics::Filter::Linear, fontData);

	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Interface: {} texts, font {} with {} glyphs", texts.GetCount(), font->GetName(),
	                    font->GetGlyphs().size());
	return std::unique_ptr<GameInterface>(new GameInterface(std::move(texts), std::move(*font), std::move(atlas),
	                                                        std::move(fontTexture), std::move(symbols), std::move(mice),
	                                                        playerName, std::move(settings)));
}

GameInterface::GameInterface(TextDatabase texts, GameFont font, std::unique_ptr<graphics::Texture2D> atlas,
                             std::unique_ptr<graphics::Texture2D> fontTexture, std::unique_ptr<graphics::Texture2D> symbols,
                             std::unique_ptr<graphics::Texture2D> mice, std::u16string_view playerName, MenuSettings settings)
    : _texts(std::move(texts))
    , _font(std::move(font))
    , _atlas(std::move(atlas))
    , _fontTexture(std::move(fontTexture))
    , _symbols(std::move(symbols))
    , _mice(std::move(mice))
    , _painter(_canvas, _font, *_atlas, *_fontTexture)
    , _menu(std::make_unique<GameMenu>(_texts, _font, playerName, std::move(settings)))
{
	_painter.SetPictures(_symbols.get(), _mice.get());
}

GameInterface::~GameInterface() = default;

bool GameInterface::ProcessEvent(const SDL_Event& event, glm::u16vec2 resolution)
{
	_painter.Begin(resolution);
	if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
	{
		if (_menu->IsOpen())
		{
			_action = _menu->Escape();
			if (_action == GameMenu::Action::Continue)
			{
				_menu->Close();
				SDL_StopTextInput();
			}
		}
		else
		{
			_menu->Open();
			// For the names typed into the menu
			SDL_StartTextInput();
		}
		return true;
	}
	if (!_menu->IsOpen())
	{
		return false;
	}

	switch (event.type)
	{
	case SDL_MOUSEMOTION:
		_menu->MouseMove(_painter.ToDialog({event.motion.x, event.motion.y}));
		return true;
	case SDL_MOUSEBUTTONDOWN:
		if (event.button.button == SDL_BUTTON_LEFT)
		{
			_menu->MouseDown(_painter.ToDialog({event.button.x, event.button.y}));
		}
		return true;
	case SDL_MOUSEBUTTONUP:
		if (event.button.button == SDL_BUTTON_LEFT)
		{
			_action = _menu->MouseUp(_painter.ToDialog({event.button.x, event.button.y}));
			// SetupBox: every control clicks as it acts
			if (_menu->TakeClicked())
			{
				Locator::audio::value().PlaySoundEffect(static_cast<entt::id_type>(audio::SoundId::G_MenuButton), std::nullopt);
			}
			if (_action == GameMenu::Action::Continue)
			{
				_menu->Close();
				SDL_StopTextInput();
			}
		}
		return true;
	case SDL_MOUSEWHEEL:
	{
		glm::ivec2 mouse;
		SDL_GetMouseState(&mouse.x, &mouse.y);
		_menu->Wheel(_painter.ToDialog(mouse), event.wheel.y);
		return true;
	}
	case SDL_TEXTINPUT:
		_menu->TextInput(ToUtf16(event.text.text));
		return true;
	// Keys let go of still reach the game, so that none stays held down
	case SDL_KEYDOWN:
		_menu->KeyDown(event.key.keysym.sym);
		return true;
	default:
		return false;
	}
}

GameMenu::Action GameInterface::TakeAction()
{
	return std::exchange(_action, GameMenu::Action::None);
}

void GameInterface::Update(float deltaSeconds)
{
	_menu->Update(deltaSeconds);
}

void GameInterface::Draw(glm::u16vec2 resolution, glm::ivec2 mouse, uint32_t milliseconds, bool overDebugWindow)
{
	_canvas.Begin(resolution);
	_pointerCanvas.Begin(resolution);
	_painter.Begin(resolution);
	const bool menuOpen = _menu->IsVisible() && _menu->IsOpen();
	if (_menu->IsVisible())
	{
		_menu->Draw(_painter);
	}
	if (menuOpen || overDebugWindow)
	{
		_painter.DrawPointer(_pointerCanvas, mouse, milliseconds);
	}
	_canvas.End();
	_pointerCanvas.End();
}
