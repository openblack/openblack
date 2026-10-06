/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GameInterface.h"

#include <cmath>

#include <algorithm>
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
#include "ECS/Systems/CinematicDirectorSystemInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "Graphics/Texture2D.h"
#include "Gui/CinemaBars.h"
#include "InfoConstants.h"
#include "Locator.h"

using namespace openblack::gui;
using openblack::Locator;
using openblack::filesystem::Path;

namespace
{
/// The scripts the game's text is in, in the order the game reads them
constexpr std::array k_TextScripts = {"InfoScript2.txt", "InfoScriptPatch2.txt", "InfoScriptMultiplayer2.txt"};
/// The dialogs' font, the first of the game's fonts
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

/// The tooltips' priorities and times from the info script, or the most common of them without it
std::array<ToolTipInfo, ToolTips::k_Count> ReadToolTipsInfo()
{
	std::array<ToolTipInfo, ToolTips::k_Count> info {};
	info.fill({.priority = 0.5f, .displayTime = 1.0f, .displayTimeAfterFocus = 0.0f});
	if (Locator::infoConstants::has_value())
	{
		const auto& toolTips = Locator::infoConstants::value().toolTips;
		for (size_t i = 0; i < info.size(); ++i)
		{
			info.at(i) = {.priority = toolTips.at(i).priority,
			              .displayTime = toolTips.at(i).displayTime,
			              .displayTimeAfterFocus = toolTips.at(i).displayTimeAfterFocus};
		}
	}
	return info;
}

/// atmos.raw's glow, which the tooltips' glow boxes are cut from, a ninth at a time
constexpr std::array k_GlowU = {0.0f, 0.078125f, 0.16797f, 0.24609f};
constexpr std::array k_GlowV = {0.25f, 0.32813f, 0.41797f, 0.49609f};
/// atmos.raw's arrows about a tooltip's mouse
struct ToolTipArrow
{
	uint32_t bit;
	glm::vec2 uvMin;
	glm::vec2 uvMax;
};
constexpr std::array k_ToolTipArrows = {
    ToolTipArrow {ToolTipArrows::k_Left, {0.7539f, 0.1289f}, {0.8711f, 0.2461f}},
    ToolTipArrow {ToolTipArrows::k_Right, {0.8789f, 0.0039f}, {0.9961f, 0.1211f}},
    ToolTipArrow {ToolTipArrows::k_Up, {0.7539f, 0.0039f}, {0.8711f, 0.1211f}},
    ToolTipArrow {ToolTipArrows::k_Down, {0.8789f, 0.1289f}, {0.9961f, 0.2461f}},
};
/// The tooltips' mice for a wheel mouse, the third row of mousehelp.raw
constexpr int k_ToolTipMouseRow = 2;
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

	// The front end atlas, and the pictures of the player's symbols and of the tooltips' mice
	auto atlas = LoadTexture("Front_end_buttons");
	if (!atlas)
	{
		return nullptr;
	}
	auto symbols = LoadTexture("ChooseSymbol");
	auto mice = LoadTexture("mousehelp");
	// The atmosphere texture's glows and the tooltips' arrows
	auto atmos = LoadTexture("ATMOS");

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
	                                                        std::move(atmos), playerName, std::move(settings)));
}

GameInterface::GameInterface(TextDatabase texts, GameFont font, std::unique_ptr<graphics::Texture2D> atlas,
                             std::unique_ptr<graphics::Texture2D> fontTexture, std::unique_ptr<graphics::Texture2D> symbols,
                             std::unique_ptr<graphics::Texture2D> mice, std::unique_ptr<graphics::Texture2D> atmos,
                             std::u16string_view playerName, MenuSettings settings)
    : _texts(std::move(texts))
    , _font(std::move(font))
    , _atlas(std::move(atlas))
    , _fontTexture(std::move(fontTexture))
    , _symbols(std::move(symbols))
    , _mice(std::move(mice))
    , _atmos(std::move(atmos))
    , _painter(_canvas, _font, *_atlas, *_fontTexture)
    , _menu(std::make_unique<GameMenu>(_texts, _font, playerName, std::move(settings)))
    , _toolTips(ReadToolTipsInfo())
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
			// In the game's dialogs every control clicks as it acts
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
	_toolTips.Update(deltaSeconds);
	_screenFade.Update(deltaSeconds);
}

void GameInterface::Draw(glm::u16vec2 resolution, glm::ivec2 mouse, uint32_t milliseconds, bool overDebugWindow)
{
	_canvas.Begin(resolution);
	_pointerCanvas.Begin(resolution);
	_painter.Begin(resolution);
	const bool menuOpen = _menu->IsVisible() && _menu->IsOpen();
	if (_message.has_value())
	{
		_painter.DrawTextWrapped(DialogRect {{0, 0}, DialogPainter::k_Size}, true, _message->text, 60,
		                         glm::vec4(1.0f, 1.0f, 1.0f, _message->alpha));
	}
	// The game leaves the creature's panel and the tooltip out under a dialog
	if (!menuOpen)
	{
		DrawFightPanel(resolution);
		DrawCreaturePanel(resolution);
		DrawToolTip(resolution);
	}
	if (_menu->IsVisible())
	{
		_menu->Draw(_painter);
	}
	if (menuOpen || overDebugWindow)
	{
		_painter.DrawPointer(_pointerCanvas, mouse, milliseconds);
	}
	// The scripts' fade covers the picture between the cinema bars, which are black
	const auto& director = Locator::cinematicDirectorSystem::value();
	const auto bars = static_cast<float>(CinemaBars::BarHeight(resolution.x, resolution.y, director.GetWideScreenFraction()));
	const auto screen = glm::vec2(resolution);
	if (const auto fade = director.GetFadeColour(); (fade >> 24u) != 0)
	{
		const auto colour = glm::vec4(static_cast<float>((fade >> 16u) & 0xFFu), static_cast<float>((fade >> 8u) & 0xFFu),
		                              static_cast<float>(fade & 0xFFu), static_cast<float>(fade >> 24u)) /
		                    255.0f;
		_canvas.DrawQuad({0.0f, bars}, {screen.x, screen.y - bars}, glm::vec2(0.0f), glm::vec2(1.0f), colour, nullptr);
	}
	if (bars > 0.0f)
	{
		const auto black = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		_canvas.DrawQuad(glm::vec2(0.0f), {screen.x, bars}, glm::vec2(0.0f), glm::vec2(1.0f), black, nullptr);
		_canvas.DrawQuad({0.0f, screen.y - bars}, screen, glm::vec2(0.0f), glm::vec2(1.0f), black, nullptr);
	}
	// The game covers the frame with the fade's colour last of all
	if (const auto fade = _screenFade.GetColour(); fade.a > 0.0f)
	{
		_canvas.DrawQuad(glm::vec2(0.0f), glm::vec2(resolution), glm::vec2(0.0f), glm::vec2(1.0f), fade, nullptr);
	}
	_canvas.End();
	_pointerCanvas.End();
}

void GameInterface::DrawGlow(glm::vec2 min, glm::vec2 max, glm::vec4 colour)
{
	// A ninth of the glow at each corner, a quarter of the box's height square, outside the box
	if (_atmos == nullptr || colour.a <= 0.0f)
	{
		return;
	}
	const float corner = (max.y - min.y) * 0.25f;
	const std::array x = {min.x - corner, min.x, max.x, max.x + corner};
	const std::array y = {min.y - corner, min.y, max.y, max.y + corner};
	_canvas.SetBlend(Canvas::Blend::Additive);
	for (size_t row = 0; row < 3; ++row)
	{
		for (size_t column = 0; column < 3; ++column)
		{
			_canvas.DrawQuad({x.at(column), y.at(row)}, {x.at(column + 1), y.at(row + 1)},
			                 {k_GlowU.at(column), k_GlowV.at(row)}, {k_GlowU.at(column + 1), k_GlowV.at(row + 1)}, colour,
			                 _atmos.get());
		}
	}
	_canvas.SetBlend(Canvas::Blend::Alpha);
}

void GameInterface::DrawCreaturePanel(glm::u16vec2 resolution)
{
	if (!_creaturePanel.has_value())
	{
		return;
	}
	using creature_panel::Row;
	const auto& values = *_creaturePanel;
	const bool withReward = values.reward.has_value();
	const float textSize = static_cast<float>(resolution.y) / 32.0f;

	// The labels' column is as wide as the widest label
	std::array<std::u16string_view, creature_panel::k_RowCount> labels {};
	std::array<float, creature_panel::k_RowCount> widths {};
	for (size_t i = 0; i < labels.size(); ++i)
	{
		labels.at(i) = _texts.Get(creature_panel::k_LabelNames.at(i));
		widths.at(i) = _font.GetWidth(labels.at(i), textSize);
	}
	const auto layout =
	    creature_panel::Compute(resolution, withReward, creature_panel::LabelColumnWidth(widths, textSize * 2.0f));
	const creature_panel::RewardTexts rewardTexts {
	    .bad = _texts.Get(creature_panel::k_BadBoyName),
	    .good = _texts.Get(creature_panel::k_GoodBoyName),
	    .none = _texts.Get(creature_panel::k_NoRewardName),
	};

	// See-through black at the left, fading out to the right
	const glm::vec4 shade {0.0f, 0.0f, 0.0f, 95.0f / 255.0f};
	const glm::vec4 clear {0.0f, 0.0f, 0.0f, 0.0f};
	const auto& box = layout.box;
	_canvas.DrawShape({box.min, glm::vec2(box.max.x, box.min.y), box.max, glm::vec2(box.min.x, box.max.y)},
	                  {shade, clear, clear, shade});

	const glm::vec4 white {1.0f, 1.0f, 1.0f, 1.0f};
	const glm::vec4 black {0.0f, 0.0f, 0.0f, 1.0f};
	// Each text in white over its black shadow, two pixels down and right
	const auto drawText = [&](glm::vec2 at, std::u16string_view text) {
		_painter.DrawString(at + 2.0f, text, layout.textSize, black);
		_painter.DrawString(at, text, layout.textSize, white);
	};
	for (const auto& row : layout.Rows())
	{
		const auto label = labels.at(static_cast<size_t>(row.row));
		drawText({row.labelRight.x - std::floor(_font.GetWidth(label, layout.textSize)), row.labelRight.y}, label);
		drawText(row.valueLeft, creature_panel::FormatValue(row.row, values, rewardTexts));

		// The bar's dark box and white frame, its fill brightening towards where it ends, and shadows inside its frame
		const auto& bar = row.bar;
		_painter.DrawBevelBox({_painter.ToDialog(glm::ivec2(bar.min)), _painter.ToDialog(glm::ivec2(bar.max))}, 1,
		                      DialogPainter::All, white);
		const float fill = creature_panel::Fill(row.row, values);
		const auto filled = creature_panel::FillOf(bar, fill, row.row == Row::Reward);
		if (!filled.Empty())
		{
			const auto bright = creature_panel::BarColour(row.row, fill);
			const auto dim = glm::vec4(glm::vec3(bright) * 0.5f, 1.0f);
			_canvas.DrawShape({glm::vec2(filled.from, filled.top), glm::vec2(filled.to, filled.top),
			                   glm::vec2(filled.to, filled.bottom), glm::vec2(filled.from, filled.bottom)},
			                  {dim, bright, bright, dim});
		}
		if (filled.middle.has_value())
		{
			_canvas.DrawLine(glm::ivec2(static_cast<int>(*filled.middle), static_cast<int>(filled.top)),
			                 glm::ivec2(static_cast<int>(*filled.middle), static_cast<int>(filled.bottom) - 1), black);
		}
		const float inset = creature_panel::k_BarInset;
		const glm::vec2 inner {bar.min.x + inset, bar.min.y + inset};
		const glm::vec2 innerMax {bar.max.x - inset, bar.max.y - inset};
		_canvas.DrawShape({inner, glm::vec2(innerMax.x, inner.y), glm::vec2(innerMax.x, inner.y + inset),
		                   glm::vec2(inner.x, inner.y + inset)},
		                  {black, black, clear, clear});
		_canvas.DrawShape({inner, glm::vec2(inner.x + inset, inner.y), glm::vec2(inner.x + inset, innerMax.y),
		                   glm::vec2(inner.x, innerMax.y)},
		                  {black, clear, clear, black});
	}
}

void GameInterface::DrawFightPanel(glm::u16vec2 resolution)
{
	if (!_fightPanel.has_value())
	{
		return;
	}
	const auto layout = creature_fight_hud::Compute(resolution);
	const glm::vec4 shade {0.0f, 0.0f, 0.0f, 95.0f / 255.0f};
	const glm::vec4 white {1.0f, 1.0f, 1.0f, 1.0f};
	const glm::vec4 black {0.0f, 0.0f, 0.0f, 1.0f};
	const auto quad = [this](const creature_fight_hud::Rect& rect, const glm::vec4& colour) {
		_canvas.DrawShape({rect.min, glm::vec2(rect.max.x, rect.min.y), rect.max, glm::vec2(rect.min.x, rect.max.y)},
		                  {colour, colour, colour, colour});
	};
	quad(layout.box, shade);
	for (size_t i = 0; i < layout.rows.size(); ++i)
	{
		const auto& row = layout.rows.at(i);
		const auto& side = _fightPanel->sides.at(i);
		// The name in white over its black shadow
		_painter.DrawString(row.name + 2.0f, side.name, layout.textSize, black);
		_painter.DrawString(row.name, side.name, layout.textSize, white);
		for (const auto& [bar, value] : {std::pair(row.health, side.health), std::pair(row.stamina, side.stamina)})
		{
			quad(bar, black);
			quad(creature_fight_hud::Filled(bar, value), creature_fight_hud::BarColour(value));
		}
	}
}

void GameInterface::DrawToolTip(glm::u16vec2 resolution)
{
	const auto shown = _toolTips.GetShown();
	if (!shown.has_value() || !_handOnScreen.has_value())
	{
		return;
	}
	// The colours' alpha goes in 256ths, and below 4 nothing is drawn
	const float alpha = std::floor(shown->alpha * 255.0f) / 255.0f;
	if (alpha * 255.0f < 4.0f)
	{
		return;
	}
	const auto text = _texts.Get(ToolTips::TextName(shown->index));
	const glm::vec4 yellow {1.0f, 1.0f, 0.0f, alpha};
	const glm::vec4 white {1.0f, 1.0f, 1.0f, alpha};
	const glm::vec4 shadow {0.0f, 0.0f, 0.0f, alpha};

	// Sized by the screen's height, and kept on it by the hand
	const int width = resolution.x;
	const int height = resolution.y;
	const int size = height / 25;
	const int handX = std::clamp(static_cast<int>(_handOnScreen->x), 0, width - size);
	const int handY = std::clamp(static_cast<int>(_handOnScreen->y), 0, height - size);

	// The words, then the mouse with its arrows either side
	const int iconWidth = shown->action != ToolTipAction::None && _mice != nullptr ? size : 0;
	const bool left = (shown->arrows & ToolTipArrows::k_Left) != 0;
	const bool right = (shown->arrows & ToolTipArrows::k_Right) != 0;
	const int iconsWidth = iconWidth + (left ? size / 2 : 0) + (right ? size / 2 : 0);
	const int textSize = size * 2 / 3;
	const float textWidth = _font.GetWidth(text, static_cast<float>(textSize));
	const float total = textWidth + static_cast<float>(iconsWidth) + 2.0f;

	// Right of the hand, or left of it in the right third of the screen, until it reaches the left third
	if (handX > width * 2 / 3)
	{
		_toolTipOnLeft = true;
	}
	else if (handX < width / 3)
	{
		_toolTipOnLeft = false;
	}
	const float x = _toolTipOnLeft ? static_cast<float>(handX + (size / 2)) - total : static_cast<float>(handX - (size / 2));
	const float textX = x;
	const float iconX = std::trunc(x + textWidth + 2.0f) + (left ? static_cast<float>(size / 2) : 0.0f);
	const auto y = static_cast<float>(handY);
	const auto sizeF = static_cast<float>(size);
	const auto textOffset = static_cast<float>((size - textSize) / 2);

	// The glows behind: white behind the mouse, and faint yellow behind the words and arrows
	const float glowAlpha = std::trunc(128.0f * shown->alpha) / 255.0f;
	const float faintAlpha = std::trunc(128.0f * shown->alpha / 3.0f) / 255.0f;
	if (iconWidth != 0)
	{
		DrawGlow({iconX, y}, {iconX + static_cast<float>(iconWidth), y + sizeF}, glm::vec4(1.0f, 1.0f, 1.0f, glowAlpha));
	}
	if (!text.empty())
	{
		DrawGlow({textX, y + textOffset}, {textX + textWidth, y + sizeF - textOffset}, glm::vec4(1.0f, 1.0f, 0.0f, faintAlpha));
	}
	const float half = sizeF * 0.5f;
	const float quarter = sizeF * 0.25f;
	const auto arrowRect = [&](uint32_t bit) -> std::pair<glm::vec2, glm::vec2> {
		const float iconRight = iconX + static_cast<float>(iconWidth);
		switch (bit)
		{
		case ToolTipArrows::k_Left:
			return {{iconX - half, y + quarter}, {iconX, y + (3.0f * quarter)}};
		case ToolTipArrows::k_Right:
			return {{iconRight, y + quarter}, {iconRight + half, y + (3.0f * quarter)}};
		case ToolTipArrows::k_Up:
			return {{iconX + quarter, y - half}, {iconX + (3.0f * quarter), y}};
		default:
			return {{iconX + quarter, y + sizeF}, {iconX + (3.0f * quarter), y + (1.5f * sizeF)}};
		}
	};
	for (const auto& arrow : k_ToolTipArrows)
	{
		if ((shown->arrows & arrow.bit) != 0)
		{
			const auto [min, max] = arrowRect(arrow.bit);
			DrawGlow(min, max, glm::vec4(1.0f, 1.0f, 0.0f, faintAlpha));
		}
	}

	// The mouse, its button the action's, and the arrows over the glows
	if (iconWidth != 0)
	{
		constexpr float k_Grid = 0.25f;
		const bool selecting = shown->action == ToolTipAction::Select;
		const auto uvMin = glm::vec2(1.0f, static_cast<float>(k_ToolTipMouseRow)) * k_Grid;
		const auto uvMax = uvMin + k_Grid;
		// The left button is the right one mirrored
		_canvas.DrawQuad({iconX, y}, {iconX + sizeF, y + sizeF}, selecting ? glm::vec2(uvMax.x, uvMin.y) : uvMin,
		                 selecting ? glm::vec2(uvMin.x, uvMax.y) : uvMax, white, _mice.get());
	}
	if (_atmos != nullptr)
	{
		const float grow = static_cast<float>(size / 9);
		for (const auto& arrow : k_ToolTipArrows)
		{
			if ((shown->arrows & arrow.bit) != 0)
			{
				const auto [min, max] = arrowRect(arrow.bit);
				_canvas.DrawQuad(min - grow, max + grow, arrow.uvMin, arrow.uvMax, white, _atmos.get());
			}
		}
	}

	// The words, in yellow over two black shadows
	const auto textSizeF = static_cast<float>(textSize);
	_painter.DrawString({textX - 1.0f, y - 1.0f + textOffset}, text, textSizeF, shadow);
	_painter.DrawString({textX + 1.0f, y + 1.0f + textOffset}, text, textSizeF, shadow);
	_painter.DrawString({textX, y + textOffset}, text, textSizeF, yellow);
}
