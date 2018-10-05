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
		inline uint8_t Country() { return flags1 & 0x0F; }
		inline bool IsWater() { return flags1 & 0x10; }
		inline bool IsCoast() { return flags1 & 0x20; }
	private:
		rgba_t color;

		uint8_t altitude;
		uint8_t savecolor;

		// Cell splitting: Normal, Other (Triangle direction?)
		// Cell properties: Cell has water, Cell has coastline, Cell is full watercell
		// Sound properties: Has coastal sound, Has land sound, Has sea sound, Has freshwater sound
		// Country Style
		uint8_t flags1; // 0x0F = country 0x80 = split // & 0x10 = water & 0x20 = coast
		uint8_t flags2;
	};
}

#endif
