/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <algorithm>
#include <array>
#include <filesystem>
#include <memory>
#include <ranges>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <MindFile.h>
#include <fmt/format.h>
#include <imgui.h>

#include "Creature/CreatureDecisionTree.h"
#include "Creature/CreatureDesires.h"
#include "Creature/CreatureLearning.h"
#include "Creature/CreatureMindModel.h"
#include "Creature/CreatureMindTables.h"
#include "Creature/CreaturePlanner.h"
#include "Creature/CreatureWatching.h"
#include "CreatureSpawner.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::debug::gui;
using creature_desires::Desire;
using openblack::ecs::components::CreatureMindState;
using openblack::ecs::components::Transform;

namespace
{
std::string Narrow(const std::u16string& text)
{
	std::string narrow;
	for (const auto c : text)
	{
		narrow.push_back(c < 0x80 ? static_cast<char>(c) : '?');
	}
	return narrow;
}

/// The mind files in a folder, leaving out the bodies the game saves beside the creatures' minds
std::vector<std::filesystem::path> MindFilesIn(const std::filesystem::path& folder)
{
	std::vector<std::filesystem::path> files;
	std::error_code error;
	for (const auto& entry : std::filesystem::directory_iterator(folder, error))
	{
		const auto name = entry.path().filename().string();
		if (entry.is_regular_file(error) && !name.starts_with("Physique"))
		{
			files.push_back(entry.path());
		}
	}
	std::ranges::sort(files);
	return files;
}

std::string ActionName(const creature_mind_tables::Tables* tables, uint32_t action)
{
	return tables != nullptr && action < tables->actions.size() ? tables->actions[action].name : fmt::format("#{}", action);
}
} // namespace

void CreatureSpawner::DrawLearning(entt::entity entity) noexcept
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* mind = registry.TryGet<CreatureMindState>(entity);
	if (mind == nullptr || !Locator::creatureMindSystem::has_value())
	{
		return;
	}
	auto& minds = Locator::creatureMindSystem::value();
	const auto* tables = minds.GetTables();

	ImGui::SeparatorText("Learning mind");
	if (!mind->learnt.has_value() || !mind->desires.has_value())
	{
		ImGui::TextUnformatted("The mind hasn't thought yet");
		return;
	}
	auto& learnt = *mind->learnt;
	const auto& desires = *mind->desires;
	ImGui::Text("Name \"%s\", stage %u, attitude to the player %+.2f", Narrow(learnt.name).c_str(), mind->developmentPhase,
	            static_cast<double>(mind->attitudeToPlayer));
	if (learnt.file != nullptr)
	{
		ImGui::Text("From a version %u mind file saved as \"%s\" by \"%s\"", learnt.file->version,
		            learnt.file->SavedAsText().c_str(), Narrow(learnt.file->ProfileText()).c_str());
	}

	if (ImGui::TreeNodeEx("Mind files", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::Button("Open mind file..."))
		{
			ChooseMindFile(MindFileUse::LoadIntoSelected);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Loads a saved creature's mind into this creature: what it has learnt and its name");
		}
		ImGui::SameLine();
		if (ImGui::Button("Save mind as..."))
		{
			ChooseMindFile(MindFileUse::SaveSelected);
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear learning"))
		{
			minds.ClearLearning(entity);
			_lastMindFile = "Learning cleared";
		}
		if (!_lastMindFile.empty())
		{
			ImGui::TextWrapped("%s", _lastMindFile.c_str());
		}
		if (Locator::filesystem::has_value())
		{
			// The game's own minds, and the creatures it saved between lands
			const auto folder = Locator::filesystem::value().GetPath<filesystem::Path::CreatureMind>(true);
			const auto files = MindFilesIn(folder);
			if (ImGui::TreeNode("game", "The game's mind files: %zu", files.size()))
			{
				for (const auto& path : files)
				{
					ImGui::PushID(path.generic_string().c_str());
					if (ImGui::SmallButton("Load"))
					{
						UseMindFile(MindFileUse::LoadIntoSelected, path);
					}
					ImGui::SameLine();
					ImGui::TextUnformatted(path.filename().string().c_str());
					ImGui::PopID();
				}
				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}

	{
		// The trainer strokes it for each thing it does to the kind of thing picked, and slaps it for anything else
		constexpr std::array<std::pair<const char*, std::optional<uint32_t>>, 6> k_Trainers {{
		    {"No trainer", std::nullopt},
		    {"Reward villagers", creature_tree::belief_types::k_Villager},
		    {"Reward objects", creature_tree::belief_types::k_Other},
		    {"Reward trees", creature_tree::belief_types::k_Tree},
		    {"Reward creatures", creature_tree::belief_types::k_Creature},
		    {"Reward abodes", creature_tree::belief_types::k_Abode},
		}};
		const auto current =
		    std::ranges::find(k_Trainers, mind->trainer, &std::pair<const char*, std::optional<uint32_t>>::second);
		if (ImGui::BeginCombo("Trainer", current != k_Trainers.end() ? current->first : "Custom"))
		{
			for (const auto& [label, kind] : k_Trainers)
			{
				if (ImGui::Selectable(label, mind->trainer == kind))
				{
					mind->trainer = kind;
				}
			}
			ImGui::EndCombo();
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Strokes the creature for each thing it does to that kind of thing and slaps it for anything "
			                  "else, as soon as it is done");
		}
	}
	ImGui::TextUnformatted("Feedback");
	ImGui::SameLine();
	if (ImGui::SmallButton("Stroke"))
	{
		minds.ReceiveFeedback(entity, 1.0f);
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Slap"))
	{
		minds.ReceiveFeedback(entity, -1.0f);
	}

	if (ImGui::TreeNodeEx("Planner", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const auto& planner = mind->planner;
		if (planner.current.has_value())
		{
			ImGui::Text("Carrying out %s for %s, priority %.1f", ActionName(tables, planner.current->action).c_str(),
			            creature_desires::Name(planner.current->desire).data(), static_cast<double>(planner.current->priority));
		}
		else
		{
			ImGui::TextUnformatted("No plan: doing as it pleases with nothing better to do");
		}
		if (ImGui::BeginTable("plans", 5, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Desire");
			ImGui::TableSetupColumn("Value");
			ImGui::TableSetupColumn("Action");
			ImGui::TableSetupColumn("Goal (usefulness)");
			ImGui::TableSetupColumn("Priority");
			ImGui::TableHeadersRow();
			for (size_t d = 0; d < creature_desires::k_DesireCount; ++d)
			{
				const auto& plan = planner.best.at(d);
				if (!plan.has_value())
				{
					continue;
				}
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(creature_desires::Name(plan->desire).data());
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", static_cast<double>(desires.desires.at(d).value));
				ImGui::TableNextColumn();
				ImGui::Text("%s (%.4f)", ActionName(tables, plan->action).c_str(), static_cast<double>(plan->actionPriority));
				ImGui::TableNextColumn();
				if (plan->object.has_value())
				{
					ImGui::Text("entity %u (%.2f)", *plan->object, static_cast<double>(plan->goalUsefulness));
				}
				else
				{
					ImGui::TextUnformatted("-");
				}
				ImGui::TableNextColumn();
				ImGui::Text("%.1f", static_cast<double>(plan->priority));
			}
			ImGui::EndTable();
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Desires and likes"))
	{
		for (size_t d = 0; d < creature_desires::k_DesireCount; ++d)
		{
			const auto& state = desires.desires.at(d);
			if (!state.activated)
			{
				continue;
			}
			const auto desire = static_cast<Desire>(d);
			const auto likes = creature_learning::LikesLevel(desire, state, learnt.initialThresholds.at(d));
			ImGui::Text("%-18s %.2f / %.2f, grows in %.0f s%s, likes it %s", creature_desires::Name(desire).data(),
			            static_cast<double>(state.value), static_cast<double>(state.max),
			            static_cast<double>(state.increaseSeconds), state.suppressedTurns > 0 ? ", held back" : "",
			            creature_learning::LikesWords(likes).data());
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Opinions of actions"))
	{
		for (size_t a = 0; a < learnt.opinions.size(); ++a)
		{
			if (learnt.opinions[a] != 0.0f)
			{
				ImGui::Text("%-32s %+.2f", ActionName(tables, static_cast<uint32_t>(a)).c_str(),
				            static_cast<double>(learnt.opinions[a]));
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Decision trees"))
	{
		for (size_t k = 0; k < creature_mind_model::k_TreeKinds; ++k)
		{
			for (size_t d = 0; d < creature_desires::k_DesireCount; ++d)
			{
				const auto& episodes = learnt.episodes.at(k).at(d);
				if (episodes.empty())
				{
					continue;
				}
				const auto label =
				    fmt::format("{} - {}: {} examples##tree{}_{}", creature_desires::Name(static_cast<Desire>(d)),
				                k == 0 ? "what to act on" : "what to use", episodes.size(), k, d);
				if (ImGui::TreeNode(label.c_str()))
				{
					for (const auto& line : creature_tree::Describe(learnt.trees.at(k).at(d)))
					{
						ImGui::TextUnformatted(line.c_str());
					}
					ImGui::TreePop();
				}
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Recent actions", ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (const auto& context : learnt.contexts | std::views::reverse)
		{
			const auto priority = creature_learning::LearningPriority(context);
			ImGui::Text("%s for %s%s%s: %s, credit %.2f%s", ActionName(tables, context.action).c_str(),
			            creature_desires::Name(context.desire).data(), context.belief.has_value() ? " on a " : "",
			            context.belief.has_value() ? creature_tree::BeliefName(context.belief->type) : "",
			            context.running ? "running" : fmt::format("{:.0f} s ago", context.secondsSince).c_str(),
			            static_cast<double>(priority),
			            context.credited.has_value() ? fmt::format(", fed back {:+.2f}", *context.credited).c_str() : "");
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Attitudes"))
	{
		ImGui::Text("To the player %+.2f, average feedback %+.2f", static_cast<double>(mind->attitudeToPlayer),
		            static_cast<double>(mind->averageFeedback));
		for (const auto& attitude : learnt.creatures)
		{
			ImGui::Text("Creature %u: nice %+.2f, impressive %.2f, attention %.2f", attitude.creature,
			            static_cast<double>(attitude.howNice), static_cast<double>(attitude.howImpressive),
			            static_cast<double>(attitude.attention));
		}
		ImGui::TreePop();
	}

	if (tables != nullptr && ImGui::TreeNode("Skills, miracles and copying"))
	{
		const auto* transform = registry.TryGet<const Transform>(entity);
		const auto& knowledge = learnt.knowledge;
		for (size_t i = 0; i < tables->skills.size() && i < knowledge.skillsSeen.size(); ++i)
		{
			ImGui::Text("%s %-20s seen %u", knowledge.skillsKnown.at(i) ? "[known]" : "[     ]", tables->skills[i].name.c_str(),
			            knowledge.skillsSeen[i].count);
		}
		for (size_t i = 0; i < tables->miracles.size() && i < knowledge.miraclesSeen.size(); ++i)
		{
			if (knowledge.miraclesKnown.at(i) || knowledge.miraclesSeen[i].count > 0)
			{
				ImGui::Text("%s %-28s seen %u", knowledge.miraclesKnown.at(i) ? "[known]" : "[     ]",
				            tables->miracles[i].name.c_str(), knowledge.miraclesSeen[i].count);
			}
		}
		const auto pick = [](const char* label, int& index, const auto& rows) {
			if (rows.empty())
			{
				return;
			}
			index = std::clamp(index, 0, static_cast<int>(rows.size()) - 1);
			if (ImGui::BeginCombo(label, rows.at(static_cast<size_t>(index)).name.c_str()))
			{
				for (size_t i = 0; i < rows.size(); ++i)
				{
					if (ImGui::Selectable(fmt::format("{}##{}", rows[i].name, i).c_str(), static_cast<int>(i) == index))
					{
						index = static_cast<int>(i);
					}
				}
				ImGui::EndCombo();
			}
		};
		if (transform != nullptr)
		{
			pick("Skill", _skill, tables->skills);
			ImGui::SameLine();
			if (ImGui::SmallButton("Show it##skill"))
			{
				minds.SeeSkill(transform->position, static_cast<size_t>(_skill));
			}
			pick("Miracle", _miracle, tables->miracles);
			ImGui::SameLine();
			if (ImGui::SmallButton("Cast it near##miracle"))
			{
				minds.SeeMiracle(transform->position, static_cast<size_t>(_miracle));
			}
			pick("Player's deed", _deed, tables->mimics);
			ImGui::SameLine();
			if (ImGui::SmallButton("Do it near##deed"))
			{
				minds.PlayerDid(static_cast<size_t>(_deed), transform->position + glm::vec3(20.0f, 0.0f, 0.0f), std::nullopt);
			}
		}
		if (learnt.mimicry.has_value())
		{
			ImGui::Text("Copying \"%s\": %s, %u steps left", tables->mimics.at(learnt.mimicry->rule).name.c_str(),
			            creature_watching::Name(learnt.mimicry->stage), learnt.mimicry->stepsLeft);
		}
		ImGui::TreePop();
	}

	ImGui::TextUnformatted("Thoughts:");
	for (const auto& thought : learnt.thoughts | std::views::reverse)
	{
		ImGui::BulletText("%s", thought.c_str());
	}
}
