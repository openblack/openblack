/* OpenBlack - A reimplementation of Lionhead's Black & White.
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

#ifndef OPENBLACK_VIDEO_BINK_VIDEO_H
#define OPENBLACK_VIDEO_BINK_VIDEO_H

#include <Common/OSFile.h>
#include <Graphics/Texture2D.h>
#include <vector>

// FIXME: This has to be included last, since it defines int*_t, which causes
// mingw g++ 4.5.0 to choke.
#include "GetBitContext.h"
#include "common.h"
#include "dsputil.h"
#include "rational.h"

using OpenBlack::Graphics::Texture2D;

namespace OpenBlack {
namespace Video {

#define BIK_SIGNATURE_LEN 4
#define BIK_SIGNATURE_DATA "BIKi"

#define MAX_CHANNELS 2
#define BINK_BLOCK_MAX_SIZE (MAX_CHANNELS << 11)

#if defined(__arm__)
#define SET_INT_TYPE uint8_t
#define SET_INT_VALUE(ptr, value)\
	*(ptr) = (value) && 0xff; \
	(ptr)++; \
	*(ptr) = ((value) >> 8) && 0xff; \
	(ptr)++;
#else
#define SET_INT_TYPE int16_t
#define SET_INT_VALUE(ptr, value)\
	*(ptr)++ = (value);
#endif

#if defined(__arm__)
#define GET_INT_VALUE(value, ptr)\
	(value) = *(ptr); \
	(ptr)++; \
	(value) |= (*(ptr)) << 8; \
	(ptr)++;
#else
#define GET_INT_VALUE(value, ptr)\
	(value) = *reinterpret_cast<int16_t*>(ptr); \
	(ptr) += 2;
#endif

/**
 * IDs for different data types used in Bink video codec
 */
enum Sources {
	BINK_SRC_BLOCK_TYPES = 0, ///< 8x8 block types
	BINK_SRC_SUB_BLOCK_TYPES, ///< 16x16 block types (a subset of 8x8 block types)
	BINK_SRC_COLORS,          ///< pixel values used for different block types
	BINK_SRC_PATTERN,         ///< 8-bit values for 2-colour pattern fill
	BINK_SRC_X_OFF,           ///< X components of motion value
	BINK_SRC_Y_OFF,           ///< Y components of motion value
	BINK_SRC_INTRA_DC,        ///< DC values for intrablocks with DCT
	BINK_SRC_INTER_DC,        ///< DC values for intrablocks with DCT
	BINK_SRC_RUN,             ///< run lengths for special fill block

	BINK_NB_SRC
};

/**
 * Bink video block types
 */
enum BlockTypes {
	SKIP_BLOCK = 0, ///< skipped block
	SCALED_BLOCK,   ///< block has size 16x16
	MOTION_BLOCK,   ///< block is copied from previous frame with some offset
	RUN_BLOCK,      ///< block is composed from runs of colours with custom scan order
	RESIDUE_BLOCK,  ///< motion block with some difference added
	INTRA_BLOCK,    ///< intra DCT block
	FILL_BLOCK,     ///< block is filled with single colour 
	INTER_BLOCK,    ///< motion block with DCT applied to the difference
	PATTERN_BLOCK,  ///< block is filled with two colours following custom pattern
	RAW_BLOCK       ///< uncoded 8x8 block
};

typedef struct AVFrame {
	/**
	 * pointer to the picture planes.
	 * This might be different from the first allocated byte
	 * - encoding:
	 * - decoding:
	 */
	uint8_t *data[3];
	int linesize[3];
} AVFrame;

typedef struct {
	char signature[BIK_SIGNATURE_LEN];
	unsigned int filesize;
	unsigned int framecount;
	unsigned int maxframesize;
	//unsigned int framecount2; //unused by player
	unsigned int width;
	unsigned int height;
	unsigned int fps;
	unsigned int divider;
	unsigned int videoflag;
	unsigned int tracks;  //BinkAudFlags
	//optional if tracks == 1 (original had multiple tracks)
	unsigned short unknown2;
	unsigned short channels;
	unsigned short samplerate;
	unsigned short audioflag;
	unsigned int unknown4;
} binkheader;

typedef struct {
	int     keyframe;
	unsigned int pos;
	unsigned int size;
} binkframe;

typedef struct Bundle {
	int     len;       ///< length of number of entries to decode (in bits)
	Tree    tree;      ///< Huffman tree-related data
	uint8_t *data;     ///< buffer for decoded symbols
	uint8_t *data_end; ///< buffer end
	uint8_t *cur_dec;  ///< pointer to the not yet decoded part of the buffer
	uint8_t *cur_ptr;  ///< pointer to the data that is not read from buffer yet
} Bundle;

class BinkVideo {
private:
	binkheader header;
	unsigned int currentFrame;
public:
	BinkVideo(OSFile* file);
	~BinkVideo();

	bool SetFrame(unsigned int frame);
	void CopyToTexture(Texture2D* texture);

	unsigned int GetFrameCount() { return header.framecount; };
	unsigned int GetFPS() { return header.fps; };
	unsigned int GetWidth() { return header.width; }
	unsigned int GetHeight() { return header.height; }
	unsigned int GetCurrentFrame() { return currentFrame; }
private:
	OSFile* file;

	std::vector<binkframe> frames;
	uint8_t *frameBuffer;

	//video context (consider packing it in a struct)
	AVRational v_timebase;
	//bink specific
	ScanTable c_scantable;
	Bundle c_bundle[BINK_NB_SRC];  ///< bundles for decoding all data types
	Tree c_col_high[16];         ///< trees for decoding high nibble in "colours" data type
	int  c_col_lastval;          ///< value of last decoded high nibble in "colours" data type 

	//huffman trees for video decoding
	VLC bink_trees[16];
	int16_t table[16 * 128][2];
	GetBitContext v_gb;
	AVFrame c_pic, c_last;

private:
	bool readHeader();
	bool nextFrame();
	bool decodeVideoFrame(void *data, int data_size);

	/* ffmpeg bink video decoding functions */
	void ff_init_scantable(ScanTable *st, const uint8_t *src_scantable);
	void initVideoTables();
	void av_set_pts_info(AVRational &time_base, unsigned int pts_num, unsigned int pts_den);
	inline int get_value(int bundle);
	int read_dct_coeffs(DCTELEM block[64], const uint8_t *scan, bool is_intra);
	int read_residue(DCTELEM block[64], int masks_count);
	int read_runs(Bundle *b);
	int read_motion_values(Bundle *b);
	int read_block_types(Bundle *b);
	int read_patterns(Bundle *b);
	int read_colors(Bundle *b);
	int read_dcs(Bundle *b, int start_bits, int has_sign);
	int get_vlc2(int16_t(*table)[2], int bits, int max_depth);
	void read_bundle(int bundle_num);
	void init_lengths(int width, int bw);
};

}
}

#endif
