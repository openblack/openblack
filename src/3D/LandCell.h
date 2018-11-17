#pragma once
#ifndef OPENBLACK_LANDCELL_H
#define OPENBLACK_LANDCELL_H

#include <Common/Types.h>

namespace OpenBlack
{
	class LandCell
	{
	public:
		inline uint8_t Altitude() { return altitude; }
		inline uint8_t Country() { return flags1 & 0x0F; } // 0000 1111
		inline bool Split() { return flags1 & 0x80; } // 1000 0000
		inline bool HasWater() { return flags1 & 0x10; } // 0001 0000
		inline bool Coastline() { return flags1 & 0x20; } // 0010 0000
		inline bool FullWater() { return flags1 & 0x40; } // 0100 0000
	private:
		rgba_t color;

		uint8_t altitude;
		uint8_t savecolor;

		// Sound properties: coastal sound, land sound, sea sound, freshwater sound
		uint8_t flags1; // 0x0F = country 0x80 = split // & 0x10 = water & 0x20 = coast
		uint8_t flags2; // sound properties ..
	};
}

#endif
