/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Audio.h"

#include <entt/entt.hpp>
#include <imgui.h>

#include "ECS/Registry.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::audio;
using namespace openblack::debug::gui;
using namespace openblack::ecs::components;

const ImVec4 k_RedColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
const ImVec4 k_GreenColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
const std::array<const char*, 3> k_AudioBankLoopStrings = {"Repeat", "Once", "Overlap"};

Audio::Audio()
    : Window("Audio Player", ImVec2(600.0f, 600.0f))
{
}

void Audio::Emitters()
{
	if (ImGui::Button("Play") && _selectedSound != entt::null)
	{
		Locator::audio::value().PlaySound(_selectedSound, _playType);
	}
	ImGui::SameLine();
	auto currentCombo = static_cast<int>(_playType);
	ImGui::Combo("PlayType", &currentCombo, k_AudioBankLoopStrings.data(), static_cast<int>(k_AudioBankLoopStrings.size()));
	_playType = static_cast<PlayType>(currentCombo);
	ImGui::Separator();
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
	ImGui::BeginChild("SoundPacks", ImVec2(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y), true);
	ImGui::Button("Sort by sound count");
	ImGui::SameLine();
	ImGui::Button("Sort by bytes");
	ImGui::SameLine();
	ImGui::Columns(3, "SoundPackColumns", true);
	ImGui::Separator();
	ImGui::Text("File");
	ImGui::NextColumn();
	ImGui::Text("Description");
	ImGui::NextColumn();
	ImGui::Text("Sounds");
	ImGui::NextColumn();
	ImGui::Separator();
	for (const auto& [name, group] : Locator::audio::value().GetSoundGroups())
	{
		if (ImGui::Selectable(name.c_str(), _selectedSoundPack == name, ImGuiSelectableFlags_SpanAllColumns))
		{
			_selectedSoundPack = name;
		}

		ImGui::NextColumn();
		ImGui::Text("%s", name.c_str());
		ImGui::NextColumn();
		ImGui::Text("%zu", group.sounds.size());
		ImGui::NextColumn();
	}
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("Sounds", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), true);
	ImGui::Separator();
	ImGui::Columns(2, "SoundColumns", true);
	ImGui::Text("Name / Play");
	ImGui::NextColumn();
	ImGui::Text("Length in seconds");
	ImGui::NextColumn();
	ImGui::Separator();
	auto& soundManager = Locator::audio::value();
	for (const auto& [name, group] : Locator::audio::value().GetSoundGroups())
	{
		if (_selectedSoundPack != name)
		{
			continue;
		}

		for (auto soundId : group.sounds)
		{
			auto sound = Locator::resources::value().GetSounds().Handle(soundId);
			if (ImGui::Selectable(("##" + std::to_string(soundId)).c_str(), _selectedSound == soundId,
			                      ImGuiSelectableFlags_SpanAllColumns))
			{
				// Play the sound if already selected
				if (_selectedSound == soundId)
				{
					soundManager.PlaySound(_selectedSound, _playType);
				}

				_selectedSound = soundId;
			}
			ImGui::SameLine();
			ImGui::Text("%s", sound->name.c_str());
			ImGui::NextColumn();
			auto length = sound->duration;
			ImGui::TextColored(length < 0 ? k_RedColor : k_GreenColor, "%s",
			                   length < 0 ? "N/A" : std::to_string(length).c_str());
			ImGui::NextColumn();
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
}

void Audio::Music()
{
	if (ImGui::Button("Play") && !_selectedMusicPack.empty())
	{
		Locator::audio::value().PlayMusic(_selectedMusicPack, _playType);
	}
	ImGui::SameLine();
	auto currentCombo = static_cast<int>(_playType);
	ImGui::Combo("PlayType", &currentCombo, k_AudioBankLoopStrings.data(), static_cast<int>(k_AudioBankLoopStrings.size()));
	_playType = static_cast<PlayType>(currentCombo);
	ImGui::Separator();
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
	ImGui::BeginChild("MusicPacks", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), true);
	ImGui::Columns(1, "MusicPackColumns", true);
	ImGui::Separator();
	ImGui::Text("Name");
	ImGui::NextColumn();
	ImGui::Separator();
	auto& soundManager = Locator::audio::value();
	for (const auto& name : Locator::audio::value().GetMusicTracks())
	{
		if (ImGui::Selectable(name.c_str(), _selectedMusicPack == name, ImGuiSelectableFlags_SpanAllColumns))
		{
			// Play the sound if it is already selected
			if (_selectedMusicPack == name)
			{
				soundManager.PlayMusic(_selectedMusicPack, _playType);
			}

			_selectedMusicPack = name;
		}

		ImGui::NextColumn();
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
}

void Audio::AudioSettings()
{
	auto& soundManager = Locator::audio::value();
	if (!soundManager.EmitterExists(_selectedEmitter))
	{
		_selectedEmitter = entt::null;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
	ImGui::BeginChild("Audio Handler Settings", ImVec2(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y),
	                  true);
	ImGui::Text("Audio handler settings");
	ImGui::Separator();
	ImGui::Text("Active Emitters");
	ImGui::Separator();
	ImGui::Columns(5, "PlayingEmitters", true);
	ImGui::Text("Emitter ID");
	ImGui::NextColumn();
	ImGui::Text("Sound Name");
	ImGui::NextColumn();
	ImGui::Text("Audio source ID");
	ImGui::NextColumn();
	ImGui::Text("3D?");
	ImGui::NextColumn();
	ImGui::Text("World Location");
	ImGui::NextColumn();
	ImGui::Separator();
	Locator::entitiesRegistry::value().Each<ecs::components::AudioEmitter>(
	    [this](entt::entity entity, const AudioEmitter& emitter) {
		    if (ImGui::Selectable(("##" + std::to_string(emitter.sourceId)).c_str(), _selectedEmitter == entity,
		                          ImGuiSelectableFlags_SpanAllColumns))
		    {
			    _selectedEmitter = entity;
		    }
		    ImGui::SameLine();
		    ImGui::Text("%u", emitter.sourceId);
		    ImGui::NextColumn();
		    ImGui::Text("%s", Locator::audio::value().GetSound(emitter.soundId).name.c_str());
		    ImGui::NextColumn();
		    ImGui::Text("%d", emitter.sourceId);
		    ImGui::NextColumn();
		    ImGui::Text("%s", emitter.relative ? "Yes" : "No");
		    ImGui::NextColumn();
		    if (emitter.relative)
		    {
			    ImGui::Text("(%.1f,%.1f, %.1f)", emitter.position.x, emitter.position.y, emitter.position.z);
		    }
		    else
		    {
			    ImGui::Text("N/A");
		    }
		    ImGui::NextColumn();
	    });
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("Audio Player Settings", ImGui::GetContentRegionAvail(), true);
	ImGui::Text("Audio Player settings");
	ImGui::Separator();
	float globalVolume = soundManager.GetGlobalVolume();
	float musicVolume = soundManager.GetMusicVolume();
	float sfxVolume = soundManager.GetSfxVolume();
	ImGui::SliderFloat("Global Volume", &globalVolume, 0.0f, 1.0f, "%.3f");
	ImGui::SliderFloat("Music Volume", &musicVolume, 0.0f, 1.0f, "%.3f");
	ImGui::SliderFloat("SFX Volume", &sfxVolume, 0.0f, 1.0f, "%.3f");
	ImGui::Separator();
	if (globalVolume != soundManager.GetGlobalVolume())
	{
		soundManager.SetGlobalVolume(globalVolume);
	}
	if (musicVolume != soundManager.GetMusicVolume())
	{
		soundManager.SetMusicVolume(musicVolume);
	}
	if (sfxVolume != soundManager.GetSfxVolume())
	{
		soundManager.SetSfxVolume(sfxVolume);
	}
	ImGui::Text("Active Sounds");
	ImGui::SameLine();
	if (ImGui::Button("Play") && _selectedEmitter != entt::null)
	{
		soundManager.PlayEmitter(_selectedEmitter);
	}
	ImGui::SameLine();
	if (ImGui::Button("Pause") && _selectedEmitter != entt::null)
	{
		soundManager.PauseEmitter(_selectedEmitter);
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop") && _selectedEmitter != entt::null)
	{
		soundManager.StopEmitter(_selectedEmitter);
	}
	ImGui::Separator();
	ImGui::Columns(5, "PlayingSounds", true);
	ImGui::Text("Audio Source ID");
	ImGui::NextColumn();
	ImGui::Text("Current Audio Buffer ID");
	ImGui::NextColumn();
	ImGui::Text("Sound Name");
	ImGui::NextColumn();
	ImGui::Text("Status");
	ImGui::NextColumn();
	ImGui::Text("Progress");
	ImGui::NextColumn();
	ImGui::Separator();
	Locator::entitiesRegistry::value().Each<ecs::components::AudioEmitter>(
	    [this](entt::entity entity, const AudioEmitter& emitter) {
		    if (ImGui::Selectable(("##" + std::to_string(emitter.sourceId)).c_str(), _selectedEmitter == entity,
		                          ImGuiSelectableFlags_SpanAllColumns))
		    {
			    _selectedEmitter = entity;
		    }
		    const auto& sound = Locator::audio::value().GetSound(emitter.soundId);
		    ImGui::SameLine();
		    ImGui::Text("%u", emitter.sourceId);
		    ImGui::NextColumn();
		    ImGui::Text("%u", sound.bufferId);
		    ImGui::NextColumn();
		    ImGui::Text("%s", sound.name.c_str());
		    ImGui::NextColumn();
		    switch (Locator::audio::value().GetStatus(entity))
		    {
		    case AudioStatus::Initial:
			    ImGui::TextColored(k_RedColor, "Initial");
			    break;
		    case AudioStatus::Playing:
			    ImGui::TextColored(k_GreenColor, "Playing");
			    break;
		    case AudioStatus::Paused:
			    ImGui::TextColored(k_RedColor, "Paused");
			    break;
		    case AudioStatus::Stopped:
			    ImGui::TextColored(k_RedColor, "Stopped");
			    break;
		    }
		    ImGui::NextColumn();
		    ImGui::ProgressBar(Locator::audio::value().GetProgress(entity));
		    ImGui::NextColumn();
	    });
	ImGui::EndChild();
	ImGui::PopStyleVar();
}
void Audio::Draw(Game&)
{
	const ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("Tabs", tabBarFlags))
	{
		if (ImGui::BeginTabItem("Sound"))
		{
			ImGui::Text("View sound packs and their contents");
			ImGui::Separator();
			Audio::Emitters();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Music"))
		{
			ImGui::Text("View music packs and their contents");
			ImGui::Separator();
			Audio::Music();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Emitters"))
		{
			ImGui::Text("Manage sound emitters");
			ImGui::Separator();
			Audio::AudioSettings();
			ImGui::EndTabItem();
		}
	}
	ImGui::EndTabBar();
	ImGui::Separator();
}

void Audio::Update(Game&, const Renderer&) {}

void Audio::ProcessEventOpen(const SDL_Event&) {}

void Audio::ProcessEventAlways(const SDL_Event&) {}
