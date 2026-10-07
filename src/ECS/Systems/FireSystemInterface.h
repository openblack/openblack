/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <optional>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::particles::draw
{
struct Frame;
}

namespace openblack::ecs::systems
{

/// Fire: everything hotter than the air has a temperature (see fire/FireModel.h). Miracles, scripts and other fires heat
/// things; at their combustion temperature they burn, hurt themselves, heat what is round them and spread through a
/// forest or a town as one blaze, while water, the rain and the villagers beating them cool them. Burning things are
/// drawn with flames, steam as they cool and smoke as they go out, light the land round them and crackle near the camera.
class FireSystemInterface
{
public:
	/// What the debug window shows of a fire
	struct FireInfo
	{
		entt::entity object;
		glm::vec3 position;
		float temperature;
		float combustion;
		float charring;
		float fraction;
		float life;
		bool burning;
		/// The first fire of its blaze, and how many fires and firemen the blaze has
		entt::entity root;
		size_t blazeSize;
		size_t firemen;
	};

	/// What a villager fighting a blaze needs of it
	struct Blaze
	{
		entt::entity root;
		/// The radius of all its burning fires together, and the most urgent of them
		float burningRadius;
		float burningPriority;
		size_t firemen;
	};

	virtual ~FireSystemInterface() = default;

	/// Once a game turn, oldest fire last: each burns, cools, hurts its object and heats what is round it. A fire made
	/// this turn after the burn point waits for the next; one made during the walk is newer still and waits too.
	virtual void ProcessTurn() = 0;
	/// The point in the turn at which the game burns its fires, after the living and before the scripts and the
	/// miracles. The fires burn later in this turn, but those made after this point wait for the next turn.
	virtual void MarkBurnPoint() = 0;
	/// Once a frame: the flames, steam and smoke move on by the game time
	virtual void Update(float seconds) = 0;
	/// A new land: no fires
	virtual void Reset() = 0;

	/// A burn reaches an object, as a miracle's effect or a beating villager gives it: the object's temperature moves
	/// towards the air's plus the burn. A positive burn sets alight what isn't hot yet; a negative one only cools what
	/// already is. A villager it reaches while burning runs.
	virtual void ApplyBurn(entt::entity object, float burn, std::optional<PlayerNames> player) = 0;
	/// An object is made exactly this hot, setting it alight only when that is hotter than it is. A fire it starts is
	/// put down to the object's own player (world_objects::PlayerOf), never heating its source back.
	virtual void SetTemperature(entt::entity object, float temperature, entt::entity source) = 0;
	/// An object is set alight at a speed: its combustion temperature and that much more of twice it
	virtual void SetOnFire(entt::entity object, float speed) = 0;
	/// An object goes back to the air's temperature, putting it out
	virtual void PutOut(entt::entity object) = 0;
	/// An object is leaving the world: its fire goes with it, out of its blaze, without the people taking up what they
	/// thought of it before it burned
	virtual void Forget(entt::entity object) = 0;
	/// An object held in a hand catches from the fires in its cell
	virtual void HeatHeldObject(entt::entity object) = 0;
	/// An object is picked up or thrown: it leaves its blaze, and the living flee it in the hand
	virtual void StartedMoving(entt::entity object, bool inHand) = 0;
	/// A script keeps an object from catching, or lets it catch without being hurt
	virtual void SetCanBeSetOnFire(entt::entity object, bool can) = 0;
	virtual void SetHurtByFire(entt::entity object, bool hurt) = 0;

	[[nodiscard]] virtual float GetTemperature(entt::entity object) const = 0;
	[[nodiscard]] virtual bool IsOnFire(entt::entity object) const = 0;
	/// Whether anything burns within a radius of a point
	[[nodiscard]] virtual bool IsFireNear(const glm::vec3& point, float radius) const = 0;
	/// How charred an object is, 0..1
	[[nodiscard]] virtual float GetCharring(entt::entity object) const = 0;

	// The villagers who fight a blaze
	/// The blaze an object's fire is part of, none when it doesn't burn
	[[nodiscard]] virtual std::optional<Blaze> GetBlaze(entt::entity object) const = 0;
	/// The member of a blaze nearest a point that is still hot enough to react to, none when all are out
	[[nodiscard]] virtual std::optional<entt::entity> NearestHotMember(entt::entity root, const glm::vec3& point) const = 0;
	/// Where a fire is and how far it reaches now, at its fiercest and safely beyond
	struct Reach
	{
		/// Where its object stands, and where it burns from
		glm::vec3 position;
		glm::vec3 centre;
		/// The radius a fire on its object reaches by default, its object's across the ground
		float defaultRadius;
		float radius;
		float maxRadius;
		float safeRadius;
		bool aboveReactionTemperature;
		bool burning;
	};
	[[nodiscard]] virtual std::optional<Reach> GetReach(entt::entity object) const = 0;
	/// The fires of an object's blaze, its first fire first, none when it doesn't burn
	[[nodiscard]] virtual std::vector<entt::entity> GetBlazeMembers(entt::entity object) const = 0;
	/// The fire of an object's blaze nearest a point by the distance to the edge of its safe radius, or its object's own
	/// radius when that is wider, of those hot enough to react to; none when none is
	[[nodiscard]] virtual std::optional<entt::entity> NearestSafeFire(entt::entity object, const glm::vec3& point) const = 0;
	/// Whether two objects' fires are of one blaze
	[[nodiscard]] virtual bool InSameBlaze(entt::entity a, entt::entity b) const = 0;
	/// Another object's blaze joins the blaze of an object's fire
	virtual void MergeBlazes(entt::entity object, entt::entity other) = 0;
	/// The villagers about a blaze, those fighting it and those running from it on fire, are listed with its first
	/// fire: a villager added to, taken from and looked for in the list of an object's blaze
	virtual void AddFireman(entt::entity object, entt::entity villager) = 0;
	virtual void RemoveFireman(entt::entity object, entt::entity villager) = 0;
	[[nodiscard]] virtual bool IsFiremanOf(entt::entity object, entt::entity villager) const = 0;

	/// The flames, steam and smoke drawn this frame, t of the way through the turn, and the light the fires cast on the
	/// land, added to the particles' frame
	virtual void CollectDrawFrame(float turnFraction, particles::draw::Frame& frame) const = 0;
	/// The grey a charred object is drawn in, 0xRRGGBB, none for one not charred
	[[nodiscard]] virtual std::optional<uint32_t> GetCharredColour(entt::entity object) const = 0;
	/// How a tree with a fire on it is drawn: the grey of 256 its light is scaled by, the alpha below which its foliage
	/// is cut away, of 255, and the share of its size it keeps; none for a tree without a fire
	struct TreeLook
	{
		int grey;
		float alphaReference;
		float scale;
	};
	[[nodiscard]] virtual std::optional<TreeLook> GetBurningTreeLook(entt::entity tree) const = 0;
	/// The red-orange glow, 0xRRGGBB, added over an object with a fire on it as it is drawn, flickering with the game
	/// time; none without a fire. Trees are tinted their own way (GetBurningTreeLook) instead.
	[[nodiscard]] virtual std::optional<uint32_t> GetGlowColour(entt::entity object) const = 0;

	[[nodiscard]] virtual std::vector<FireInfo> GetFires() const = 0;
};

} // namespace openblack::ecs::systems
