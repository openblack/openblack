/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Magic.h"

#include <cstring>

#include <array>
#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include <fmt/format.h>

#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/MagicTables.h"

using namespace openblack;
using namespace openblack::debug::gui;
using namespace openblack::magic;

namespace
{
constexpr float k_SecondsPerTurn = std::chrono::duration<float>(k_ChantTurnDuration).count();
constexpr int k_MaxDancers = 200;

constexpr std::array<std::string_view, static_cast<size_t>(MagicInfoSection::_COUNT)> k_SectionNames {
    "General", "Heal", "Teleport", "Forest",       "Food",         "Storm and tornado",
    "Shield",  "Wood", "Water",    "Flying flock", "Ground flock", "Creature spell",
};

/// A worship site paying for a miracle cast from it
class WorshipCaster final: public SpellCasterInterface
{
public:
	explicit WorshipCaster(WorshipBattery& site)
	    : _site(site)
	{
	}

	float MaintainSpell(float amount) override { return MaintainSpellFromWorship(_site, amount); }

private:
	WorshipBattery& _site;
};

std::string_view NameOf(const std::array<char, 0x30>& name)
{
	return {name.data(), strnlen(name.data(), name.size())};
}

std::string MagicName(const InfoConstants& info, MagicType type)
{
	const auto name = NameOf(GetMagicEffectInfo(info, type).debugString);
	return name.empty() ? fmt::format("Magic type {}", static_cast<uint32_t>(type)) : std::string(name);
}

std::string SeedName(const InfoConstants& info, std::optional<SpellSeedType> seed)
{
	if (!seed)
	{
		return "none";
	}
	return std::string(NameOf(GetSpellSeedInfo(info, *seed).debugString));
}

std::string Seconds(float seconds)
{
	return seconds < 0.0f ? std::string("no limit") : fmt::format("{:.1f} s", seconds);
}

void Row(std::string_view label, const std::string& value)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(label.data(), label.data() + label.size());
	ImGui::TableNextColumn();
	ImGui::TextUnformatted(value.c_str());
}

/// A labelled bar for an amount out of a maximum
void AmountBar(const char* label, float value, float maximum)
{
	const float fraction = maximum > 0.0f ? value / maximum : 0.0f;
	ImGui::ProgressBar(fraction, ImVec2(-160.0f, 0.0f), fmt::format("{:.0f} / {:.0f}", value, maximum).c_str());
	ImGui::SameLine();
	ImGui::TextUnformatted(label);
}
} // namespace

Magic::Magic() noexcept
    : Window("Magic", ImVec2(560.0f, 640.0f))
{
}

void Magic::Draw() noexcept
{
	if (!Locator::infoConstants::has_value())
	{
		ImGui::TextUnformatted("No info.dat loaded");
		return;
	}
	if (ImGui::BeginTabBar("MagicTabs"))
	{
		if (ImGui::BeginTabItem("Running"))
		{
			DrawMiracles();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Miracle"))
		{
			DrawSelected();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("All miracles"))
		{
			DrawTable();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Prayer power"))
		{
			DrawSandbox();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Particles"))
		{
			DrawParticles();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

void Magic::DrawSelected() noexcept
{
	const auto& info = Locator::infoConstants::value();
	if (ImGui::BeginCombo("Magic type", MagicName(info, _selected).c_str()))
	{
		for (size_t i = 0; i < k_MagicTypeCount; ++i)
		{
			const auto type = static_cast<MagicType>(i);
			if (ImGui::Selectable(MagicName(info, type).c_str(), type == _selected))
			{
				_selected = type;
			}
		}
		ImGui::EndCombo();
	}

	const auto& magic = GetMagicInfo(info, _selected);
	const auto& effect = GetMagicEffectInfo(info, _selected);
	const auto slot = SlotOf(_selected);
	const auto seed = FindFirstSpellSeedForMagicType(info, _selected);
	const auto powerUp = GetPowerUpGestureForMagicType(info, _selected);

	SpellChants fresh;
	SetChants(fresh, effect.initialChants);
	const auto rules = ChantRulesFor(_selected, magic, effect);

	if (ImGui::BeginTable("MagicInfo", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		Row("Section", fmt::format("{} #{}", k_SectionNames.at(static_cast<size_t>(slot.section)), slot.index));
		Row("Cost to create", fmt::format("{:.0f}", effect.costToCreate));
		Row("Prayer power when cast", fmt::format("{:.0f}", effect.initialChants));
		Row("Upkeep per turn", fmt::format("{:.1f}", effect.costPerGameTurn));
		Row("Cost per event", fmt::format("{:.1f}", effect.costPerEvent));
		Row("Cost per shield impact", fmt::format("{:.1f}", effect.costPerShieldCollide));
		Row("Safety level when cast", fmt::format("{:.0f}", GetChantSafetyLevel(fresh, rules)));
		Row("Maintained", IsMaintainedSpell(_selected) ? "yes" : "no");
		Row("Recharged by its caster", magic.isSpellRecharged != 0 ? "yes" : "no");
		Row("Cheaper with tribal power", effect.divideCostsByTribalPower == 1 ? "yes" : "no");
		Row("Timer, one shot", Seconds(effect.timerWhenOneShot));
		Row("Timer, player", Seconds(effect.timerWhenPlayerCasting));
		Row("Timer, creature", Seconds(effect.timerWhenCreatureCasting));
		Row("Timer, computer player", Seconds(effect.timerWhenComputerPlayerCasting));
		Row("Creature casts from above", IsCreatureCastFromAbove(info, _selected) ? "yes" : "no");
		Row("Aggressive range", fmt::format("{:.0f} to {:.0f}", effect.agressiveRangeMin, effect.agressiveRangeMax));
		Row("Perceived power", fmt::format("{:.2f}", magic.perceivedPower));
		Row("First seed", SeedName(info, seed));
		Row("Power-up level", powerUp.level == k_BasePowerUpLevel ? std::string("base") : fmt::format("{}", powerUp.level));
		Row("Power-up gesture", fmt::format("{}", static_cast<uint32_t>(powerUp.gesture)));
		if (const auto* creatureSpell = GetMagicInfoAs<GMagicCreatureSpellInfo>(info, _selected))
		{
			Row("Total duration", Seconds(creatureSpell->totalDuration));
		}
		if (const auto* radius = GetMagicInfoAs<GMagicRadiusSpellInfo>(info, _selected))
		{
			Row("Radius", fmt::format("{:.0f} to {:.0f} (normal cost at {:.0f})", radius->minRadius, radius->maxRadius,
			                          radius->radiusForNormalCost));
		}
		ImGui::EndTable();
	}
}

void Magic::DrawTable() noexcept
{
	const auto& info = Locator::infoConstants::value();
	constexpr auto k_Flags =
	    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit;
	if (!ImGui::BeginTable("AllMagic", 7, k_Flags))
	{
		return;
	}
	ImGui::TableSetupScrollFreeze(1, 1);
	ImGui::TableSetupColumn("Magic type");
	ImGui::TableSetupColumn("Create");
	ImGui::TableSetupColumn("When cast");
	ImGui::TableSetupColumn("Per turn");
	ImGui::TableSetupColumn("Per event");
	ImGui::TableSetupColumn("Player");
	ImGui::TableSetupColumn("Creature");
	ImGui::TableHeadersRow();
	for (size_t i = 0; i < k_MagicTypeCount; ++i)
	{
		const auto type = static_cast<MagicType>(i);
		const auto& effect = GetMagicEffectInfo(info, type);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		if (ImGui::Selectable(MagicName(info, type).c_str(), type == _selected, ImGuiSelectableFlags_SpanAllColumns))
		{
			_selected = type;
		}
		ImGui::TableNextColumn();
		ImGui::Text("%.0f", static_cast<double>(effect.costToCreate));
		ImGui::TableNextColumn();
		ImGui::Text("%.0f", static_cast<double>(effect.initialChants));
		ImGui::TableNextColumn();
		ImGui::Text("%.1f", static_cast<double>(effect.costPerGameTurn));
		ImGui::TableNextColumn();
		ImGui::Text("%.1f", static_cast<double>(effect.costPerEvent));
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(Seconds(effect.timerWhenPlayerCasting).c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(Seconds(effect.timerWhenCreatureCasting).c_str());
	}
	ImGui::EndTable();
}

void Magic::DrawSandbox() noexcept
{
	const auto& info = Locator::infoConstants::value();
	ImGui::TextWrapped("A worship site with dancers feeding the miracle chosen on the Miracle tab. Each step is one game "
	                   "turn: the miracle pays its upkeep and is recharged from the site, then the site ends its turn.");

	auto tribe = static_cast<int>(_tribe);
	if (ImGui::Combo(
	        "Tribe", &tribe,
	        [](void*, int index, const char** text) {
		        *text = k_TribeStrs.at(static_cast<size_t>(index)).data();
		        return true;
	        },
	        nullptr, static_cast<int>(k_TribeCount)))
	{
		_tribe = static_cast<Tribe>(tribe);
	}
	ImGui::SliderInt("Dancers", &_dancers, 0, k_MaxDancers);
	ImGui::SliderFloat("Tribal power", &_tribalPower, 0.5f, 4.0f, "%.2f");
	ImGui::Checkbox("Infinite prayer power", &_site.infinite);
	ImGui::SameLine();
	ImGui::Checkbox("Free upkeep", &_site.freeMaintenance);

	if (ImGui::Button(fmt::format("Cast {}", MagicName(info, _selected)).c_str()))
	{
		CastInSandbox();
	}
	ImGui::SameLine();
	if (ImGui::Button("Step turn"))
	{
		StepSandbox();
	}
	ImGui::SameLine();
	ImGui::Checkbox("Run", &_running);
	ImGui::SameLine();
	if (ImGui::Button("Reset"))
	{
		ResetSandbox();
	}

	const auto& siteInfo = info.worshipSite.at(static_cast<size_t>(_tribe));
	const auto rules = WorshipBatteryRulesFor(siteInfo, _tribalPower);
	const auto dancers = static_cast<uint32_t>(_dancers);

	ImGui::SeparatorText(fmt::format("Worship site, turn {}", _turns).c_str());
	AmountBar("Battery", _site.battery, WorshipMaxBattery(rules, dancers));
	ImGui::Text("Chanted each turn at full speed: %.1f", static_cast<double>(WorshipCapacity(rules, dancers)));
	ImGui::Text("Available this turn: %.1f", static_cast<double>(WorshipAvailable(_site)));
	ImGui::Text("Dance intensity: %.2f   Strain: %.2f   Chants per dancer: %.2f", static_cast<double>(_site.danceIntensity),
	            static_cast<double>(_site.strain), static_cast<double>(_site.chantsPerDancer));

	ImGui::SeparatorText("Miracle");
	if (_spellType == MagicType::None)
	{
		ImGui::TextUnformatted("Nothing cast");
		return;
	}
	const auto spellRules = ChantRulesFor(_spellType, GetMagicInfo(info, _spellType), GetMagicEffectInfo(info, _spellType));
	ImGui::Text("%s%s", MagicName(info, _spellType).c_str(), _spellRunning ? "" : " (ended)");
	AmountBar("Prayer power", _spell.chants, _spell.initialChants);
	ImGui::Text("Safety level: %.0f", static_cast<double>(GetChantSafetyLevel(_spell, spellRules)));
	ImGui::Text("Strength: %.2f   Age: %.1f s of %s", static_cast<double>(_spellStrength), static_cast<double>(_spellAge),
	            Seconds(GetTimerWhenPlayerCasting(info, _spellType)).c_str());
}

void Magic::ResetSandbox() noexcept
{
	_site = {.infinite = _site.infinite, .freeMaintenance = _site.freeMaintenance};
	_spell = {};
	_spellType = MagicType::None;
	_spellRunning = false;
	_spellAge = 0.0f;
	_spellStrength = 0.0f;
	_turns = 0;
}

void Magic::CastInSandbox() noexcept
{
	const auto& info = Locator::infoConstants::value();
	_spellType = _selected;
	_spell = {};
	SetChants(_spell, GetMagicEffectInfo(info, _selected).initialChants);
	_spellRunning = true;
	_spellAge = 0.0f;
	_spellStrength = 0.0f;
}

void Magic::StepSandbox() noexcept
{
	const auto& info = Locator::infoConstants::value();
	const auto rules = WorshipBatteryRulesFor(info.worshipSite.at(static_cast<size_t>(_tribe)), _tribalPower);
	const auto dancers = static_cast<uint32_t>(_dancers);

	if (_spellRunning)
	{
		auto spellRules = ChantRulesFor(_spellType, GetMagicInfo(info, _spellType), GetMagicEffectInfo(info, _spellType));
		spellRules.tribalPower = _tribalPower;
		WorshipCaster caster(_site);
		_spellAge += k_SecondsPerTurn;
		const float duration = GetTimerWhenPlayerCasting(info, _spellType);
		if (duration >= 0.0f && _spellAge > duration)
		{
			_spellRunning = false;
		}
		else
		{
			_spellStrength = GetSpellStrength(_spell, spellRules, &caster);
			PayForOneTurn(_spell, spellRules, &caster);
			Recharge(_spell, spellRules, caster);
			_spellRunning = _spellStrength > 0.0f;
		}
	}

	UpdateWorshipStrain(_site, rules, dancers);
	EndWorshipTurn(_site, rules, dancers);
	++_turns;
}

void Magic::UpdateAlways() noexcept
{
	UpdateTimedEffects(ImGui::GetIO().DeltaTime);
}

void Magic::Update() noexcept
{
	if (!_running || !Locator::infoConstants::has_value())
	{
		return;
	}
	_sinceTurn += ImGui::GetIO().DeltaTime;
	while (_sinceTurn >= k_SecondsPerTurn)
	{
		_sinceTurn -= k_SecondsPerTurn;
		StepSandbox();
	}
}

void Magic::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) noexcept {}

void Magic::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) noexcept {}
