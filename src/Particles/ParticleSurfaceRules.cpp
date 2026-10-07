/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The rule that makes one atom in its collection and draws it as a surface turned round the vertical: the swirl of light
// under a miracle dispenser, the teleport's pool, the vortices and volcanoes the land opens. The surface is built once,
// in the colours its rings are shaded with, laid over the land when asked; then it sways, unless it lies on the land, and
// its texture slides round and out.

#include <cmath>

#include <algorithm>
#include <memory>
#include <vector>

#include <ParticleFile.h>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>

#include "ParticleClassRegistry.h"
#include "ParticleSurfaces.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
/// The sway turns over once in this many seconds times two pi
constexpr double k_SwayPeriod = 6.2831854820251465;
constexpr float k_SwaySpeed = 1.0f;
/// A texture's size is given in pixels; its sheet spans 256
constexpr float k_SheetPixels = 256.0f;
/// Without a player to take the colour of, the dark rings shine white
constexpr surface::Argb k_NoPlayerSpecular = 0xFFFFFFFFu;

uint8_t Channel(const ParticleObject& object, std::string_view key, int fallback)
{
	return static_cast<uint8_t>(std::clamp(object.Int(key, fallback), 0, 255));
}

/// Where a point of an atom's own frame is in the world, and back
struct AtomFrame
{
	glm::mat4 toWorld;
	glm::mat4 toAtom;
};

AtomFrame FrameOf(const Effect& effect, const Collection& collection, const Atom& atom)
{
	const auto scale = Effect::FrameScale(atom);
	const auto origin = effect.LocalToGlobal(collection, atom.position);
	glm::mat4 toWorld(1.0f);
	for (int axis = 0; axis < 3; ++axis)
	{
		const auto along = atom.rotation[axis] * scale[axis];
		toWorld[axis] = glm::vec4(effect.LocalToGlobal(collection, atom.position + along) - origin, 0.0f);
	}
	toWorld[3] = glm::vec4(origin, 1.0f);
	return {.toWorld = toWorld, .toAtom = glm::inverse(toWorld)};
}

class SurfaceOfRevolutionRule final: public Modifier
{
public:
	explicit SurfaceOfRevolutionRule(const ParticleObject& object)
	    : _speed(object.Float("SpeedU", 0.1f), object.Float("SpeedV", 0.1f))
	    , _scale(object.Float("Scale", 1.0f))
	    , _nextGroups(object.IntArray("NextGroups"))
	{
		auto& creator = _creator;
		creator.kind = Creator::Kind::Surface;
		creator.className = object.className;
		creator.rgba = {Channel(object, "ColorR", 255), Channel(object, "ColorG", 255), Channel(object, "ColorB", 255),
		                Channel(object, "ColorA", 255)};
		creator.texture = TextureBaseName(object.String("TextureFileName"));
		creator.additive = object.Bool("UseAdditiveAlpha", false);
		creator.writeDepth = object.Bool("MaterialUpdateZBuffer", false);
		creator.doubleSided = object.Bool("MaterialSetDoubleSided", true);
		const float height = static_cast<float>(object.Int("TextureHeight", 256)) / k_SheetPixels;
		const float width = static_cast<float>(object.Int("TextureWidth", 256)) / k_SheetPixels;
		creator.shape = {
		    .profile = surface::ProfileOf(object.Int("FunctionIndex", 0)),
		    .numU = std::max(object.Int("NumU", 10), 2),
		    .numV = std::max(object.Int("NumV", 10), 2),
		    .fadeAlphas = object.Bool("FadeAlphas", false),
		    .fadeIn = object.Float("AlphaFadeIn", 0.4f),
		    .fadeOut = object.Float("AlphaFadeOut", 0.4f),
		    .specular = object.Bool("ChangeSpecColor", true),
		    // The sheet's height scales the texture round and its width out, as the game has them
		    .uvScale = {height, width},
		};
		creator.wrap = {width, height};
		creator.maxTwist = object.Float("MaxVertexChange", 1.0f);
		creator.maxSlide = object.Float("MaxUVChange", 1.0f);
		creator.drapeOverLand = object.Bool("DoRaiseAboveLandscape", false);
		creator.clampToLand = object.Bool("ClampToLandscape", false);
		creator.lit = object.Bool("UseLighting", true);
	}

	bool ModifyCollection(Effect& effect, Collection& collection, Collection::Slot& slot) const override
	{
		if (slot.first)
		{
			slot.first = false;
			Make(effect, collection);
		}
		const bool sways = !_creator.drapeOverLand && (_creator.maxTwist != 0.0f || _creator.maxSlide != 0.0f);
		const float sway =
		    sways ? static_cast<float>(std::sin(std::fmod(effect.CollectionAge(collection) * k_SwaySpeed, k_SwayPeriod)))
		          : 0.0f;
		for (auto& atom : collection.atoms)
		{
			auto* instance = atom->creator == &_creator ? atom->surface.get() : nullptr;
			if (instance == nullptr)
			{
				continue;
			}
			if (sways)
			{
				surface::Sway(instance->still, _creator.shape, _creator.maxTwist, _creator.maxSlide, sway, instance->mesh);
			}
			// The texture slides; while it and the last step's are both two sheets past the start, both come back a sheet
			instance->uvCurrent += _speed * effect.GetDt();
			for (int axis = 0; axis < 2; ++axis)
			{
				const float wrap = _creator.wrap[axis];
				auto& current = instance->uvCurrent[axis];
				auto& last = instance->uvLast[axis];
				while (current < -2.0f * wrap && last < -2.0f * wrap)
				{
					current += wrap;
					last += wrap;
				}
				while (current > 2.0f * wrap && last > 2.0f * wrap)
				{
					current -= wrap;
					last -= wrap;
				}
			}
			instance->uvPrevious = instance->uvLast;
			instance->uvLast = instance->uvCurrent;
		}
		return true;
	}

private:
	/// The atom, its surface built in its rings' colours, lit if asked, and laid over the land if asked
	void Make(Effect& effect, Collection& collection) const
	{
		auto& atom = effect.NewAtom(collection, &_creator, _nextGroups);
		atom.baseScale *= _scale;
		const int player = effect.GetPlayer();
		const auto specular = _creator.shape.specular && player >= 0
		                          ? (0xFF000000u | effect.Services().world.PlayerColour(player))
		                          : k_NoPlayerSpecular;
		auto instance = std::make_shared<SurfaceInstance>();
		instance->mesh = surface::Build(_creator.shape, specular);
		if (_creator.lit)
		{
			surface::ComputeNormals(instance->mesh);
		}
		instance->still = instance->mesh;
		if (_creator.drapeOverLand)
		{
			// Swayed once to the full, then laid over the land in the world and brought back to the atom's frame
			if (_creator.maxTwist != 0.0f || _creator.maxSlide != 0.0f)
			{
				surface::Sway(instance->still, _creator.shape, _creator.maxTwist, _creator.maxSlide, 1.0f, instance->mesh);
			}
			const auto frame = FrameOf(effect, collection, atom);
			for (auto& vertex : instance->mesh.vertices)
			{
				vertex.position = glm::vec3(frame.toWorld * glm::vec4(vertex.position, 1.0f));
			}
			const glm::vec2 middle {frame.toWorld[3].x, frame.toWorld[3].z};
			const auto& world = effect.Services().world;
			surface::DrapeOverLand(instance->mesh, middle, [&world](glm::vec2 xz) { return world.LandHeight(xz); });
			for (auto& vertex : instance->mesh.vertices)
			{
				vertex.position = glm::vec3(frame.toAtom * glm::vec4(vertex.position, 1.0f));
			}
		}
		atom.surface = std::move(instance);
	}

	SurfaceCreator _creator;
	glm::vec2 _speed;
	float _scale;
	std::vector<int> _nextGroups;
};
} // namespace

glm::vec2 openblack::particles::SurfaceUvOffset(const SurfaceInstance& instance, const SurfaceCreator& creator, float fraction)
{
	auto offset = instance.uvPrevious + (instance.uvLast - instance.uvPrevious) * fraction;
	for (int axis = 0; axis < 2; ++axis)
	{
		const float wrap = creator.wrap[axis];
		while (wrap > 0.0f && offset[axis] > wrap)
		{
			offset[axis] -= wrap;
		}
	}
	return offset;
}

void openblack::particles::RegisterSurfaceRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("ZR_SurfRevol", ParticleClassRegistry::Make<SurfaceOfRevolutionRule>);
}
