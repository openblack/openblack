/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>

#include <MindFile.h>
#include <fmt/format.h>
#include <imgui.h>

#include "Common/FileDialog.h"
#include "Creature/CreatureMindFileBody.h"
#include "CreatureSpawner.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

using namespace openblack;
using namespace openblack::debug::gui;

namespace
{
/// Under these names the folder mind files were last chosen in, and the last mind file opened, are remembered
constexpr std::string_view k_FolderKey = "creature-mind-folder";
constexpr std::string_view k_FileKey = "creature-mind-file";

std::string Narrow(const std::u16string& text)
{
	std::string narrow;
	for (const auto c : text)
	{
		narrow.push_back(c < 0x80 ? static_cast<char>(c) : '?');
	}
	return narrow;
}

file_dialog::Request MindFileRequest(bool save)
{
	std::filesystem::path gameFolder;
	if (Locator::filesystem::has_value())
	{
		gameFolder = Locator::filesystem::value().GetPath<filesystem::Path::CreatureMind>(true);
	}
	const auto exists = [](const std::filesystem::path& folder) {
		std::error_code error;
		return std::filesystem::is_directory(folder, error);
	};
	return {
	    .mode = save ? file_dialog::Mode::Save : file_dialog::Mode::Open,
	    .title = save ? "Save the creature's mind as" : "Open a creature mind file",
	    // The game's own mind files have no extension; the creatures it saves between lands end in .erc
	    .filters = {{.name = "All files", .patterns = {"*"}}, {.name = "Saved creatures (*.erc)", .patterns = {"*.erc"}}},
	    .startFolder = file_dialog::StartFolder(file_dialog::RememberedPath(k_FolderKey), gameFolder, exists),
	    .defaultName = save ? "openblack_creature_mind" : "",
	    .owner = Locator::windowing::has_value() ? Locator::windowing::value().GetNativeHandles().nativeWindow : nullptr,
	};
}
} // namespace

void CreatureSpawner::ChooseMindFile(MindFileUse use) noexcept
{
	const auto request = MindFileRequest(use == MindFileUse::SaveSelected);
	const auto outcome = file_dialog::Show(request);
	switch (outcome.status)
	{
	case file_dialog::Status::Chosen:
		UseMindFile(use, outcome.path);
		break;
	case file_dialog::Status::Cancelled:
		break;
	case file_dialog::Status::Unavailable:
		_browserUse = use;
		_fileBrowser.Open(request);
		break;
	}
}

void CreatureSpawner::DrawFileBrowser() noexcept
{
	if (const auto path = _fileBrowser.Draw())
	{
		UseMindFile(_browserUse, *path);
	}
}

void CreatureSpawner::UseMindFile(MindFileUse use, const std::filesystem::path& path) noexcept
{
	file_dialog::RememberPath(k_FolderKey, path.parent_path());
	const auto name = path.filename().string();

	if (use == MindFileUse::SaveSelected)
	{
		if (!_selected.has_value() || !Locator::creatureMindSystem::has_value())
		{
			_lastMindFile = "No creature is selected to save";
			return;
		}
		const auto file = Locator::creatureMindSystem::value().SaveMind(*_selected);
		if (!file.has_value())
		{
			_lastMindFile = "The creature hasn't thought yet, so there is no mind to save";
			return;
		}
		const auto result = creaturemind::WriteFile(path, *file);
		_lastMindFile = fmt::format("Saved to {}: {}", path.generic_string(), creaturemind::ResultToStr(result));
		return;
	}

	auto data = std::make_shared<creaturemind::MindFileData>();
	const auto result = creaturemind::ReadFile(path, *data);
	if (result != creaturemind::MindResult::Success)
	{
		_lastMindFile = fmt::format("Couldn't load {}: {}", name, creaturemind::ResultToStr(result));
		return;
	}
	file_dialog::RememberPath(k_FileKey, path);
	const auto body = creature_mind_body::FromMindFile(*data);
	const auto who = fmt::format("\"{}\", a version {} mind{}", Narrow(data->name), data->version,
	                             body.has_value() ? fmt::format(" of a {}", SpeciesName(body->species)) : "");

	if (use == MindFileUse::LoadIntoSelected)
	{
		if (!_selected.has_value() || !Locator::creatureMindSystem::has_value())
		{
			_lastMindFile = "No creature is selected to load the mind into";
			return;
		}
		Locator::creatureMindSystem::value().LoadMind(*_selected, data);
		_lastMindFile = fmt::format("Loaded {} from {}", who, name);
		return;
	}

	// Spawning from the file: the creature it describes, as far as the file's version keeps it
	if (!body.has_value())
	{
		_lastMindFile = fmt::format("Couldn't spawn from {}: its species isn't one the game knows", name);
		return;
	}
	_species = body->species;
	UseSpeciesDefaults();
	_defaultsFor = _species;
	_alignment = body->alignment.value_or(_alignment);
	_strength = body->strength;
	_scale = body->size.value_or(_scale);
	_spawnTattoos = body->tattoos;
	_spawnMind = data;
	_spawnMindName = fmt::format("{} from {}", who, name);
	_placing = true;
	_placeAt.reset();
	_commanding = false;
	_commandAt.reset();
	_lastMindFile = fmt::format("Spawning {}: right click on the land to place it", who);
}

void CreatureSpawner::DrawSpawnMind() noexcept
{
	ImGui::SeparatorText("Mind");
	if (ImGui::Button("Open mind file..."))
	{
		ChooseMindFile(MindFileUse::Spawn);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Spawns creatures from a saved creature: its species, name, alignment, strength, size, tattoos "
		                  "and what it has learnt");
	}
	if (_spawnMind != nullptr)
	{
		ImGui::SameLine();
		if (ImGui::Button("Fresh mind"))
		{
			_spawnMind.reset();
			_spawnTattoos.reset();
			_spawnMindName.clear();
		}
		ImGui::TextWrapped("Spawning %s", _spawnMindName.c_str());
	}
	else
	{
		ImGui::TextUnformatted("Spawning with a fresh mind");
	}
	if (!_lastMindFile.empty())
	{
		ImGui::TextWrapped("%s", _lastMindFile.c_str());
	}
}
