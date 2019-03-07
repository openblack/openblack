/* OpenBlack - A reimplementation of Lionheads's Black & White engine
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

//code derived from FFMPEG
//using code from get_bits.h, bitstream.c

#include "common.h"

#define INIT_VLC_LE         2
#define INIT_VLC_USE_NEW_STATIC 4 

class VLC
{
public:
	int bits;
	int16_t (*table)[2]; ///< code, bits
	int table_size, table_allocated;
public:
	int init_vlc(int nb_bits, int nb_codes,
		const void *bits, int bits_wrap, int bits_size,
		const void *codes, int codes_wrap, int codes_size,
		int flags);
private:
	int alloc_table(int size);
	int build_table(int table_nb_bits, int nb_codes,
		const void *bits, int bits_wrap, int bits_size,
		const void *codes, int codes_wrap, int codes_size,
		uint32_t code_prefix, int n_prefix, int flags);

};

#if defined(__arm__)
#define GET_DATA(v, table, i, wrap, size) \
{\
	const uint8_t *ptr = (const uint8_t *)table + i * wrap;\
	v = 0;\
	switch(size) {\
	default:\
		v |= *((const uint8_t *)ptr+3) << 24;\
		v |= *((const uint8_t *)ptr+2) << 16;\
	case 2:\
		v |= *((const uint8_t *)ptr+1) << 8;\
	case 1:\
		v |= *(const uint8_t *)ptr;\
	}\
}
#else
#define GET_DATA(v, table, i, wrap, size) \
{\
	const uint8_t *ptr = reinterpret_cast<const uint8_t *>(table) + i * wrap;\
	switch(size) {\
	case 1:\
		v = *reinterpret_cast<const uint8_t *>(ptr);\
		break;\
	case 2:\
		v = *reinterpret_cast<const uint16_t *>(ptr);\
		break;\
	default:\
		v = *reinterpret_cast<const uint32_t *>(ptr);\
		break;\
	}\
}
#endif

#define AV_RL32(x)                           \
	((((const uint8_t*)(x))[3] << 24) |  \
	(((const uint8_t*)(x))[2] << 16) |   \
	(((const uint8_t*)(x))[1] <<  8) |   \
	((const uint8_t*)(x))[0]) 

class GetBitContext
{
public:
	const uint8_t *buffer, *buffer_end;
	int index;
	int size_in_bits;
public:
	GetBitContext() : buffer(NULL), buffer_end(NULL), index(0), size_in_bits(0) {};
	void debug(const char *prefix);
	float get_float();
	void skip_bits(int x) { index+=x; }
	int get_bits_count() { return index; }
	void get_bits_align32();
	unsigned int get_bits(int x);
	unsigned int peek_bits(int x);
	unsigned int get_bits_long(int n);
	void init_get_bits(const uint8_t *b, int bit_size);
	void read_tree(Tree *tree);
private:
	void merge( uint8_t *dst, uint8_t *src, int size);
};
