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

// code derived from FFMPEG's libavcodec/get_bits.h 
// copyright (c) 2004 Michael Niedermayer <michaelni@gmx.at> 
// and binkaudio.cpp 
// Copyright (c) 2007-2009 Peter Ross (pross@xvid.org)
// Copyright (c) 2009 Daniel Verkamp (daniel@drv.nu) 

#include <math.h>
#include "GetBitContext.h"

//don't return more than 25 bits this way
unsigned int GetBitContext::get_bits(int n) {
	register unsigned int tmp;

	tmp = AV_RL32(buffer+(index>>3))>>(index&7);
	index+=n;
	return tmp& (0xffffffff>>(32-n));
}

//peek at the next <25 bits (without bumping the bit counter)
unsigned int GetBitContext::peek_bits(int n) {
	register unsigned int tmp;

	tmp = AV_RL32(buffer+(index>>3))>>(index&7);
	return tmp& (0xffffffff>>(32-n));
}

//i think, this get_bits_long could be simple get_bits (less than 25 bits taken)
float GetBitContext::get_float()
{
	int power = get_bits(5);
	float f = ldexpf((float) get_bits_long(23), power - 23);
	if (get_bits(1))
	    f = -f;
	return f;
}

void GetBitContext::get_bits_align32()
{
	int n = (-get_bits_count()) & 31;
	if (n) skip_bits(n);
}

unsigned int GetBitContext::get_bits_long(int n) 
{
	if(n<=17) return get_bits(n);
	else {
	    int ret= get_bits(16);
	    return ret | (get_bits(n-16) << 16);
	}
} 

void GetBitContext::init_get_bits(const uint8_t *b, int bit_size)
{
	int buffer_size = (bit_size+7)>>3;
	if(buffer_size < 0 || bit_size < 0) {
	    buffer_size = bit_size = 0;
	    buffer = NULL;
	}

	buffer = b;
	size_in_bits = bit_size;
	buffer_end = buffer + buffer_size;
	index=0;
}

//bink video related from bink.c
// * Copyright (c) 2009 Konstantin Shishkov

/**
 * Reads information about Huffman tree used to decode data.
 *
 * @param tree pointer for storing tree data
 */
void GetBitContext::read_tree(Tree *tree)
{
	uint8_t tmp1[16], tmp2[16], *in = tmp1, *out = tmp2;
	int i, t, len;

	tree->vlc_num = get_bits(4);
	if (!tree->vlc_num) {
	    for (i = 0; i < 16; i++)
	        tree->syms[i] = i;
	    return;
	}
	if (get_bits(1)) {
	    len = get_bits(3);
	    memset(tmp1, 0, sizeof(tmp1));
	    for (i = 0; i <= len; i++) {
	        tree->syms[i] = get_bits(4);
	        tmp1[tree->syms[i]] = 1;
	    }
	    for (i = 0; i < 16; i++)
	        if (!tmp1[i])
	            tree->syms[++len] = i;
	} else {
	    len = get_bits(2);
	    for (i = 0; i < 16; i++)
	        in[i] = i;
	    for (i = 0; i <= len; i++) {
	        int size = 1 << i;
	        for (t = 0; t < 16; t += size << 1)
	            merge(out + t, in + t, size);
	        FFSWAP(uint8_t*, in, out);
	    }
	    memcpy(tree->syms, in, 16);
	}
}

/**
 * Merges two consequent lists of equal size depending on bits read.
 *
 * @param gb   context for reading bits
 * @param dst  buffer where merged list will be written to
 * @param src  pointer to the head of the first list (the second lists starts at src+size)
 * @param size input lists size
 */
void GetBitContext::merge( uint8_t *dst, uint8_t *src, int size)
{
	uint8_t *src2 = src + size;
	int size2 = size;

	do {
	    if (!get_bits(1)) {
	        *dst++ = *src++;
	        size--;
	    } else {
	        *dst++ = *src2++;
	        size2--;
	    }
	} while (size && size2);

	while (size--)
	    *dst++ = *src++;
	while (size2--)
	    *dst++ = *src2++;
}


void GetBitContext::debug(const char *prefix)
{
	printf("%s: %d", prefix, index);
}

//VLC specific code from bitstream.c
//removed all non-static parts for simplicity

int VLC::alloc_table(int size)
{
	int index;
	index = table_size;
	table_size += size;
	if (table_size > table_allocated) {
	        abort(); //cant do anything, init_vlc() is used with too little memory
	}
	return index;
}

int VLC::build_table(int table_nb_bits, int nb_codes,
	             const void *bits, int bits_wrap, int bits_size,
	             const void *codes, int codes_wrap, int codes_size,
	             uint32_t code_prefix, int n_prefix, int flags)
{
	int i, j, k, n, table_size, table_index, nb, n1, index, symbol;
	uint32_t code_prefix2;
	uint32_t code;
	int16_t (*p_table)[2];

	table_size = 1 << table_nb_bits;
	table_index = alloc_table(table_size);
	if (table_index < 0 || table_nb_bits > 30 || n_prefix > 31)
	    return -1;
	p_table = &table[table_index];

	for(i=0;i<table_size;i++) {
	    p_table[i][1] = 0; //bits
	    p_table[i][0] = -1; //codes
	}

	/* first pass: map codes and compute auxillary table sizes */
	for(i=0;i<nb_codes;i++) {
	    GET_DATA(n, bits, i, bits_wrap, bits_size);
	    GET_DATA(code, codes, i, codes_wrap, codes_size);
	    /* we accept tables with holes */
	    if (n <= 0)
	        continue;
	    symbol = i;
	    /* if code matches the prefix, it is in the table */
	    n -= n_prefix;
	    if(flags & INIT_VLC_LE)
	        code_prefix2= code & ((1 << n_prefix)-1);
	    else
	        code_prefix2= code >> n;
	    if (n > 0 && code_prefix2 == code_prefix) {
	        if (n <= table_nb_bits) {
	            /* no need to add another table */
	            j = (code << (table_nb_bits - n)) & (table_size - 1);
	            nb = 1 << (table_nb_bits - n);
	            for(k=0;k<nb;k++) {
	                if(flags & INIT_VLC_LE)
	                    j = (code >> n_prefix) + (k<<n);
	                if (p_table[j][1] /*bits*/ != 0) {
	                    //av_log(NULL, AV_LOG_ERROR, "incorrect codes\n");
	                    return -1;
	                }
	                p_table[j][1] = n; //bits
	                p_table[j][0] = symbol;
	                j++;
	            }
	        } else {
	            n -= table_nb_bits;
	            j = (code >> ((flags & INIT_VLC_LE) ? n_prefix : n)) & ((1 << table_nb_bits) - 1);
	            /* compute table size */
	            n1 = -p_table[j][1]; //bits
	            if (n > n1)
	                n1 = n;
	            p_table[j][1] = -n1; //bits
	        }
	    }
	}

	/* second pass : fill auxillary tables recursively */
	for(i=0;i<table_size;i++) {
	    n = p_table[i][1]; //bits
	    if (n < 0) {
	        n = -n;
	        if (n > table_nb_bits) {
	            n = table_nb_bits;
	            p_table[i][1] = -n; //bits
	        }
	        index = build_table(n, nb_codes,
	                            bits, bits_wrap, bits_size,
	                            codes, codes_wrap, codes_size,
	                            (flags & INIT_VLC_LE) ? (code_prefix | (i << n_prefix)) : ((code_prefix << table_nb_bits) | i),
	                            n_prefix + table_nb_bits, flags);
	        if (index < 0)
	            return -1;
	        /* note: realloc has been done, so reload tables */
	        p_table = &table[table_index];
	        p_table[i][0] = index; //code
	    }
	}
	return table_index;
}

/* Build VLC decoding tables suitable for use with get_vlc().

	 'nb_bits' set thee decoding table size (2^nb_bits) entries. The
	 bigger it is, the faster is the decoding. But it should not be too
	 big to save memory and L1 cache. '9' is a good compromise.

	 'nb_codes' : number of vlcs codes

	 'bits' : table which gives the size (in bits) of each vlc code.

	 'codes' : table which gives the bit pattern of of each vlc code.

	 'symbols' : table which gives the values to be returned from get_vlc().

	 'xxx_wrap' : give the number of bytes between each entry of the
	 'bits' or 'codes' tables.

	 'xxx_size' : gives the number of bytes of each entry of the 'bits'
	 or 'codes' tables.

	 'wrap' and 'size' allows to use any memory configuration and types
	 (byte/word/long) to store the 'bits', 'codes', and 'symbols' tables.

	 'use_static' should be set to 1 for tables, which should be freed
	 with av_free_static(), 0 if free_vlc() will be used.
*/
int VLC::init_vlc(int nb_bits, int nb_codes,
	         const void *p_bits, int bits_wrap, int bits_size,
	         const void *codes, int codes_wrap, int codes_size,
	         int flags)
{
	bits = nb_bits;
	if(table_size) {
	if(table_size == table_allocated) {
		return 0;
	} else if(table_size) {
		//trying to reallocate static table
		return -1;
		}
	}
	
	if (build_table(nb_bits, nb_codes, p_bits, bits_wrap, bits_size,
		codes, codes_wrap, codes_size, 0, 0, flags) < 0) {
		//no need of freeing this, it was allocated staticly
		//av_freep((void **) &table);
		return -1;
	}
	if(table_size != table_allocated) {
		//av_log(NULL, AV_LOG_ERROR, "needed %d had %d\n", table_size, table_allocated);
	}
	return 0;
}

