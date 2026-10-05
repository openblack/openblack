/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Weather.h"

#include <cmath>

#include <array>
#include <numbers>
#include <string_view>

#include <fmt/format.h>

#include "Camera/Camera.h"
#include "ECS/Components/Weather.h"
#include "ECS/Registry.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::debug::gui;
using openblack::ecs::components::Storm;
using openblack::ecs::components::WeatherInfo;
using openblack::ecs::systems::ForcedStorm;

namespace
{

constexpr float k_DegreesToRadians = std::numbers::pi_v<float> / 180.0f;

struct Preset
{
	std::string_view name;
	std::string_view tooltip;
	ImVec4 colour;
	int8_t rain;
	int8_t snow;
	int8_t overcast;
	int8_t temperature;
	int windStrength;
	bool lightning;
};

constexpr std::array<Preset, 6> k_Presets {{
    {"Clear", "Every storm clears: calm air over the whole island", {0.95f, 0.75f, 0.25f, 1.0f}, 0, 0, 0, 20, 0, false},
    {"Drizzle", "A light rain under grey skies", {0.45f, 0.65f, 0.80f, 1.0f}, 30, 0, 60, 15, 5, false},
    {"Rain", "A steady downpour", {0.25f, 0.45f, 0.85f, 1.0f}, 80, 0, 90, 12, 15, false},
    {"Thunderstorm",
     "Heavy rain, wind, thunder and bolts of lightning",
     {0.55f, 0.35f, 0.85f, 1.0f},
     100,
     0,
     100,
     20,
     30,
     true},
    {"Snow", "Snow falling gently", {0.80f, 0.88f, 0.95f, 1.0f}, 0, 70, 80, -5, 5, false},
    {"Blizzard", "Thick snow on a gale", {0.65f, 0.80f, 0.95f, 1.0f}, 0, 100, 100, -15, 60, false},
}};

/// How often a lightning storm flashes, in seconds: with thunder, then with a bolt
constexpr glm::vec2 k_ThunderWait {3.0f, 15.0f};
constexpr glm::vec2 k_BoltWait {2.0f, 8.0f};

ImVec4 Darker(ImVec4 colour, float by)
{
	return {colour.x * by, colour.y * by, colour.z * by, colour.w};
}

/// A labelled bar for an amount of 0 to 100
void AmountBar(const char* label, int value, ImVec4 colour)
{
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, colour);
	ImGui::ProgressBar(static_cast<float>(value) / 100.0f, ImVec2(-80.0f, 0.0f), fmt::format("{}%", value).c_str());
	ImGui::PopStyleColor();
	ImGui::SameLine();
	ImGui::TextUnformatted(label);
}

/// A compass showing which way the wind blows and how hard
void WindCompass(float windX, float windZ, float radius)
{
	auto* drawList = ImGui::GetWindowDrawList();
	const auto corner = ImGui::GetCursorScreenPos();
	const ImVec2 centre {corner.x + radius, corner.y + radius};
	drawList->AddCircleFilled(centre, radius, IM_COL32(30, 40, 55, 255));
	drawList->AddCircle(centre, radius, IM_COL32(120, 140, 170, 255), 32, 1.5f);
	drawList->AddText(ImVec2(centre.x - 3.0f, corner.y + 1.0f), IM_COL32(160, 170, 190, 255), "N");
	const auto strength = std::sqrt((windX * windX) + (windZ * windZ));
	if (strength > 0.0f)
	{
		// Longer the harder it blows, at most to the rim at 100
		const auto length = std::min(strength / 100.0f, 1.0f) * (radius - 4.0f);
		const ImVec2 tip {centre.x + (windX / strength * length), centre.y - (windZ / strength * length)};
		drawList->AddLine(centre, tip, IM_COL32(120, 220, 255, 255), 2.5f);
		drawList->AddCircleFilled(tip, 3.5f, IM_COL32(120, 220, 255, 255));
	}
	ImGui::Dummy(ImVec2(radius * 2.0f, radius * 2.0f));
}

} // namespace

Weather::Weather() noexcept
    : Window("Weather", ImVec2(460.0f, 470.0f))
{
	_storm.effect.temperature = 12;
	_storm.effect.rain = 80;
	_storm.effect.overcast = 90;
}

void Weather::Draw() noexcept
{
	if (!Locator::weatherSystem::has_value())
	{
		ImGui::TextUnformatted("There is no weather on this island");
		return;
	}
	DrawAtCamera();
	ImGui::Separator();
	DrawPresets();
	ImGui::Separator();
	DrawCustom();
	ImGui::Separator();
	DrawActions();
	ImGui::Separator();
	DrawStorms();
}

void Weather::DrawAtCamera() noexcept
{
	if (!Locator::camera::has_value())
	{
		return;
	}
	auto& weather = Locator::weatherSystem::value();
	const auto camera = Locator::camera::value().GetOrigin();
	const auto here = weather.GetWeatherSmooth(camera);

	ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f), "At the camera");
	ImGui::BeginGroup();
	WindCompass(static_cast<float>(here.windX), static_cast<float>(here.windZ), 36.0f);
	ImGui::EndGroup();
	ImGui::SameLine();
	ImGui::BeginGroup();
	AmountBar("Rain", here.rain, ImVec4(0.30f, 0.55f, 0.95f, 1.0f));
	AmountBar("Snow", here.snow, ImVec4(0.85f, 0.90f, 0.98f, 1.0f));
	AmountBar("Cloud", here.overcast, ImVec4(0.55f, 0.58f, 0.62f, 1.0f));
	const auto flash = weather.GetLightningFlash(camera);
	AmountBar("Flash", (flash * 100) / 255, ImVec4(1.0f, 0.95f, 0.55f, 1.0f));
	ImGui::EndGroup();
	const auto temperature = static_cast<int>(here.temperature);
	const auto cold = temperature < 0;
	ImGui::TextColored(cold ? ImVec4(0.6f, 0.8f, 1.0f, 1.0f) : ImVec4(1.0f, 0.7f, 0.4f, 1.0f), "%d C", temperature);
	ImGui::SameLine();
	ImGui::TextDisabled("wind (%d, %d)  snow cover %d", here.windX, here.windZ, here.snowCover);
}

void Weather::DrawPresets() noexcept
{
	ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f), "Over the whole island");
	const auto width = (ImGui::GetContentRegionAvail().x - (ImGui::GetStyle().ItemSpacing.x * 2.0f)) / 3.0f;
	for (size_t i = 0; i < k_Presets.size(); ++i)
	{
		const auto& preset = k_Presets.at(i);
		if (i % 3 != 0)
		{
			ImGui::SameLine();
		}
		ImGui::PushStyleColor(ImGuiCol_Button, Darker(preset.colour, 0.55f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Darker(preset.colour, 0.75f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, preset.colour);
		const auto pressed = ImGui::Button(preset.name.data(), ImVec2(width, 32.0f));
		ImGui::PopStyleColor(3);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", preset.tooltip.data());
		}
		if (!pressed)
		{
			continue;
		}
		// A preset becomes the island's weather: the other storms clear and the climates stop breeding new ones, so
		// only a thunderstorm brings thunder
		auto& weather = Locator::weatherSystem::value();
		weather.ClearStorms();
		weather.SetStormCreationEnabled(false);
		if (preset.rain == 0 && preset.snow == 0)
		{
			continue;
		}
		// The made-to-measure storm takes the preset, keeping its own length, so it can be tweaked from there
		_storm.effect.rain = preset.rain;
		_storm.effect.snow = preset.snow;
		_storm.effect.overcast = preset.overcast;
		_storm.effect.temperature = preset.temperature;
		_windStrength = preset.windStrength;
		_lightning = preset.lightning;
		_storm.effect.windX = static_cast<int8_t>(std::lround(std::sin(_windDegrees * k_DegreesToRadians) * _windStrength));
		_storm.effect.windZ = static_cast<int8_t>(std::lround(std::cos(_windDegrees * k_DegreesToRadians) * _windStrength));
		_storm.thunderWait = _lightning ? k_ThunderWait : glm::vec2(0.0f);
		_storm.boltWait = _lightning ? k_BoltWait : glm::vec2(0.0f);
		weather.ForceStorm(_storm);
	}
}

void Weather::DrawCustom() noexcept
{
	if (!ImGui::CollapsingHeader("Made to measure"))
	{
		return;
	}
	const auto byteSlider = [](const char* label, int8_t& value, int min, int max, const char* format) {
		int wide = value;
		if (ImGui::SliderInt(label, &wide, min, max, format))
		{
			value = static_cast<int8_t>(wide);
		}
	};
	byteSlider("Rain", _storm.effect.rain, 0, 100, "%d%%");
	byteSlider("Snow", _storm.effect.snow, 0, 100, "%d%%");
	byteSlider("Cloud", _storm.effect.overcast, 0, 100, "%d%%");
	byteSlider("Temperature", _storm.effect.temperature, -30, 45, "%d C");
	ImGui::SliderFloat("Wind from", &_windDegrees, 0.0f, 360.0f, "%.0f deg");
	ImGui::SliderInt("Wind strength", &_windStrength, 0, 100);
	_storm.effect.windX = static_cast<int8_t>(std::lround(std::sin(_windDegrees * k_DegreesToRadians) * _windStrength));
	_storm.effect.windZ = static_cast<int8_t>(std::lround(std::cos(_windDegrees * k_DegreesToRadians) * _windStrength));

	ImGui::Checkbox("Lightning", &_lightning);
	if (_lightning)
	{
		if (_storm.thunderWait.y == 0.0f)
		{
			_storm.thunderWait = k_ThunderWait;
			_storm.boltWait = k_BoltWait;
		}
		ImGui::DragFloatRange2("Thunder every", &_storm.thunderWait.x, &_storm.thunderWait.y, 0.1f, 0.5f, 120.0f, "%.1f s",
		                       "%.1f s");
		ImGui::DragFloatRange2("Bolt every", &_storm.boltWait.x, &_storm.boltWait.y, 0.1f, 0.5f, 120.0f, "%.1f s", "%.1f s");
	}
	else
	{
		_storm.thunderWait = glm::vec2(0.0f);
		_storm.boltWait = glm::vec2(0.0f);
	}

	ImGui::SliderFloat("Lasts", &_storm.seconds, 10.0f, 3600.0f, "%.0f s", ImGuiSliderFlags_Logarithmic);
	ImGui::SliderFloat("Comes in over", &_storm.fadeSeconds, 0.1f, 60.0f, "%.1f s", ImGuiSliderFlags_Logarithmic);
	ImGui::SliderFloat("Cloud height", &_storm.cloudHeight, 0.0f, 500.0f, _storm.cloudHeight > 0.0f ? "%.0f" : "climate's");
	ImGui::SliderFloat("Rain speed", &_storm.rainSpeed, 0.0f, 5.0f, _storm.rainSpeed > 0.0f ? "%.2f" : "climate's");

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.30f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.60f, 0.38f, 1.0f));
	if (ImGui::Button("Force this storm", ImVec2(-1.0f, 28.0f)))
	{
		Locator::weatherSystem::value().ForceStorm(_storm);
	}
	ImGui::PopStyleColor(2);
}

void Weather::DrawActions() noexcept
{
	auto& weather = Locator::weatherSystem::value();
	if (ImGui::Button("Clear every storm"))
	{
		weather.ClearStorms();
	}
	ImGui::SameLine();
	if (ImGui::Button("Bolt now"))
	{
		weather.StrikeLightning(true);
	}
	ImGui::SameLine();
	if (ImGui::Button("Thunder now"))
	{
		weather.StrikeLightning(false);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Only storms with lightning flash");
	}

	auto climates = weather.IsClimateSystemEnabled();
	if (ImGui::Checkbox("Climates", &climates))
	{
		weather.SetClimateSystemEnabled(climates);
	}
	ImGui::SameLine();
	auto storms = weather.IsStormCreationEnabled();
	if (ImGui::Checkbox("Climates breed storms", &storms))
	{
		weather.SetStormCreationEnabled(storms);
	}
}

void Weather::DrawStorms() noexcept
{
	auto& registry = Locator::entitiesRegistry::value();
	size_t alive = 0;
	registry.Each<const Storm>([&alive](entt::entity, const Storm& storm) { alive += storm.dead ? 0 : 1; });
	if (!ImGui::CollapsingHeader(fmt::format("Storms ({})###Storms", alive).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}
	constexpr auto k_Flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp;
	if (!ImGui::BeginTable("storms", 6, k_Flags))
	{
		return;
	}
	ImGui::TableSetupColumn("Where");
	ImGui::TableSetupColumn("Radius");
	ImGui::TableSetupColumn("Life");
	ImGui::TableSetupColumn("Rain/snow/cloud");
	ImGui::TableSetupColumn("Lightning");
	ImGui::TableSetupColumn("");
	ImGui::TableHeadersRow();
	auto& weather = Locator::weatherSystem::value();
	registry.Each<const Storm>([&weather](entt::entity entity, const Storm& storm) {
		if (storm.dead)
		{
			return;
		}
		ImGui::PushID(static_cast<int>(entt::to_integral(entity)));
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::Text("%.0f, %.0f", storm.currentPosition.x, storm.currentPosition.z);
		ImGui::TableNextColumn();
		ImGui::Text("%.0f", storm.outerRadius);
		ImGui::TableNextColumn();
		ImGui::ProgressBar(storm.lastsFor > 0.0f ? storm.age / storm.lastsFor : 0.0f, ImVec2(-1.0f, 0.0f),
		                   fmt::format("{:.0f}/{:.0f}s", storm.age, storm.lastsFor).c_str());
		ImGui::TableNextColumn();
		ImGui::Text("%d/%d/%d", storm.effect.rain, storm.effect.snow, storm.effect.overcast);
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(storm.boltWait.y != 0.0f || storm.thunderWait.y != 0.0f ? "yes" : "-");
		ImGui::TableNextColumn();
		// A storm that is clearing has no more than its fading time left
		if (storm.lastsFor - storm.age < storm.fadeTime && storm.age >= storm.fadeTime)
		{
			ImGui::TextDisabled("clearing");
		}
		else if (ImGui::SmallButton("End"))
		{
			weather.EndStorm(entity);
		}
		ImGui::PopID();
	});
	ImGui::EndTable();
}

void Weather::Update() noexcept {}

void Weather::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) noexcept {}

void Weather::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) noexcept {}
