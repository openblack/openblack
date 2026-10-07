/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ParticleSounds.h"

#include <algorithm>
#include <memory>

using namespace openblack::particles;

ParticleSoundLink* openblack::particles::StartAtomSound(Effect& effect, Atom& atom, const ParticleSound& sound)
{
	if (sound.Silent())
	{
		return nullptr;
	}
	const auto& world = effect.Services().world;
	auto position = effect.GlobalPosition(atom);
	if (sound.onLand)
	{
		position.y = world.LandHeight({position.x, position.z});
	}
	auto played = sound;
	played.alignment = world.SoundAlignment(effect.GetPlayer());
	if (sound.action.useSurface)
	{
		played.surface = world.SurfaceAt(position);
	}
	auto link = std::make_shared<ParticleSoundLink>(
	    ParticleSoundLink {.sound = played, .atom = &atom, .position = position, .fadeStep = 0});
	atom.sounds.insert(atom.sounds.begin(), link);
	effect.Services().world.StartSound(effect, link);
	return link.get();
}

ParticleSoundLink* openblack::particles::FindAtomSound(const Atom& atom, std::string_view action)
{
	const auto found =
	    std::ranges::find_if(atom.sounds, [action](const auto& sound) { return sound->sound.action.sound == action; });
	return found != atom.sounds.end() ? found->get() : nullptr;
}

void openblack::particles::StopAtomSound(Atom& atom, const ParticleSoundLink& sound, int fadeStep)
{
	std::erase_if(atom.sounds, [&sound, fadeStep](const auto& link) {
		if (link.get() != &sound)
		{
			return false;
		}
		link->atom = nullptr;
		if (fadeStep != 0)
		{
			link->fadeStep = fadeStep;
		}
		return true;
	});
}

void openblack::particles::StopAllAtomSounds(Atom& atom)
{
	for (const auto& link : atom.sounds)
	{
		link->atom = nullptr;
	}
	atom.sounds.clear();
}

int openblack::particles::SoundSizeFromThrow(float share)
{
	if (share > k_LargeThrowShare)
	{
		return 1;
	}
	return share > k_MediumThrowShare ? 2 : 3;
}

int openblack::particles::SoundSizeFromImpactSpeed(float speed, float medium, float large)
{
	if (speed < medium)
	{
		return 3;
	}
	return speed < large ? 2 : 1;
}
