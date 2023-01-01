/*******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#include "TextureViewer.h"

#include "Debug/Gui.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"
#include "Resources/Resources.h"

using namespace openblack;
using namespace openblack::debug::gui;

TextureViewer::TextureViewer()
    : Window("Texture Viewer", ImVec2(950.0f, 780.0f))
{
}

void TextureViewer::Draw([[maybe_unused]] Game& game)
{
	float fontSize = ImGui::GetFontSize();
	const auto& textures = Locator::resources::value().GetTextures();

	_filter.Draw();

	ImGui::BeginChild("textures", ImVec2(fontSize * 15.0f, 0));
	auto textureSize = ImGui::GetItemRectSize();
	ImGui::BeginChild("texturesSelect", ImVec2(textureSize.x - 5, textureSize.y - ImGui::GetTextLineHeight() - 5), true);
	uint32_t displayedTexture = 0;

	textures.Each([this, &displayedTexture](entt::id_type id, const graphics::Texture2D& texture) {
		if (_filter.PassFilter(texture.GetName().c_str()))
		{
			displayedTexture++;

			if (ImGui::Selectable(texture.GetName().c_str(), id == _selectedTexture))
			{
				_selectedTexture = id;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", texture.GetName().c_str());
			}
		}
	});
	ImGui::EndChild();
	ImGui::Text("%u textures", displayedTexture);
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("viewer", ImVec2(fontSize * -15.0f, 0));

	if (textures.Contains(_selectedTexture))
	{
		auto texture = textures.Handle(_selectedTexture);

		const auto format = texture->GetFormat();
		std::string formatStr = std::to_string(format);
		if (format == bgfx::TextureFormat::R8)
		{
			formatStr = "R8";
		}
		else if (format == bgfx::TextureFormat::RGB8)
		{
			formatStr = "RGB8";
		}
		ImGui::Text("width: %u, height: %u, format: %s", texture->GetWidth(), texture->GetHeight(), formatStr.c_str());
		ImGui::Image(texture->GetNativeHandle(), ImVec2(512, 512));
	}

	ImGui::EndChild();
}

void TextureViewer::Update([[maybe_unused]] Game& game, [[maybe_unused]] const Renderer& renderer) {}

void TextureViewer::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) {}

void TextureViewer::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) {}
