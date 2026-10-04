/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Audio.h"

#include <string>

#include <imgui.h>

#include "Audio/AtmosAudio.h"
#include "Audio/AtmosPlayer.h"
#include "Audio/GameMusic.h"
#include "Audio/MusicPlayer.h"
#include "Camera/Camera.h"
#include "ECS/Components/Weather.h"
#include "ECS/Registry.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "Game.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::audio;
using namespace openblack::debug::gui;
using namespace openblack::ecs::components;

const ImVec4 k_RedColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
const ImVec4 k_GreenColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
const std::array<const char*, 3> k_AudioBankLoopStrings = {"Repeat", "Once", "Overlap"};

Audio::Audio() noexcept
    : Window("Audio Player", ImVec2(600.0f, 600.0f))
    , _selectedSound(entt::null)
    , _selectedEmitter(entt::null)
{
}

void Audio::Emitters() noexcept
{
	using namespace std::literals;
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
	ImGui::BeginChild("SoundPacks", ImVec2(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y),
	                  ImGuiChildFlags_Borders);
	ImGui::Button("Sort by sound count");
	ImGui::SameLine();
	ImGui::Button("Sort by bytes");
	ImGui::Separator();
	if (ImGui::BeginTable("SoundPackTable", 3,
	                      ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV |
	                          ImGuiTableFlags_SizingFixedFit))
	{
		ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthFixed, 150.0f);
		ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Sounds", ImGuiTableColumnFlags_WidthFixed, 60.0f);
		ImGui::TableHeadersRow();

		for (const auto& [name, group] : Locator::audio::value().GetSoundGroups())
		{
			ImGui::TableNextRow();

			// Column 0: file (selectable)
			ImGui::TableSetColumnIndex(0);
			if (ImGui::Selectable(name.c_str(), _selectedSoundPack == name, ImGuiSelectableFlags_SpanAllColumns))
			{
				_selectedSoundPack = name;
			}

			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(name.c_str());
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%zu", group.sounds.size());
		}
		ImGui::EndTable();
	}
	ImGui::EndChild();
	ImGui::SameLine();

	ImGui::BeginChild("Sounds", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y),
	                  ImGuiChildFlags_Borders);

	ImGui::Separator();
	const float extraPadding = ImGui::GetStyle().ItemSpacing.x * 2;
	const float firstColumnWidth = ImGui::CalcTextSize("123").x + extraPadding;
	const std::string lastColumnString = "Length (s)";
	const float lastColumnWidth = ImGui::CalcTextSize(lastColumnString.c_str()).x + extraPadding;

	if (ImGui::BeginTable("SoundTable", 3,
	                      ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV |
	                          ImGuiTableFlags_SizingFixedFit))
	{
		ImGui::TableSetupColumn("id", ImGuiTableColumnFlags_WidthFixed, firstColumnWidth);
		ImGui::TableSetupColumn("Name / Play", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn(lastColumnString.c_str(), ImGuiTableColumnFlags_WidthFixed, lastColumnWidth);
		ImGui::TableHeadersRow();

		auto& audio = Locator::audio::value();

		for (const auto& [name, group] : audio.GetSoundGroups())
		{
			if (_selectedSoundPack != name)
			{
				continue;
			}

			for (auto soundId : group.sounds)
			{
				auto sound = Locator::resources::value().GetSounds().Handle(soundId);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				if (ImGui::Selectable(("##" + std::to_string(soundId)).c_str(), _selectedSound == soundId,
				                      ImGuiSelectableFlags_SpanAllColumns))
				{
					// Play the sound if already selected
					if (_selectedSound == soundId)
					{
						audio.PlaySound(_selectedSound, _playType);
					}

					_selectedSound = soundId;
				}
				ImGui::SameLine();
				ImGui::Text("%u", sound->id);

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%s", sound->name.c_str());
				ImGui::TableSetColumnIndex(2);
				auto length = sound->duration;
				ImGui::TextColored(length < 0 ? k_RedColor : k_GreenColor, "%s",
				                   length < 0 ? "N/A" : std::to_string(length).c_str());
			}
		}
		ImGui::EndTable();
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
}

void Audio::Music() noexcept
{
	// What GAudio has picked and what LHAudio's music channels are playing
	const auto* game = Game::Instance();
	if (const auto* gameMusic = game != nullptr ? game->GetGameMusic() : nullptr)
	{
		ImGui::Text("Playing: %s", std::string(audio::GetMusicTypeName(gameMusic->GetPlaying())).c_str());
		ImGui::Text("Land music: %s", std::string(audio::GetMusicTypeName(gameMusic->GetLandType())).c_str());
		ImGui::Text("Script music: %s", std::string(audio::GetMusicTypeName(gameMusic->GetScriptType())).c_str());
		ImGui::Text("Played out: %s, %u turns ago", std::string(audio::GetMusicTypeName(gameMusic->GetBlockedType())).c_str(),
		            gameMusic->GetBlockedTurns());
		ImGui::Text("Alignment music %s", gameMusic->IsAlignmentMusicEnabled() ? "enabled" : "disabled");
		for (const auto& [group, chunk] : gameMusic->GetResumeChunks())
		{
			ImGui::Text("Group %d resumes from chunk %u", group, chunk);
		}
	}
	if (const auto* music = Locator::audio::value().GetMusic())
	{
		if (ImGui::BeginTable("MusicChannels", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("Channel");
			ImGui::TableSetupColumn("Bank");
			ImGui::TableSetupColumn("Group");
			ImGui::TableSetupColumn("Chunk");
			ImGui::TableSetupColumn("Volume");
			ImGui::TableSetupColumn("Loops");
			ImGui::TableHeadersRow();
			const auto& channels = music->GetChannels();
			for (size_t i = 0; i < channels.size(); ++i)
			{
				const auto& channel = channels.at(i);
				if (!channel.active)
				{
					continue;
				}
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%zu%s", i, music->GetCurrentChannel() == static_cast<int>(i) ? " (current)" : "");
				ImGui::TableNextColumn();
				ImGui::Text("%s", channel.bank->path.c_str());
				ImGui::TableNextColumn();
				ImGui::Text("%d", channel.groupId);
				ImGui::TableNextColumn();
				ImGui::Text("%u / %u", channel.playingChunk, channel.bank->GetChunkCount());
				ImGui::TableNextColumn();
				ImGui::Text("%d -> %d", channel.volume, channel.targetVolume);
				ImGui::TableNextColumn();
				ImGui::Text("%d", channel.loops);
			}
			ImGui::EndTable();
		}
	}
	ImGui::Separator();

	if (ImGui::Button("Play") && !_selectedMusicPack.empty())
	{
		Locator::audio::value().PlayMusic(_selectedMusicPack, _playType);
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop"))
	{
		Locator::audio::value().StopMusic();
	}
	ImGui::SameLine();
	auto currentCombo = static_cast<int>(_playType);
	ImGui::Combo("PlayType", &currentCombo, k_AudioBankLoopStrings.data(), static_cast<int>(k_AudioBankLoopStrings.size()));
	_playType = static_cast<PlayType>(currentCombo);
	ImGui::Separator();
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
	ImGui::BeginChild("MusicPacks", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y),
	                  ImGuiChildFlags_Borders);
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

void Audio::AudioSettings() noexcept
{
	auto& soundManager = Locator::audio::value();
	if (!soundManager.EmitterExists(_selectedEmitter))
	{
		_selectedEmitter = entt::null;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
	ImGui::BeginChild("Audio Handler Settings", ImVec2(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y),
	                  ImGuiChildFlags_Borders);
	ImGui::Text("Audio handler settings");
	ImGui::Separator();
	ImGui::Text("Active Emitters");
	ImGui::Separator();
	ImGui::Columns(7, "PlayingEmitters", true);
	ImGui::Text("Emitter ID");
	ImGui::NextColumn();
	ImGui::Text("Sound Name");
	ImGui::NextColumn();
	ImGui::Text("Audio source ID");
	ImGui::NextColumn();
	ImGui::Text("Audio");
	ImGui::NextColumn();
	ImGui::Text("World Location");
	ImGui::NextColumn();
	ImGui::Text("Gain");
	ImGui::NextColumn();
	ImGui::Text("Pitch");
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
		    const char* kind = emitter.spatial ? "3D" : "2D";
		    ImGui::Text("%s", emitter.music ? "Music" : kind);
		    ImGui::NextColumn();
		    if (emitter.spatial)
		    {
			    ImGui::Text("(%.1f, %.1f, %.1f)", emitter.position.x, emitter.position.y, emitter.position.z);
		    }
		    else
		    {
			    ImGui::Text("N/A");
		    }
		    ImGui::NextColumn();
		    ImGui::Text("%.3f", emitter.gain);
		    ImGui::NextColumn();
		    ImGui::Text("%u%%", emitter.pitchPercent);
		    ImGui::NextColumn();
	    });
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("Audio Player Settings", ImGui::GetContentRegionAvail(), ImGuiChildFlags_Borders);
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
namespace
{
std::string AtmosSampleName(const std::string& bankName, int32_t sampleId)
{
	auto id = fmt::format("{}/{}", bankName, sampleId);
	const entt::id_type hashed = entt::hashed_string(id.c_str());
	auto& sounds = Locator::resources::value().GetSounds();
	if (sounds.Contains(hashed))
	{
		id += fmt::format(" ({})", sounds.Handle(hashed)->name);
	}
	return id;
}

constexpr ImGuiTableFlags k_TableFlags =
    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;
} // namespace

void Audio::Atmos() noexcept
{
	const auto* game = Game::Instance();
	const auto* atmosAudio = game != nullptr ? game->GetAtmosAudio() : nullptr;
	const auto* atmos = Locator::audio::value().GetAtmos();
	if (atmosAudio == nullptr || atmos == nullptr)
	{
		ImGui::TextUnformatted("No ambience: no level loaded or no audio device");
		return;
	}

	const auto& soundMap = atmosAudio->GetSoundMap();
	const auto camera = Locator::camera::value().GetOrigin();
	ImGui::Text("Camera (%.0f, %.0f, %.0f)  above land %.1f  one-shot clock %u", camera.x, camera.y, camera.z,
	            soundMap.GetHeightAboveLand(), atmos->GetTick());
	const auto& sky = Locator::skySystem::value();
	ImGui::Text("Sky type %.2f (0 day, 2 night)  alignment %.2f, group %u (%s)",
	            audio::AtmosAudio::CalculateSkyType(sky.GetTime(), sky.GetDayNightTimes()), atmosAudio->GetAlignmentValue(),
	            atmosAudio->GetGroup(), atmosAudio->GetGroup() == 2 ? "evil" : "good");
	if (Locator::weatherSystem::has_value())
	{
		const auto weather = Locator::weatherSystem::value().GetWeatherSmooth(camera);
		ImGui::Text("Weather at camera: rain %d  snow %d  wind (%d, %d)  temperature %d  overcast %d", weather.rain,
		            weather.snow, weather.windX, weather.windZ, weather.temperature, weather.overcast);
	}

	if (ImGui::CollapsingHeader("Banks", ImGuiTreeNodeFlags_DefaultOpen) &&
	    ImGui::BeginTable("AtmosBanks", 6, k_TableFlags, ImVec2(0.0f, 280.0f)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Type");
		ImGui::TableSetupColumn("Cells");
		ImGui::TableSetupColumn("Nearest");
		ImGui::TableSetupColumn("Target");
		ImGui::TableSetupColumn("Volume");
		ImGui::TableSetupColumn("Bank (0-127)");
		ImGui::TableHeadersRow();
		const auto& scans = soundMap.GetScans();
		for (size_t i = 1; i < k_AtmosTypeCount; ++i)
		{
			const auto volume = atmosAudio->GetVolumes().at(i);
			const auto name = k_AtmosTypeInfos.at(i).name.substr(std::string_view("ATMOS_TYPE_").size());
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			if (volume > 0.0f)
			{
				ImGui::TextColored(k_GreenColor, "%.*s", static_cast<int>(name.size()), name.data());
			}
			else
			{
				ImGui::Text("%.*s", static_cast<int>(name.size()), name.data());
			}
			ImGui::TableNextColumn();
			ImGui::Text("%u", scans.at(i).count);
			ImGui::TableNextColumn();
			if (scans.at(i).count != 0)
			{
				ImGui::Text("%.0f", std::sqrt(scans.at(i).distanceSquared));
			}
			ImGui::TableNextColumn();
			ImGui::Text("%.3f", atmosAudio->GetTargets().at(i));
			ImGui::TableNextColumn();
			ImGui::ProgressBar(volume, ImVec2(-1.0f, 0.0f));
			ImGui::TableNextColumn();
			ImGui::Text("%d", static_cast<int>(volume * 127.0f));
		}
		ImGui::EndTable();
	}

	if (ImGui::CollapsingHeader("Playing voices", ImGuiTreeNodeFlags_DefaultOpen) &&
	    ImGui::BeginTable("AtmosVoices", 3, k_TableFlags, ImVec2(0.0f, 180.0f)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Sample");
		ImGui::TableSetupColumn("Kind");
		ImGui::TableSetupColumn("Volume (0-127)");
		ImGui::TableHeadersRow();
		for (const auto& voice : atmos->GetVoices())
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(AtmosSampleName(voice.bankName, voice.sampleId).c_str());
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(voice.loop ? "loop" : "one-shot");
			ImGui::TableNextColumn();
			ImGui::ProgressBar(static_cast<float>(voice.volume) / 127.0f, ImVec2(-1.0f, 0.0f),
			                   std::to_string(voice.volume).c_str());
		}
		ImGui::EndTable();
	}

	if (ImGui::CollapsingHeader("Loops") && ImGui::BeginTable("AtmosLoops", 5, k_TableFlags, ImVec2(0.0f, 180.0f)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Sample");
		ImGui::TableSetupColumn("Group");
		ImGui::TableSetupColumn("Fade");
		ImGui::TableSetupColumn("Target");
		ImGui::TableSetupColumn("Playing");
		ImGui::TableHeadersRow();
		for (const auto& loop : atmos->GetLoops())
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(AtmosSampleName(loop.bankName, loop.sampleId).c_str());
			ImGui::TableNextColumn();
			ImGui::Text("%u", loop.group);
			ImGui::TableNextColumn();
			ImGui::Text("%d", loop.current);
			ImGui::TableNextColumn();
			ImGui::Text("%u", loop.target);
			ImGui::TableNextColumn();
			ImGui::TextColored(loop.playing ? k_GreenColor : k_RedColor, "%s", loop.playing ? "yes" : "no");
		}
		ImGui::EndTable();
	}

	if (ImGui::CollapsingHeader("Upcoming one-shots") && ImGui::BeginTable("AtmosQueue", 3, k_TableFlags, ImVec2(0.0f, 180.0f)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Sample");
		ImGui::TableSetupColumn("Group");
		ImGui::TableSetupColumn("In turns");
		ImGui::TableHeadersRow();
		const auto tick = atmos->GetTick();
		for (const auto& shot : atmos->GetQueue())
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(AtmosSampleName(shot.bankName, shot.sampleId).c_str());
			ImGui::TableNextColumn();
			ImGui::Text("%u", shot.group);
			ImGui::TableNextColumn();
			ImGui::Text("%d", static_cast<int32_t>(shot.nextTime - tick));
		}
		ImGui::EndTable();
	}

	if (ImGui::CollapsingHeader("Climates and storms"))
	{
		auto& registry = Locator::entitiesRegistry::value();
		if (ImGui::BeginTable("Climates", 7, k_TableFlags, ImVec2(0.0f, 140.0f)))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Climate");
			ImGui::TableSetupColumn("Centre");
			ImGui::TableSetupColumn("Rain desire");
			ImGui::TableSetupColumn("Raining days");
			ImGui::TableSetupColumn("Temperature");
			ImGui::TableSetupColumn("Wind");
			ImGui::TableSetupColumn("Storms");
			ImGui::TableHeadersRow();
			registry.Each<const Climate>([](entt::entity, const Climate& climate) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%d%s (type %u)", climate.index, climate.global ? " global" : "", climate.info);
				ImGui::TableNextColumn();
				ImGui::Text("(%u, %u) r %.0f", climate.cellX * 10u, climate.cellZ * 10u, climate.outerRadius);
				ImGui::TableNextColumn();
				ImGui::ProgressBar(climate.rainDesire, ImVec2(-1.0f, 0.0f));
				ImGui::TableNextColumn();
				ImGui::Text("%d%s", climate.rainingDays, climate.raining ? " raining" : "");
				ImGui::TableNextColumn();
				ImGui::Text("%.1f -> %.1f", climate.temperature, climate.targetTemperature);
				ImGui::TableNextColumn();
				ImGui::Text("(%.0f, %.0f)", climate.windX, climate.windZ);
				ImGui::TableNextColumn();
				ImGui::Text("%zu / %u", climate.storms.size(), climate.maxStorms);
			});
			ImGui::EndTable();
		}
		if (ImGui::BeginTable("Storms", 6, k_TableFlags, ImVec2(0.0f, 140.0f)))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Storm");
			ImGui::TableSetupColumn("Distance");
			ImGui::TableSetupColumn("Radius");
			ImGui::TableSetupColumn("Life");
			ImGui::TableSetupColumn("Strength");
			ImGui::TableSetupColumn("Rain / snow / wind");
			ImGui::TableHeadersRow();
			registry.Each<const Storm>([&camera](entt::entity, const Storm& storm) {
				if (storm.dead)
				{
					return;
				}
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("(%.0f, %.0f)", storm.currentPosition.x, storm.currentPosition.z);
				ImGui::TableNextColumn();
				ImGui::Text("%.0f", glm::distance(glm::vec2(camera.x, camera.z),
				                                  glm::vec2(storm.currentPosition.x, storm.currentPosition.z)));
				ImGui::TableNextColumn();
				ImGui::Text("%.0f / %.0f", storm.currentInnerRadius, storm.outerRadius);
				ImGui::TableNextColumn();
				ImGui::Text("%.0fs / %.0fs", storm.age, storm.lastsFor);
				ImGui::TableNextColumn();
				ImGui::ProgressBar(storm.currentStrength, ImVec2(-1.0f, 0.0f));
				ImGui::TableNextColumn();
				ImGui::Text("%d / %d / (%d, %d)", storm.effect.rain, storm.effect.snow, storm.effect.windX, storm.effect.windZ);
			});
			ImGui::EndTable();
		}
	}
}

void Audio::Draw() noexcept
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
		if (ImGui::BeginTabItem("Atmos"))
		{
			ImGui::Text("View Ambient sounds");
			ImGui::Separator();
			Audio::Atmos();
			ImGui::EndTabItem();
		}
	}
	ImGui::EndTabBar();
	ImGui::Separator();
}

void Audio::Update() noexcept {}

void Audio::ProcessEventOpen(const SDL_Event&) noexcept {}

void Audio::ProcessEventAlways(const SDL_Event&) noexcept {}
