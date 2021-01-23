/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "AudioDebug.h"
#include "SoundHandler.h"
#include <imgui.h>

using namespace openblack;
using namespace openblack::audio;

PlayType AudioDebug::_playType = PlayType::Once;
SoundId AudioDebug::_selectedSound = -1;
std::string AudioDebug::_selectedPack = "";
AudioEmitterId AudioDebug::_selectedEmitter = InitialAudioEmitterId;
const ImVec4 redColor = ImVec4(1.f, .0f, .0f, 1.f);
const ImVec4 greenColor = ImVec4(0.f, 1.f, .0f, 1.f);

void AudioDebug::AudioPlayer(Game& game, const std::vector<std::shared_ptr<SoundPack>>& soundPacks)
{
	auto& handler = game.GetSoundHandler();
	if (ImGui::Button("Play") && _selectedSound != -1)
		handler.PlaySound(_selectedSound, _playType);
	ImGui::SameLine();
	const char* items[] = { "Repeat", "Once", "Overlap" };
	auto currentCombo = static_cast<int>(_playType);
	ImGui::Combo("PlayType", &currentCombo, items, IM_ARRAYSIZE(items));
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
	for (auto& soundPack : soundPacks)
	{
		auto& name = soundPack->GetFileName();
		if (ImGui::Selectable(name.c_str(), _selectedPack == name, ImGuiSelectableFlags_SpanAllColumns))
			_selectedPack = name;

		ImGui::NextColumn();
		ImGui::Text("%s", soundPack->GetName().c_str());
		ImGui::NextColumn();
		ImGui::Text("%zu", soundPack->GetSounds().size());
		ImGui::NextColumn();
	}
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("Sounds", ImVec2(ImGui::GetContentRegionAvail().x - 50, ImGui::GetContentRegionAvail().y), true);
	ImGui::Separator();
	ImGui::Columns(5, "SoundColumns", true);
	ImGui::Text("Name / Play");
	ImGui::NextColumn();
	ImGui::Text("Loaded");
	ImGui::NextColumn();
	ImGui::Text("Length in seconds");
	ImGui::NextColumn();
	ImGui::Text("Sectors merged");
	ImGui::NextColumn();
	ImGui::Text("Bytes");
	ImGui::NextColumn();
	ImGui::Separator();
	for (auto& soundPack : soundPacks)
	{
		if (_selectedPack != soundPack->GetFileName())
			continue;

		auto& sounds = soundPack->GetSounds();

		for (auto& [soundId, sound] : sounds)
		{
			if (ImGui::Selectable(("##" + std::to_string(sound->id)).c_str(), _selectedSound == soundId, ImGuiSelectableFlags_SpanAllColumns))
				_selectedSound = sound->id;
			ImGui::SameLine();
			ImGui::Text("%s", sound->name.c_str());
			ImGui::NextColumn();
			ImGui::TextColored(sound->loaded ? greenColor : redColor, sound->loaded ? "Yes" : "No");
			ImGui::NextColumn();
			auto length = sound->lengthInSeconds;
			ImGui::TextColored(length < 0 ? redColor : greenColor, "%s", length < 0 ? "N/A" : std::to_string(length).c_str());
			ImGui::NextColumn();
			ImGui::Text("%d", sound->sectorCount);
			ImGui::NextColumn();
			ImGui::Text("%zu", sound->bytes.size());
			ImGui::NextColumn();
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
}

void AudioDebug::AudioSettings(Game& game)
{
	auto& handler = game.GetSoundHandler();
	const auto& emitters = handler.GetEmitters();

	if (!handler.EmitterExists(_selectedEmitter))
		_selectedEmitter = InitialAudioEmitterId;

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
	ImGui::BeginChild("Audio Handler Settings", ImVec2(500, ImGui::GetContentRegionAvail().y), true);
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
	for (auto& [emitterId, emitter] : emitters)
	{
		if (ImGui::Selectable(("##" + std::to_string(emitterId)).c_str(), _selectedEmitter == emitterId,
		                      ImGuiSelectableFlags_SpanAllColumns))
			_selectedEmitter = emitterId;
		ImGui::SameLine();
		ImGui::Text("%llu", emitterId);
		ImGui::NextColumn();
		ImGui::Text("%s", handler.GetSound(emitter.soundId)->name.c_str());
		ImGui::NextColumn();
		ImGui::Text("%d", emitter.audioSourceId);
		ImGui::NextColumn();
		ImGui::Text("%s", emitter.world ? "Yes" : "No");
		ImGui::NextColumn();
		if (emitter.world)
			ImGui::Text("(%.1f,%.1f, %.1f)", emitter.position.x, emitter.position.y, emitter.position.z);
		else
			ImGui::Text("N/A");
		ImGui::NextColumn();
	}
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("Audio Decoder Settings", ImVec2(500, ImGui::GetContentRegionAvail().y), true);
	ImGui::Separator();
	ImGui::Text("Audio Decoder settings");
	if (ImGui::Button(("Re-initialize Audio Decoder (" + handler.GetAudioDecoderName() + ")").c_str()))
		handler.ReplaceAudioLoader(std::make_unique<FfmpegDecoder>(FfmpegDecoder()));
	ImGui::Separator();
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("Audio Player Settings", ImGui::GetContentRegionAvail(), true);
	ImGui::Text("Audio Player settings");
	if (ImGui::Button(("Re-initialize Audio Player (" + handler.GetAudioPlayerName() + ")").c_str()))
		handler.ReplaceAudioPlayer(std::make_unique<OpenAlPlayer>());
	ImGui::Separator();
	float volume = handler.GetGlobalVolume();
	ImGui::SliderFloat("Global Volume", &volume, 0.0f, 1.0f, "%.3f");
	ImGui::SliderFloat("Music Volume", &volume, 0.0f, 1.0f, "%.3f");
	ImGui::SliderFloat("SFX Volume", &volume, 0.0f, 1.0f, "%.3f");
	ImGui::Separator();
	if (volume != handler.GetGlobalVolume())
		handler.SetGlobalVolume(volume);
	ImGui::Text("Active Sounds");
	ImGui::SameLine();
	if (ImGui::Button("Play") && _selectedEmitter != InitialAudioEmitterId)
		handler.PlayEmitter(_selectedEmitter);
	ImGui::SameLine();
	if (ImGui::Button("Pause") && _selectedEmitter != InitialAudioEmitterId)
		handler.PauseEmitter(_selectedEmitter);
	ImGui::SameLine();
	if (ImGui::Button("Stop") && _selectedEmitter != InitialAudioEmitterId)
		handler.StopEmitter(_selectedEmitter);
	ImGui::SameLine();
	if (ImGui::Button("Export to .wav") && _selectedEmitter != InitialAudioEmitterId)
		AudioDecoder::DebugEmitWavFile(*handler.GetSound(handler.GetEmitters().at(_selectedEmitter).soundId));
	ImGui::Separator();
	ImGui::Columns(6, "PlayingSounds", true);
	ImGui::Text("Audio Source ID");
	ImGui::NextColumn();
	ImGui::Text("Audio Buffer ID");
	ImGui::NextColumn();
	ImGui::Text("Sound Name");
	ImGui::NextColumn();
	ImGui::Text("Buffer size");
	ImGui::NextColumn();
	ImGui::Text("Status");
	ImGui::NextColumn();
	ImGui::Text("Progress");
	ImGui::NextColumn();
	ImGui::Separator();
	for (auto [emitterId, emitter] : emitters)
	{
		if (ImGui::Selectable(("##" + std::to_string(emitterId)).c_str(), _selectedEmitter == emitterId,
		                      ImGuiSelectableFlags_SpanAllColumns))
			_selectedEmitter = emitterId;
		ImGui::SameLine();
		ImGui::Text("%u", emitter.audioSourceId);
		ImGui::NextColumn();
		ImGui::Text("%u", emitter.audioBufferId);
		ImGui::NextColumn();
		ImGui::Text("%s", handler.GetSound(emitter.soundId)->name.c_str());
		ImGui::NextColumn();
		ImGui::Text("%llu", handler.GetSound(emitter.soundId)->bytes.size());
		ImGui::NextColumn();
		switch (handler.GetPlayer()->GetAudioStatus(emitter.audioSourceId))
		{
		case AudioStatus::Initial:
			ImGui::TextColored(redColor, "Initial");
			break;
		case AudioStatus::Playing:
			ImGui::TextColored(greenColor, "Playing");
			break;
		case AudioStatus::Paused:
			ImGui::TextColored(redColor, "Paused");
			break;
		case AudioStatus::Stopped:
			ImGui::TextColored(redColor, "Stopped");
			break;
		}
		ImGui::NextColumn();
		ImGui::ProgressBar(handler.GetPlayer()->GetAudioProgress(emitter));
		ImGui::NextColumn();
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
}

void AudioDebug::ShowDebugGui(Game& game)
{
	auto& handler = game.GetSoundHandler();
	auto& soundPacks = handler.GetSoundPacks();

	if (ImGui::Begin("Audio Manager", &game.GetConfig().showAudioDebugger))
	{
		ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("Tabs", tabBarFlags))
		{
			auto musicPacks = std::vector<std::shared_ptr<SoundPack>>();
			auto generalPacks = std::vector<std::shared_ptr<SoundPack>>();

			for (auto& [soundPackName, soundPack] : soundPacks)
			{
				if (soundPack->IsMusic())
					musicPacks.push_back(soundPack);
				else
					generalPacks.push_back(soundPack);
			}

			if (ImGui::BeginTabItem("Audio Player"))
			{
				ImGui::Text("View sound packs and their contents");
				ImGui::Separator();
				AudioDebug::AudioPlayer(game, generalPacks);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Music Player"))
			{
				ImGui::Text("Manage game music");
				ImGui::Separator();
				AudioDebug::AudioPlayer(game, musicPacks);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Audio Settings"))
			{
				ImGui::Text("Manage audio settings");
				ImGui::Separator();
				AudioDebug::AudioSettings(game);
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
		ImGui::Separator();
	}
	ImGui::End();
}
