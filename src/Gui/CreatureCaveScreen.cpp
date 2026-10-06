/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureCaveScreen.h"

#include <algorithm>
#include <string>
#include <string_view>

#include <fmt/format.h>
#include <imgui.h>

#include "3D/TempleScroll.h"
#include "3D/TempleScrolls.h"
#include "Creature/CreatureCave.h"
#include "ECS/Components/CreatureSkin.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureCaveSystemInterface.h"
#include "GameInterface.h"
#include "Locator.h"
#include "TextDatabase.h"

namespace openblack::gui
{

namespace
{
/// The screen stands at the right of the screen, this share of it wide
constexpr float k_WidthShare = 0.3f;
constexpr float k_Margin = 16.0f;
/// The lessons shown each way
constexpr size_t k_Lessons = 8;

std::string Text(const GameInterface* interface, std::string_view name, std::string_view otherwise)
{
	if (interface == nullptr)
	{
		return std::string(otherwise);
	}
	const auto text = interface->GetTexts().Get(name);
	return text.empty() ? std::string(otherwise) : ToUtf8(text);
}

/// A scroll's text, a line at a time: the scrolls end their lines with "<N>" and their text with "<E>", and keep the
/// spaces they mustn't break at as a hidden character
void DrawScroll(const GameInterface& interface, TempleScrolls::Content content, const TempleScrolls::Facts& facts)
{
	auto text = TempleScrolls::Write(content, interface.GetTexts(), interface.GetFont(), facts);
	if (!text.has_value())
	{
		return;
	}
	std::ranges::replace(*text, TempleScrollText::k_Hidden, u' ');
	auto utf8 = ToUtf8(*text);
	if (const auto end = utf8.find("<E>"); end != std::string::npos)
	{
		utf8.resize(end);
	}
	std::string_view rest = utf8;
	constexpr std::string_view k_NewLine = "<N>";
	while (!rest.empty())
	{
		const auto end = rest.find(k_NewLine);
		const auto line = rest.substr(0, end);
		ImGui::TextUnformatted(line.data(), line.data() + line.size());
		if (end == std::string_view::npos)
		{
			break;
		}
		rest.remove_prefix(end + k_NewLine.size());
	}
}

void DrawLessons(const creature_cave::Snapshot& snapshot)
{
	const auto lessons = creature_cave::LessonsOf(snapshot.opinions, k_Lessons);
	ImGui::SeparatorText("What it has learnt to do");
	if (lessons.toDo.empty())
	{
		ImGui::TextDisabled("Nothing yet");
	}
	for (const auto& lesson : lessons.toDo)
	{
		ImGui::Text("%s  %+.0f%%", lesson.action.c_str(), static_cast<double>(lesson.opinion * 100.0f));
	}
	ImGui::SeparatorText("What it has learnt not to do");
	if (lessons.notToDo.empty())
	{
		ImGui::TextDisabled("Nothing yet");
	}
	for (const auto& lesson : lessons.notToDo)
	{
		ImGui::Text("%s  %+.0f%%", lesson.action.c_str(), static_cast<double>(lesson.opinion * 100.0f));
	}
	ImGui::SeparatorText("Skills learnt by watching");
	for (const auto& skill : snapshot.skills)
	{
		ImGui::Text("%s %s", skill.known ? "[x]" : "[ ]", skill.name.c_str());
	}
}

void DrawMiracles(const creature_cave::Snapshot& snapshot)
{
	ImGui::SeparatorText("How far it has learnt each miracle");
	for (const auto& miracle : snapshot.miracles)
	{
		ImGui::ProgressBar(static_cast<float>(miracle.percent) / 100.0f, ImVec2(ImGui::GetFontSize() * 6.0f, 0.0f));
		ImGui::SameLine();
		ImGui::TextUnformatted(miracle.name.c_str());
	}
}

void DrawTattoos(ecs::systems::CreatureCaveSystemInterface& cave, const GameInterface* interface)
{
	auto& screen = cave.GetScreen();
	ImGui::TextWrapped(
	    "%s",
	    Text(interface, "HELP_TEXT_DIALOG_TATOODRAG", "Drag the symbols onto or off your Creature to tattoo him.").c_str());
	const auto creature = cave.GetCreature();
	const auto* tattoos = creature.has_value() && Locator::entitiesRegistry::has_value()
	                          ? Locator::entitiesRegistry::value().TryGet<const ecs::components::CreatureTattoos>(*creature)
	                          : nullptr;
	ImGui::SeparatorText(Text(interface, "HELP_TEXT_DIALOG_TATOOPOSITION", "Tattoo Position").c_str());
	for (uint8_t site = 0; site < creature_tattoo::k_SlotCount; ++site)
	{
		const auto name = Text(interface, creature_cave::k_SiteNames.at(site), fmt::format("Place {}", site));
		std::string worn = "-";
		if (tattoos != nullptr)
		{
			for (const auto& slot : tattoos->slots)
			{
				if (slot.site == site)
				{
					worn = fmt::format("symbol {}", slot.design + 1);
				}
			}
		}
		if (ImGui::RadioButton(fmt::format("{}##site{}", name, site).c_str(), screen.site == site))
		{
			screen.site = site;
		}
		ImGui::SameLine(ImGui::GetFontSize() * 9.0f);
		ImGui::TextDisabled("%s", worn.c_str());
	}
	ImGui::SeparatorText("Symbol");
	int design = screen.design;
	if (ImGui::SliderInt("##design", &design, 0, static_cast<int>(creature_tattoo::k_DesignCount) - 1, "symbol %d"))
	{
		screen.design = static_cast<uint8_t>(design);
	}
	float colour[3] = {screen.colour.r / 255.0f, screen.colour.g / 255.0f, screen.colour.b / 255.0f};
	if (ImGui::ColorEdit3("##colour", colour))
	{
		screen.colour = glm::u8vec3(glm::vec3(colour[0], colour[1], colour[2]) * 255.0f);
	}
	ImGui::BeginDisabled(tattoos == nullptr);
	if (ImGui::Button(Text(interface, "HELP_TEXT_DIALOG_EDITTATOO", "Edit Tattoo").c_str()))
	{
		cave.ApplyTattoo(screen.site, screen.design, screen.colour);
	}
	ImGui::SameLine();
	if (ImGui::Button("Remove"))
	{
		cave.RemoveTattoo(screen.site);
	}
	ImGui::EndDisabled();
	if (tattoos == nullptr)
	{
		ImGui::TextDisabled("Your creature has no skin to tattoo");
	}
}
} // namespace

void DrawCreatureCaveScreen()
{
	if (!Locator::creatureCaveSystem::has_value())
	{
		return;
	}
	auto& cave = Locator::creatureCaveSystem::value();
	if (!cave.IsOpen())
	{
		return;
	}
	const auto* interface = cave.GetInterface();
	auto& screen = cave.GetScreen();
	const auto& io = ImGui::GetIO();
	const float width = io.DisplaySize.x * k_WidthShare;
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - width - k_Margin, k_Margin), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(width, io.DisplaySize.y - (2.0f * k_Margin)), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.8f);
	const auto title = Text(interface, "HELP_TEXT_ROOM_CREATURE_TITLE", "Creature Cave") + "###CreatureCave";
	bool open = true;
	if (!ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		return;
	}
	const auto snapshot = cave.Snapshot();
	if (!snapshot.has_value())
	{
		ImGui::TextWrapped("You have no creature yet.");
	}
	else if (ImGui::BeginTabBar("##pages"))
	{
		auto facts = TempleScrolls::Facts::Mock();
		facts.creature = creature_cave::FactsOf(*snapshot);
		for (size_t i = 0; i < creature_cave::k_PageCount; ++i)
		{
			const auto page = static_cast<creature_cave::Page>(i);
			const auto label =
			    Text(interface, creature_cave::TitleName(page), fmt::format("Page {}", i)) + fmt::format("##page{}", i);
			const auto flags = screen.pageRequested && screen.page == page ? ImGuiTabItemFlags_SetSelected : 0;
			if (!ImGui::BeginTabItem(label.c_str(), nullptr, flags))
			{
				continue;
			}
			if (!screen.pageRequested)
			{
				screen.page = page;
			}
			ImGui::BeginChild("##page");
			if (const auto content = creature_cave::ContentOf(page); content.has_value() && interface != nullptr)
			{
				DrawScroll(*interface, *content, facts);
			}
			if (page == creature_cave::Page::ActionsLearnt)
			{
				DrawLessons(*snapshot);
			}
			else if (page == creature_cave::Page::Miracles)
			{
				DrawMiracles(*snapshot);
			}
			else if (page == creature_cave::Page::Tattoos)
			{
				DrawTattoos(cave, interface);
			}
			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
		screen.pageRequested = false;
	}
	ImGui::End();
	if (!open)
	{
		cave.Close();
	}
}

} // namespace openblack::gui
