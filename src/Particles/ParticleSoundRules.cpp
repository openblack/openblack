/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The rules that start and stop the sounds a particle keeps going, by a condition or once it is old enough

#include <string>

#include <ParticleFile.h>

#include "ParticleClassRegistry.h"
#include "ParticleSounds.h"

using namespace openblack::particles;
using openblack::psys::ParticleObject;

namespace
{
/// While the condition holds (or without one) the atom has its sound, started once; when it fails the sound is let go,
/// fading by the step
class StartStopSoundOnCondition final: public Modifier
{
public:
	explicit StartStopSoundOnCondition(const ParticleObject& object)
	    : sound({.action = object.Sound("Sound")})
	    , soundCondition(object.String("SoundCondition"))
	    , fadeStep(object.Int("FadeStep", 0))
	{
	}

	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		auto* playing = FindAtomSound(atom, sound.action.sound);
		if (!soundCondition.empty() && !effect.ConditionForAtom(soundCondition, atom))
		{
			if (playing != nullptr)
			{
				StopAtomSound(atom, *playing, fadeStep);
			}
			return true;
		}
		if (playing == nullptr)
		{
			StartAtomSound(effect, atom, sound);
		}
		return true;
	}

	ParticleSound sound;
	std::string soundCondition;
	int fadeStep;
};

/// Once the atom is old enough and the condition holds (or without one) it starts its sound, once, stopping its others
/// first when asked. The camera shake some files ask for with it is not done.
class AddSoundToAtom final: public Modifier
{
public:
	explicit AddSoundToAtom(const ParticleObject& object)
	    : sound({.action = object.Sound("Sound")})
	    , soundCondition(object.String("SoundCondition"))
	    , stopOthers(object.Bool("StopOtherSoundsFirst", false))
	    , delay(object.Float("Delay", 0.0f))
	{
	}

	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		auto& data = atom.data[this];
		if (data.started || effect.AtomAge(atom) < delay ||
		    (!soundCondition.empty() && !effect.ConditionForAtom(soundCondition, atom)))
		{
			return true;
		}
		if (stopOthers)
		{
			StopAllAtomSounds(atom);
		}
		StartAtomSound(effect, atom, sound);
		data.started = true;
		return true;
	}

	ParticleSound sound;
	std::string soundCondition;
	bool stopOthers;
	float delay;
};

/// Once the condition holds (or without one) the atom lets go of its sound, fading by the step
class RemoveSoundFromAtom final: public Modifier
{
public:
	explicit RemoveSoundFromAtom(const ParticleObject& object)
	    : sound({.action = object.Sound("Sound")})
	    , soundCondition(object.String("SoundCondition"))
	    , fadeStep(object.Int("FadeStep", 0))
	{
	}

	bool ModifyAtom(Effect& effect, Atom& atom, Collection::Slot& /*slot*/) const override
	{
		auto& data = atom.data[this];
		if (data.started || (!soundCondition.empty() && !effect.ConditionForAtom(soundCondition, atom)))
		{
			return true;
		}
		if (auto* playing = FindAtomSound(atom, sound.action.sound); playing != nullptr)
		{
			StopAtomSound(atom, *playing, fadeStep);
		}
		data.started = true;
		return true;
	}

	ParticleSound sound;
	std::string soundCondition;
	int fadeStep;
};
} // namespace

void openblack::particles::RegisterSoundRules(ParticleClassRegistry& registry)
{
	registry.AddModifier("StartStopSoundOnCondition", ParticleClassRegistry::Make<StartStopSoundOnCondition>);
	registry.AddModifier("AddSoundToAtom", ParticleClassRegistry::Make<AddSoundToAtom>);
	registry.AddModifier("RemoveSoundFromAtom", ParticleClassRegistry::Make<RemoveSoundFromAtom>);
}
