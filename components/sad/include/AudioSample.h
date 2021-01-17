/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <cstdint>

enum class AudioBankLoop : uint16_t
{
	None,
	Restart,
	Once,
	Overlap,
};

// TODO: Look for channel count (e.g 2)
// TODO: Look for word length (e.g 16);
struct AudioBankSample
{
	char audioSampleName[255];
	int32_t unknown0;
	int32_t id;
	int32_t isBank;
	uint32_t size;
	uint32_t relativeOffset;
	int32_t clone;
	int16_t group;
	int16_t atmosGroup;
	int32_t unknown4;
	int32_t unknown5;
	int16_t unknown6a;
	int16_t unknown6b;
	uint32_t sampleRate;
	int16_t unknownOthera;
	int16_t unknownOtherb;
	int16_t unknown7a;
	int16_t unknown7b;
	int32_t unknown8;
	int32_t lStart;
	int32_t lEnd;
	char sampleDescription[255];
	uint16_t priority;       // 0-9999
	uint16_t unknown9;       //
	uint16_t unknown10;      //
	uint16_t unknown11;      //
	int16_t loop;            //
	uint16_t start;          //
	uint8_t pan;             //
	uint16_t unknown12;      //
	float xPos;              // -9999 to 9999
	float yPos;              // -9999 to 9999
	float zPos;              // -9999 to 9999
	uint8_t volume;          //
	uint16_t userParam;      //
	uint16_t pitch;          //
	uint16_t unknown18;      //
	uint16_t pitchDeviation; //
	uint16_t unknown20;      //
	float minDist;           //
	float maxDist;           //
	float scale;             // 0-50 (multiply by 10)
	AudioBankLoop loopType;  //
	uint16_t unknown21;      //
	uint16_t unknown22;      //
	uint16_t unknown23;      //
	uint16_t atmos;          //
};

static_assert(sizeof(AudioBankSample) == 640, "Audio sample header has incorrect size");
