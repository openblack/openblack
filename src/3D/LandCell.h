#pragma once
#ifndef OPENBLACK_LANDCELL_H
#define OPENBLACK_LANDCELL_H

#include <Common/Types.h>

namespace OpenBlack
{
	class LandCell
	{
	public:
		inline rgba_t Color()     { return color; }
		inline uint8_t Altitude() { return altitude; }
		inline bool Split()       { return properties & 0x80; }
		inline bool HasWater()    { return properties & 0x10; }
		inline bool Coastline()   { return properties & 0x20; }
		inline bool FullWater()	  { return properties & 0x40; } 
		inline uint8_t Country()  { return properties & 0x0F; }
	private:
		rgba_t color;

		uint8_t altitude;
		uint8_t savecolor;

		uint8_t properties;
		// Sound properties: coastal sound, land sound, sea sound, freshwater sound
		uint8_t flags2;
	};
}

#endif
