/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "GestureSystem.h"

#include <stdexcept>
#include <utility>

#include <GestureFile.h>
#include <entt/core/hashed_string.hpp>
#include <glm/common.hpp>
#include <glm/vector_relational.hpp>
#include <spdlog/spdlog.h>

#include "Creature/LeashRules.h"
#include "ECS/Components/SpellSeed.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureFightSystemInterface.h"
#include "ECS/Systems/LeashSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "Gestures/GestureTrailBuilder.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/MagicTables.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using openblack::gesture::Purpose;

namespace
{
/// The game's templates in the resource caches
constexpr auto k_TemplatesId = entt::hashed_string("gestures/templates");

/// How long a recognised gesture shows in the debug window
constexpr float k_RecognisedShownSeconds = 10.0f;

/// The shape a gesture's trail takes on the land, from its symbol's file; the circle's for a gesture without one
std::optional<gesture::TrailSymbol> TrailSymbolOf(GestureType gesture)
{
	if (!Locator::resources::has_value())
	{
		return std::nullopt;
	}
	const auto& paths = Locator::resources::value().GetCameraPaths();
	for (const auto type : {gesture, GestureType::Circle})
	{
		const auto id = fmt::format("symbol/PathSymbol{}", static_cast<uint32_t>(type));
		if (!paths.Contains(entt::hashed_string(id.c_str()).value()))
		{
			continue;
		}
		const auto path = paths.Handle(entt::hashed_string(id.c_str()));
		std::vector<glm::vec3> points;
		points.reserve(path->GetPoints().size());
		for (const auto& point : path->GetPoints())
		{
			points.push_back(point.position);
		}
		return gesture::MakeTrailSymbol(points);
	}
	return std::nullopt;
}
} // namespace

GestureSystem::GestureSystem() = default;
GestureSystem::~GestureSystem() = default;

std::vector<GestureEvent> GestureSystem::TakeEvents()
{
	return std::exchange(_events, {});
}

void GestureSystem::Inject(const GestureEvent& event)
{
	_events.push_back(event);
}

void GestureSystem::Reset()
{
	_events.clear();
	ForgetPath();
	_drawing.clear();
	_picker.Close();
	_circleSeconds = 0.0f;
	_circleSeed = entt::null;
	_lastRecognised.reset();
	_requests.clear();
}

void GestureSystem::ForgetPath()
{
	_recorder.Reset();
	_sinceSample = 0.0f;
}

void GestureSystem::DrawPath(std::vector<glm::vec2> path, bool holdingAction)
{
	_drawing.assign(path.begin(), path.end());
	_drawingHoldsAction = holdingAction;
	// A drawing starts afresh, as the game's hand does once it has rested
	ForgetPath();
	_pause = 0.0f;
}

std::span<const gestures::GestureTemplate> GestureSystem::GetTemplates() const
{
	if (_templates == nullptr)
	{
		return {};
	}
	return _templates->GetTemplates();
}

void GestureSystem::LoadTemplates()
{
	if (_templatesTried || !Locator::resources::has_value() || !Locator::filesystem::has_value())
	{
		return;
	}
	_templatesTried = true;
	auto& cache = Locator::resources::value().GetGestureTemplates();
	try
	{
		if (!cache.Contains(k_TemplatesId.value()))
		{
			const auto path = Locator::filesystem::value().GetPath<filesystem::Path::Data>() / "Gestures.jty";
			cache.Load(k_TemplatesId.value(), resources::GestureTemplatesLoader::FromDiskTag {}, path);
		}
		_templates = cache.Handle(k_TemplatesId.value()).handle();
		SPDLOG_LOGGER_INFO(spdlog::get("game"), "Loaded {} gesture templates", _templates->GetTemplates().size());
	}
	catch (const std::runtime_error& error)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "No gestures can be recognised: {}", error.what());
	}
}

gesture::HandContext GestureSystem::ContextOf(const Frame& frame)
{
	const auto player = frame.player;
	gesture::HandContext context;
	context.actionHeld = frame.actionHeld || (!_drawing.empty() && _drawingHoldsAction);
	const auto* info = Locator::infoConstants::has_value() ? &Locator::infoConstants::value() : nullptr;
	if (info != nullptr)
	{
		const auto& spellSystem = info->spellSystem;
		context.leashGesture = spellSystem.leashSelectionStart;
		context.leashGestures = spellSystem.leashSelectionGestures;
	}

	// The seed in the hand
	std::optional<entt::entity> seedEntity;
	if (Locator::magicSystem::has_value())
	{
		seedEntity = Locator::magicSystem::value().GetHeldSeed();
	}
	if (seedEntity.has_value() && info != nullptr)
	{
		const auto& registry = Locator::entitiesRegistry::value();
		if (const auto* seed = registry.TryGet<ecs::components::SpellSeed>(*seedEntity); seed != nullptr)
		{
			const auto& seedInfo = magic::GetSpellSeedInfo(*info, seed->seedType);
			// Only a seed made at an icon of the player's worship can be powered up
			context.seed = gesture::HandContext::Seed {.ready = seed->ready,
			                                           .cast = seed->hasCast,
			                                           .sizingGesture = seedInfo.sizingGesture,
			                                           .powerUpGestures = seedInfo.powerUpGestures,
			                                           .powerUp = seed->powerUp,
			                                           .canPowerUp = seed->hasIcon};
			context.inInfluence = Locator::magicSystem::value().IsHandInInfluence();
		}
	}
	// A circle is remembered for the seed it was drawn for
	if (!seedEntity.has_value() || *seedEntity != _circleSeed)
	{
		_circleSeconds = 0.0f;
	}
	context.circleRemembered = _circleSeconds > 0.0f;

	// The player's creature and its leash
	if (Locator::leashSystem::has_value())
	{
		const auto& leashes = Locator::leashSystem::value();
		if (const auto creature = leashes.PlayersCreature(player))
		{
			gesture::HandContext::Creature state;
			state.fighting =
			    Locator::creatureFightSystem::has_value() && Locator::creatureFightSystem::value().IsFighting(*creature);
			state.leashed = leashes.IsLeashed(*creature);
			state.tied = leashes.TiedTo(*creature).has_value();
			for (size_t i = 0; i < creature_leash::k_Types.size(); ++i)
			{
				state.knows.at(i) = leashes.Knows(*creature, creature_leash::k_Types.at(i));
			}
			state.worn = state.leashed ? leashes.TypeOf(*creature) : LeashType::None;
			context.creature = state;
		}
	}
	context.pickerOpen = _picker.open;
	return context;
}

void GestureSystem::Update(const Frame& frame)
{
	LoadTemplates();
	if (frame.view.screenSize.x > 0.0f && frame.view.screenSize.y > 0.0f)
	{
		_screenAspect = frame.view.screenSize.x / frame.view.screenSize.y;
	}
	if (_lastRecognised.has_value())
	{
		_lastRecognised->age += frame.seconds;
		if (_lastRecognised->age > k_RecognisedShownSeconds)
		{
			_lastRecognised.reset();
		}
	}
	_pause = std::max(_pause - frame.seconds, 0.0f);
	_circleSeconds = std::max(_circleSeconds - frame.seconds, 0.0f);

	// The leash picker closes once the leash is off, or after a while
	auto context = ContextOf(frame);
	const bool leashed = context.creature.has_value() && context.creature->leashed;
	const auto timeout =
	    Locator::infoConstants::has_value() ? Locator::infoConstants::value().spellSystem.leashSelectionSystemTimeOut : 25.0f;
	_picker.Update(frame.seconds, timeout, leashed);
	context.pickerOpen = _picker.open;
	_requests = gesture::Requests(context);
	// The chain follows the hand while it powers up the miracle in it, or holds one that a circle sizes
	const auto& seed = context.seed;
	_gesturing =
	    seed.has_value() && ((seed->ready && !seed->cast && seed->canPowerUp) || seed->sizingGesture == GestureType::Circle);

	// Moving the camera wipes the path being drawn, though not while the camera shakes. Only the camera of a creature
	// fight in an arena, or one following a thing, would let the player draw on while it moves, and neither exists yet
	const bool cameraMoved = _lastCameraEye.has_value() && *_lastCameraEye != frame.view.cameraEye;
	_lastCameraEye = frame.view.cameraEye;
	if (cameraMoved && !frame.cameraShaking)
	{
		_recorder.Reset();
	}

	// The hand's path is only drawn over the world; a path drawn for the testbed goes on regardless
	if (!frame.overWorld && _drawing.empty())
	{
		ForgetPath();
		return;
	}

	// Sampled at the game's pace, not the frame's
	_sinceSample += frame.seconds;
	if (_sinceSample < gesture::k_SampleSeconds)
	{
		return;
	}
	_sinceSample = 0.0f;
	const auto toReference = frame.view.screenSize.y > 0.0f ? gesture::k_ReferenceHeight / frame.view.screenSize.y : 1.0f;
	glm::vec2 point = frame.cursor * toReference;
	if (!_drawing.empty())
	{
		point = _drawing.front();
		_drawing.pop_front();
	}
	if (_pause > 0.0f)
	{
		return;
	}
	const auto land = frame.view.landAt ? frame.view.landAt(point / toReference) : std::nullopt;
	if (land.has_value())
	{
		_recorder.AddPoint(point, *land);
	}
	else
	{
		_recorder.AddPoint(point);
	}

	// The first gesture waited for that the path draws is acted on
	const auto templates = GetTemplates();
	if (templates.empty() || _requests.empty())
	{
		return;
	}
	const auto keys = _recorder.KeyPoints();
	for (const auto& request : _requests)
	{
		if (const auto match = gesture::Recognise(templates, request.gesture, keys, _screenAspect))
		{
			Act(request, *match, frame);
			break;
		}
	}
}

void GestureSystem::LayTrail(GestureType gesture, const gesture::Match& match, const Frame& frame) const
{
	if (!Locator::particleSystem::has_value() || !frame.view.trailPointUnder)
	{
		return;
	}
	const auto points = gesture::TrailLandPoints(_recorder);
	const auto symbol = TrailSymbolOf(gesture);
	if (points.size() < gesture::k_RecognitionShownPoints || !symbol.has_value())
	{
		return;
	}
	// The path's box in the screen's pixels
	const auto toPixels = frame.view.screenSize.y > 0.0f ? frame.view.screenSize.y / gesture::k_ReferenceHeight : 1.0f;
	auto box = gesture::GestureBox(_recorder, match);
	box.min *= toPixels;
	box.max *= toPixels;
	auto trail = gesture::BuildTrail(points, box, *symbol,
	                                 {.cameraForward = frame.view.cameraForward, .pointUnder = frame.view.trailPointUnder});
	if (trail.has_value())
	{
		Locator::particleSystem::value().AddGestureTrail(std::make_shared<particles::GestureTrail>(std::move(*trail)));
	}
}

void GestureSystem::Act(const gesture::Request& request, const gesture::Match& match, const Frame& frame)
{
	const auto box = gesture::GestureBox(_recorder, match);
	const auto handAt = _recorder.Empty() ? glm::vec3(0.0f) : _recorder.At(_recorder.Count() - 1).world;
	SPDLOG_LOGGER_INFO(spdlog::get("game"), "Gesture recognised: {} ({}), template {}{}", gesture::Name(request.gesture),
	                   gesture::Name(request.purpose), match.templateIndex, match.mirrored ? " mirrored" : "");
	_lastRecognised = Recognised {.request = request, .match = match, .box = box, .number = ++_recognisedCount};

	// Every gesture but a scribble leaves its trail on the land, heard as it appears, once enough of its path lies on
	// the land
	if (gesture::ShowsRecognition(request.purpose))
	{
		LayTrail(request.gesture, match, frame);
	}

	auto* leashes = Locator::leashSystem::has_value() ? &Locator::leashSystem::value() : nullptr;
	// Set apart from its declaration: GCC on ARM otherwise takes the optional for possibly uninitialised
	std::optional<entt::entity> creature = std::nullopt;
	if (leashes != nullptr)
	{
		creature = leashes->PlayersCreature(frame.player);
	}
	const auto eventsBefore = _events.size();
	switch (request.purpose)
	{
	case Purpose::SizeCircle:
	{
		// The circle's middle on the land, and its size across the land at that depth
		const auto toPixels = frame.view.screenSize.y > 0.0f ? frame.view.screenSize.y / gesture::k_ReferenceHeight : 1.0f;
		const auto middle = frame.view.landAt ? frame.view.landAt(box.Centre() * toPixels) : std::nullopt;
		const auto centre = middle.value_or(_recorder.At(_recorder.PointsOfKeys(match.firstKey, match.lastKey).first).world);
		const auto edge = frame.view.atDepthOf ? frame.view.atDepthOf(gesture::CircleEdge(box) * toPixels, centre) : centre;
		_events.push_back({.kind = GestureEvent::Kind::Circle,
		                   .gesture = request.gesture,
		                   .centre = centre,
		                   .radius = gesture::CircleRadius(centre, edge, frame.view.cameraRight)});
		_circleSeconds = gesture::k_CircleSeconds;
		_circleSeed =
		    Locator::magicSystem::has_value() ? Locator::magicSystem::value().GetHeldSeed().value_or(entt::null) : entt::null;
		break;
	}
	case Purpose::PowerUp:
		_events.push_back({.kind = GestureEvent::Kind::PowerUp,
		                   .gesture = request.gesture,
		                   .centre = handAt,
		                   .powerUpLevel = request.powerUpLevel});
		break;
	case Purpose::DropSeed:
		_events.push_back({.kind = GestureEvent::Kind::Scribble, .gesture = request.gesture, .centre = handAt});
		break;
	case Purpose::ShakeOffLeash:
		if (leashes != nullptr)
		{
			leashes->Shake(frame.player);
		}
		break;
	case Purpose::LeashGesture:
		if (creature.has_value())
		{
			// An unleashed creature has the leash put on; knowing more than one, it may then be changed
			if (!leashes->IsLeashed(*creature))
			{
				leashes->Toggle(*creature);
			}
			int known = 0;
			for (const auto type : creature_leash::k_Types)
			{
				known += leashes->Knows(*creature, type) ? 1 : 0;
			}
			if (known > 1 && leashes->IsLeashed(*creature))
			{
				_picker.Open();
			}
		}
		break;
	case Purpose::PickLeash:
		if (creature.has_value())
		{
			leashes->ChangeType(*creature, request.leash);
		}
		_picker.Close();
		break;
	case Purpose::ClosePicker:
		_picker.Close();
		break;
	}

	if (_events.size() > eventsBefore)
	{
		_lastRecognised->event = _events.back();
		const auto& event = _events.back();
		SPDLOG_LOGGER_INFO(
		    spdlog::get("game"), "Gesture event for the miracles: kind {}, at {:.1f}, {:.1f}, {:.1f}, radius {:.1f}, level {}",
		    static_cast<int>(event.kind), event.centre.x, event.centre.y, event.centre.z, event.radius, event.powerUpLevel);
	}

	// The path is forgotten and the hand rests a moment
	ForgetPath();
	_pause = gesture::k_RecognisedPauseSeconds;
	_requests = gesture::Requests(ContextOf(frame));
}
