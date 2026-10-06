/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureSpawner.h"

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <SDL_events.h>
#include <fmt/format.h>
#include <glm/trigonometric.hpp>

#include "3D/CreatureBody.h"
#include "Camera/Camera.h"
#include "Creature/CreatureDesires.h"
#include "Creature/CreatureIdleMind.h"
#include "Creature/CreatureLayers.h"
#include "Creature/CreatureLook.h"
#include "Creature/CreatureMorph.h"
#include "Creature/CreatureSkin.h"
#include "ECS/Archetypes/CreatureArchetype.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureHairSystemInterface.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

using namespace openblack;
using namespace openblack::debug::gui;
using openblack::ecs::archetypes::CreatureArchetype;
using openblack::ecs::components::Creature;
using openblack::ecs::components::CreatureAnimation;
using openblack::ecs::components::CreatureMindState;
using openblack::ecs::components::CreatureMorph;
using openblack::ecs::components::Transform;

namespace
{
// Indexed by species, from the cow
constexpr std::array<std::string_view, static_cast<size_t>(CreatureType::_COUNT) - 1> k_SpeciesNames {
    "Cow",        "Tiger", "Leopard", "Wolf", "Lion",     "Horse", "Tortoise", "Zebra",     "Brown Bear",
    "Polar Bear", "Sheep", "Chimp",   "Ogre", "Mandrill", "Rhino", "Gorilla",  "Giant Ape",
};

constexpr std::array<std::string_view, static_cast<size_t>(PlayerNames::_COUNT)> k_OwnerNames {
    "Player One", "Player Two", "Player Three", "Player Four", "Player Five", "Player Six", "Player Seven", "Neutral",
};

const ImVec4 k_PlacingColour {0.85f, 0.30f, 0.25f, 1.0f};
const ImVec4 k_StartColour {0.25f, 0.60f, 0.30f, 1.0f};

std::string_view SpeciesName(CreatureType species)
{
	const auto index = static_cast<size_t>(species);
	return index >= 1 && index <= k_SpeciesNames.size() ? k_SpeciesNames.at(index - 1) : "Unknown";
}

const GCreatureInfo* SpeciesInfo(CreatureType species)
{
	if (!Locator::infoConstants::has_value())
	{
		return nullptr;
	}
	const auto& creatures = Locator::infoConstants::value().creature;
	const auto row = creature::InfoRow(species);
	return row < creatures.size() ? &creatures.at(row) : nullptr;
}

/// An animation's place in the creature spec and its name
std::string AnimationLabel(size_t animation)
{
	const auto name = creature_layers::animations::Name(animation);
	return name.empty() ? fmt::format("{}", animation) : fmt::format("{} {}", animation, name);
}

std::string_view LookKindName(creature_look::Interest kind)
{
	constexpr std::array<std::string_view, 8> k_Names {"dove",     "citadel", "creature", "animal",
	                                                   "villager", "abode",   "tree",     "fixed object"};
	return k_Names.at(static_cast<size_t>(kind));
}

bool IsRightButton(const SDL_Event& event)
{
	return (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == SDL_BUTTON_RIGHT;
}
} // namespace

CreatureSpawner::CreatureSpawner() noexcept
    : Window("Creature Spawner", ImVec2(420.0f, 520.0f))
{
}

void CreatureSpawner::Close() noexcept
{
	_placing = false;
	_placeAt.reset();
	Window::Close();
}

void CreatureSpawner::Draw() noexcept
{
	DrawSettings();
	ImGui::Separator();
	DrawPlacing();
	ImGui::Separator();
	DrawCreatures();
}

void CreatureSpawner::UseSpeciesDefaults() noexcept
{
	if (!Locator::infoConstants::has_value())
	{
		return;
	}
	const auto body = CreatureArchetype::StartBody(_species);
	_alignment = body.alignment;
	_fatness = body.fatness;
	_strength = body.strength;
	_scale = CreatureArchetype::StartScale(_species);
	_defaultsFor = _species;
}

void CreatureSpawner::DrawSettings() noexcept
{
	if (_defaultsFor != _species)
	{
		UseSpeciesDefaults();
	}

	ImGui::SeparatorText("Creature");

	if (ImGui::BeginCombo("Species", SpeciesName(_species).data()))
	{
		for (size_t i = 0; i < k_SpeciesNames.size(); ++i)
		{
			const auto species = static_cast<CreatureType>(i + 1);
			if (ImGui::Selectable(k_SpeciesNames.at(i).data(), species == _species))
			{
				_species = species;
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::BeginCombo("Owner", k_OwnerNames.at(static_cast<size_t>(_owner)).data()))
	{
		for (size_t i = 0; i < k_OwnerNames.size(); ++i)
		{
			const auto owner = static_cast<PlayerNames>(i);
			if (ImGui::Selectable(k_OwnerNames.at(i).data(), owner == _owner))
			{
				_owner = owner;
			}
		}
		ImGui::EndCombo();
	}

	ImGui::SeparatorText("Body");
	ImGui::SliderFloat("Alignment", &_alignment, -1.0f, 1.0f,
	                   _alignment < 0.0f ? "Evil %.2f" : (_alignment > 0.0f ? "Good %.2f" : "Neutral %.2f"));
	ImGui::SliderFloat("Fatness", &_fatness, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("Strength", &_strength, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("Size", &_scale, creature_morph::k_MinScale, creature_morph::k_MaxScale, "%.2f",
	                   ImGuiSliderFlags_Logarithmic);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Creatures grow by themselves up to size %.1f", static_cast<double>(creature_morph::k_MaxGrownScale));
	}
	if (ImGui::Button("Species defaults"))
	{
		UseSpeciesDefaults();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("As a new creature of the species is: neutral, with its size, fatness and strength");
	}

	const auto* info = SpeciesInfo(_species);
	const auto morph = creature_morph::FromAttributes(
	    _alignment, _fatness, _strength, info != nullptr ? info->strength : creature_morph::k_UnknownSpeciesStrength);
	ImGui::Text("Evil-good %+.2f, thin-fat %+.2f, weak-strong %+.2f", static_cast<double>(morph.evilGood),
	            static_cast<double>(morph.thinFat), static_cast<double>(morph.weakStrong));
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How far the body is pulled from its base mesh towards each axis' mesh, -1 to 1");
	}

	DrawSelected();

	ImGui::SeparatorText("Facing");
	ImGui::Checkbox("Random", &_randomFacing);
	if (!_randomFacing)
	{
		ImGui::SliderFloat("Degrees", &_facingDegrees, 0.0f, 360.0f, "%.0f");
	}
}

void CreatureSpawner::DrawSelected() noexcept
{
	if (!_selected.has_value() || !Locator::entitiesRegistry::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	auto* creature = registry.TryGet<Creature>(*_selected);
	auto* transform = registry.TryGet<Transform>(*_selected);
	if (creature == nullptr || transform == nullptr)
	{
		_selected.reset();
		return;
	}

	ImGui::SeparatorText("Selected creature");
	ImGui::Text("%s at %.0f, %.0f", SpeciesName(creature->species).data(), static_cast<double>(transform->position.x),
	            static_cast<double>(transform->position.z));
	ImGui::SliderFloat("Its alignment", &creature->alignment, -1.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("Its fatness", &creature->fatness, 0.0f, 1.0f, "%.2f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Its body follows its fatness by at most %.2f a game turn",
		                  static_cast<double>(creature_morph::k_MaxFatnessStep));
	}
	ImGui::SliderFloat("Its strength", &creature->strength, 0.0f, 1.0f, "%.2f");
	if (ImGui::SliderFloat("Its size", &creature->size, creature_morph::k_MinScale, creature_morph::k_MaxScale, "%.2f",
	                       ImGuiSliderFlags_Logarithmic))
	{
		creature->size = creature_morph::ClampScale(creature->size);
		transform->scale = glm::vec3(CreatureArchetype::DrawnScale(creature->species, creature->size));
		registry.SetDirty();
	}
	if (auto* morph = registry.TryGet<CreatureMorph>(*_selected); morph != nullptr)
	{
		ImGui::Text("Drawn: evil-good %+.2f, thin-fat %+.2f, weak-strong %+.2f", static_cast<double>(morph->drawn.evilGood),
		            static_cast<double>(morph->drawn.thinFat), static_cast<double>(morph->drawn.weakStrong));
		const auto skinWeight = creature_skin::BlendWeight(morph->drawn.evilGood);
		ImGui::Text("Skin %u of %u towards %s", static_cast<uint32_t>(skinWeight),
		            static_cast<uint32_t>(creature_skin::k_MaxWeight), morph->drawn.evilGood < 0.0f ? "evil" : "good");
		ImGui::Text("Fatness shown %.2f", static_cast<double>(morph->shownFatness));
		ImGui::SameLine();
		if (ImGui::SmallButton("Show now"))
		{
			morph->shownFatness = creature->fatness;
		}
	}
	if (ImGui::Button("Deselect"))
	{
		_selected.reset();
	}
	if (_selected.has_value())
	{
		DrawMind(*_selected);
	}
}

void CreatureSpawner::DrawMind(entt::entity entity) noexcept
{
	namespace animations = creature_layers::animations;
	auto& registry = Locator::entitiesRegistry::value();
	auto* mind = registry.TryGet<CreatureMindState>(entity);
	auto* animation = registry.TryGet<CreatureAnimation>(entity);
	if (mind == nullptr || animation == nullptr || !Locator::creatureMindSystem::has_value())
	{
		return;
	}
	auto& minds = Locator::creatureMindSystem::value();

	ImGui::SeparatorText("Mind");
	ImGui::Checkbox("Pause mind", &mind->paused);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("The mind leaves the body alone; the buttons below still work");
	}
	if (Locator::infoConstants::has_value())
	{
		const auto& phases = Locator::infoConstants::value().creatureDevelopmentPhaseEntry;
		auto phase = static_cast<int>(std::min<size_t>(mind->developmentPhase, phases.size() - 1));
		if (ImGui::SliderInt("Grown up", &phase, 0, static_cast<int>(phases.size()) - 1,
		                     phases.at(static_cast<size_t>(phase)).name.data()))
		{
			mind->developmentPhase = static_cast<uint32_t>(phase);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Each stage of growing up brings desires and takes some away");
		}
	}
	const auto& idle = mind->idle;
	ImGui::Text("%s, step %zu of %zu", creature_mind::Name(idle.activity).data(), std::min(idle.step + 1, idle.agenda.size()),
	            idle.agenda.size());
	if (idle.step < idle.agenda.size())
	{
		const auto& step = idle.agenda[idle.step];
		switch (step.kind)
		{
		case creature_mind::Step::Kind::Wait:
			ImGui::Text("Waiting %.1f of %.1f s", static_cast<double>(idle.stepSeconds), static_cast<double>(step.seconds));
			break;
		case creature_mind::Step::Kind::Action:
			ImGui::Text("Action %s%s", AnimationLabel(step.animation).c_str(), step.sleepyEyes ? ", sleepy eyes" : "");
			break;
		case creature_mind::Step::Kind::Sit:
			ImGui::Text("Sitting %.1f of %.1f s", static_cast<double>(idle.stepSeconds), static_cast<double>(step.seconds));
			break;
		}
	}
	ImGui::Text("Shows a desire again in %.0f s%s%s", static_cast<double>(idle.showDesireSeconds),
	            idle.shown.has_value() ? ", last " : "",
	            idle.shown.has_value() ? creature_desires::Name(*idle.shown).data() : "");

	ImGui::SeparatorText("Body");
	const auto& body = animation->body;
	ImGui::Text("Playing %s, %.0f ms%s", AnimationLabel(creature_layers::CurrentAnimation(body)).c_str(),
	            static_cast<double>(body.timeMs), body.mirrored ? ", mirrored" : "");
	ImGui::Text("Face %s%s%s", animation->face.current ? AnimationLabel(*animation->face.current).c_str() : "none",
	            animation->face.wanted != animation->face.current ? " -> " : "",
	            animation->face.wanted != animation->face.current
	                ? (animation->face.wanted ? AnimationLabel(*animation->face.wanted).c_str() : "none")
	                : "");
	ImGui::Text("Gesture %s", animation->gesture.animation ? AnimationLabel(*animation->gesture.animation).c_str() : "none");
	if (mind->look.id.has_value() && animation->lookAt.has_value())
	{
		ImGui::Text("Looks at a %s (entity %u) for %.1f s", LookKindName(mind->look.kind).data(), *mind->look.id,
		            static_cast<double>(mind->look.watchedTurns) / 10.0);
	}
	else
	{
		ImGui::Text("Looks %s", animation->lookAt.has_value() ? "ahead" : "nowhere in particular");
	}
	ImGui::Text("Head turned %+.0f degrees left, %+.0f up", static_cast<double>(glm::degrees(animation->yaw.angle)),
	            static_cast<double>(glm::degrees(animation->pitch.angle)));
	const auto pairs =
	    std::ranges::count_if(animation->mirror, [index = uint32_t {0}](uint32_t mirror) mutable { return mirror != index++; });
	ImGui::Text("%td bones have a mirror bone", pairs);

	ImGui::SeparatorText("Tell it");
	const bool busy = creature_layers::IsPlaying(body);
	if (ImGui::BeginCombo("Action", AnimationLabel(animations::k_FirstAction + _action).c_str()))
	{
		for (size_t i = 0; i < animations::k_ActionCount; ++i)
		{
			if (ImGui::Selectable(AnimationLabel(animations::k_FirstAction + i).c_str(), i == _action))
			{
				_action = i;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(busy);
	if (ImGui::Button("Play"))
	{
		minds.PlayAction(entity, animations::k_FirstAction + _action);
	}
	ImGui::EndDisabled();
	if (ImGui::BeginCombo("Gesture", AnimationLabel(animations::k_FirstGesture + _gesture).c_str()))
	{
		for (size_t i = 0; i < animations::k_GestureCount; ++i)
		{
			if (ImGui::Selectable(AnimationLabel(animations::k_FirstGesture + i).c_str(), i == _gesture))
			{
				_gesture = i;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(animation->gesture.animation.has_value());
	if (ImGui::Button("Gesture"))
	{
		minds.PlayGesture(entity, animations::k_FirstGesture + _gesture);
	}
	ImGui::EndDisabled();
	ImGui::TextUnformatted("Face");
	for (size_t i = 0; i < animations::k_FaceCount; ++i)
	{
		if (i % 6 != 0)
		{
			ImGui::SameLine();
		}
		if (ImGui::SmallButton(AnimationLabel(animations::k_FirstFace + i).c_str()))
		{
			minds.PullFace(entity, animations::k_FirstFace + i);
		}
	}
	ImGui::BeginDisabled(busy);
	if (ImGui::Button("Sit down"))
	{
		minds.SitDown(entity);
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(!creature_layers::IsLooping(body));
	if (ImGui::Button("Stand up"))
	{
		minds.StandUp(entity);
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Stroke"))
	{
		minds.Feedback(entity, true);
	}
	ImGui::SameLine();
	if (ImGui::Button("Slap"))
	{
		minds.Feedback(entity, false);
	}

	ImGui::SeparatorText("Desires");
	ImGui::Text("Energy %.2f, exhaustion %.2f, alone %.0f s", static_cast<double>(mind->energy),
	            static_cast<double>(mind->exhaustion), static_cast<double>(mind->secondsAlone));
	if (!mind->desires.has_value())
	{
		return;
	}
	ImGui::Text("All desires add up to %.2f", static_cast<double>(mind->desires->sum));
	// Strongest first
	std::array<creature_desires::Desire, creature_desires::k_DesireCount> order {};
	for (size_t i = 0; i < order.size(); ++i)
	{
		order.at(i) = static_cast<creature_desires::Desire>(i);
	}
	std::ranges::stable_sort(order, std::greater {},
	                         [&desires = *mind->desires](creature_desires::Desire desire) { return desires[desire].value; });
	for (const auto desire : order)
	{
		const auto& state = (*mind->desires)[desire];
		if (!state.activated || (state.sources.empty() && state.value <= 0.0f))
		{
			continue;
		}
		const auto label = fmt::format("{:.2f} / {:.2f}{}", state.value, state.max, state.suppressedTurns > 0 ? " held" : "");
		ImGui::ProgressBar(state.max > 0.0f ? state.value / state.max : 0.0f, ImVec2(160.0f, 0.0f), label.c_str());
		ImGui::SameLine();
		ImGui::TextUnformatted(creature_desires::Name(desire).data());
		if (ImGui::IsItemHovered() && !state.sources.empty())
		{
			std::string sources;
			for (const auto& source : state.sources)
			{
				sources += fmt::format("source {}: {:.2f} past {:.2f}\n", source.type, source.value, source.threshold);
			}
			ImGui::SetTooltip("%s", sources.c_str());
		}
	}
}

void CreatureSpawner::DrawPlacing() noexcept
{
	const auto colour = _placing ? k_PlacingColour : k_StartColour;
	ImGui::PushStyleColor(ImGuiCol_Button, colour);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(colour.x * 1.15f, colour.y * 1.15f, colour.z * 1.15f, 1.0f));
	if (ImGui::Button(_placing ? "Stop Placing Creatures" : "Start Placing Creatures", ImVec2(-1.0f, 0.0f)))
	{
		_placing = !_placing;
		_placeAt.reset();
	}
	ImGui::PopStyleColor(2);
	if (_placing)
	{
		ImGui::TextWrapped("Right click on the land to place a creature there");
	}
}

void CreatureSpawner::DrawCreatures() noexcept
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();

	std::vector<entt::entity> creatures;
	registry.Each<const Creature>([&creatures](entt::entity entity, const Creature&) { creatures.push_back(entity); });

	ImGui::SeparatorText("On the land");
	if (Locator::creatureHairSystem::has_value())
	{
		auto& hair = Locator::creatureHairSystem::value();
		bool shown = hair.IsShown();
		if (ImGui::Checkbox("Show hair", &shown))
		{
			hair.SetShown(shown);
		}
	}
	ImGui::Text("%zu creature%s", creatures.size(), creatures.size() == 1 ? "" : "s");
	if (_selected.has_value() && std::ranges::find(creatures, *_selected) == creatures.end())
	{
		_selected.reset();
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(creatures.empty());
	if (ImGui::Button("Remove all"))
	{
		registry.Destroy(creatures.begin(), creatures.end());
		creatures.clear();
		_selected.reset();
	}
	ImGui::EndDisabled();

	std::optional<entt::entity> remove;
	if (ImGui::BeginTable("Creatures", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp,
	                      ImVec2(0.0f, 0.0f)))
	{
		ImGui::TableSetupColumn("Species");
		ImGui::TableSetupColumn("Owner");
		ImGui::TableSetupColumn("Where");
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableHeadersRow();
		for (const auto entity : creatures)
		{
			const auto& creature = registry.Get<Creature>(entity);
			const auto& transform = registry.Get<Transform>(entity);
			ImGui::PushID(static_cast<int>(entt::to_integral(entity)));
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(SpeciesName(creature.species).data());
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(k_OwnerNames.at(static_cast<size_t>(creature.owner)).data());
			ImGui::TableNextColumn();
			ImGui::Text("%.0f, %.0f", static_cast<double>(transform.position.x), static_cast<double>(transform.position.z));
			ImGui::TableNextColumn();
			const bool selected = _selected == entity;
			if (ImGui::SmallButton(selected ? "Selected" : "Select"))
			{
				_selected = entity;
			}
			ImGui::TableNextColumn();
			if (ImGui::SmallButton("Remove"))
			{
				remove = entity;
			}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
	if (remove.has_value())
	{
		if (_selected == *remove)
		{
			_selected.reset();
		}
		registry.Destroy(*remove);
	}
}

void CreatureSpawner::Update() noexcept
{
	if (_placeAt.has_value())
	{
		Spawn(*_placeAt);
		_placeAt.reset();
	}
}

void CreatureSpawner::Spawn(glm::vec2 screenCoord) noexcept
{
	if (!Locator::terrainSystem::has_value() || !Locator::dynamicsSystem::has_value())
	{
		return;
	}
	const auto hit = Locator::camera::value().RaycastScreenCoordToLand(screenCoord, false);
	if (!hit.has_value())
	{
		return;
	}
	const auto degrees = _randomFacing ? std::uniform_real_distribution(0.0f, 360.0f)(_random) : _facingDegrees;
	CreatureArchetype::Create(hit->position, _owner, _species, 0, glm::radians(degrees), _scale,
	                          {.alignment = _alignment, .fatness = _fatness, .strength = _strength});
}

bool CreatureSpawner::TakesEvent(const SDL_Event& event) const noexcept
{
	// While placing, a right click on the land is the window's rather than the hand's
	return _placing && IsRightButton(event) && !ImGui::GetIO().WantCaptureMouse;
}

void CreatureSpawner::ProcessEventOpen(const SDL_Event& event) noexcept
{
	if (!TakesEvent(event) || event.type != SDL_MOUSEBUTTONDOWN || !Locator::windowing::has_value())
	{
		return;
	}
	const auto size = static_cast<glm::vec2>(Locator::windowing::value().GetSize());
	_placeAt = glm::vec2(static_cast<float>(event.button.x), static_cast<float>(event.button.y)) / size;
}

void CreatureSpawner::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) noexcept {}
