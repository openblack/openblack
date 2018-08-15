#pragma once
#ifndef OPENBLACK_LANDCELL_H
#define OPENBLACK_LANDCELL_H

#include <stdint.h>

namespace OpenBlack
{
	struct LandCell
	{
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t l;

		uint8_t altitude;
		uint8_t savecolor;

		uint8_t flags1;
		uint8_t flags2;

		// Cell splitting: Normal, Other (Triangle direction?)
		// Cell properties: Cell has water, Cell has coastline, Cell is full watercell
		// Sound properties: Has coastal sound, Has land sound, Has sea sound, Has freshwater sound
		// Country Style
		//uint8_t flags1; // 0x0F = country 0x80 = split // & 0x10 = water & 0x20 = coast
		//uint8_t flags2; //

		// vert->country = cell->flags1 & 0x0F;
		// country, water, coast, fullwater, split,
		// sound properties: coastal, land, sea, fresh

		//

		const uint8_t Country() const { return flags1 & 0x0F;  }
		const bool IsWater() const { return flags1 & 0x10; }
		const bool IsCoast() const { return flags1 & 0x20; }
	};
}

#endif
