/*
 * DSP utils
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file libavcodec/dsputil.h
 * DSP utils.
 * note, many functions in here may use MMX which trashes the FPU state, it is
 * absolutely necessary to call emms_c() between dsp & float/double code
 */

#ifndef AVCODEC_DSPUTIL_H
#define AVCODEC_DSPUTIL_H

#include "common.h"

/* dct code */
typedef short DCTELEM;
typedef int DWTELEM;
typedef short IDWTELEM;

/**
 * Scantable.
 */
typedef struct ScanTable{
    const uint8_t *scantable;
    uint8_t permutated[64];
    uint8_t raster_end[64];
} ScanTable;

void ff_init_scantable(uint8_t *, ScanTable *st, const uint8_t *src_scantable);

/**
 * Empty mmx state.
 * this must be called between any dsp function and float/double code.
 * for example sin(); dsp->idct_put(); emms_c(); cos()
 */
#define emms_c()

/* should be defined by architectures supporting
   one or more MultiMedia extension */
int mm_support(void);
extern int mm_flags;

#define DECLARE_ALIGNED_16(t, v) DECLARE_ALIGNED(16, t, v)
#define DECLARE_ALIGNED_8(t, v)  DECLARE_ALIGNED(8, t, v)

#define mm_flags 0
#define mm_support() 0

#ifndef STRIDE_ALIGN
#   define STRIDE_ALIGN 8
#endif

/* FFT computation */

/* NOTE: soon integer code will be added, so you must use the
   FFTSample type */
typedef float FFTSample;

struct MDCTContext;

typedef struct FFTComplex {
    FFTSample re, im;
} FFTComplex;

typedef struct FFTContext {
    int nbits;
    int inverse;
    uint16_t *revtab;
    FFTComplex *exptab;
    FFTComplex *exptab1; /* only used by SSE code */
    FFTComplex *tmp_buf;
    void (*fft_permute)(struct FFTContext *s, FFTComplex *z);
    void (*fft_calc)(struct FFTContext *s, FFTComplex *z);
    int split_radix;
    int permutation;
#define FF_MDCT_PERM_NONE       0
#define FF_MDCT_PERM_INTERLEAVE 1
} FFTContext;

extern FFTSample* const ff_cos_tabs[17];

#define COSTABLE_CONST
#define SINTABLE_CONST

#define COSTABLE(size) COSTABLE_CONST FFTSample ff_cos_##size[size/2]

#define SINTABLE(size) SINTABLE_CONST FFTSample ff_sin_##size[size/2]

extern COSTABLE(16);
extern COSTABLE(32);
extern COSTABLE(64);
extern COSTABLE(128);
extern COSTABLE(256);
extern COSTABLE(512);
extern COSTABLE(1024);
extern COSTABLE(2048);
extern COSTABLE(4096);
extern COSTABLE(8192);
extern COSTABLE(16384);
extern COSTABLE(32768);
extern COSTABLE(65536);

/**
 * Initializes the cosine table in ff_cos_tabs[index]
 * \param index index in ff_cos_tabs array of the table to initialize
 */
void ff_init_ff_cos_tabs(int index);

extern SINTABLE(16);
extern SINTABLE(32);
extern SINTABLE(64);
extern SINTABLE(128);
extern SINTABLE(256);
extern SINTABLE(512);
extern SINTABLE(1024);
extern SINTABLE(2048);
extern SINTABLE(4096);
extern SINTABLE(8192);
extern SINTABLE(16384);
extern SINTABLE(32768);
extern SINTABLE(65536);

/**
 * Sets up a complex FFT.
 * @param nbits           log2 of the length of the input array
 * @param inverse         if 0 perform the forward transform, if 1 perform the inverse
 */
int ff_fft_init(FFTContext *s, int nbits, int inverse);
void ff_fft_permute_c(FFTContext *s, FFTComplex *z);
void ff_fft_calc_c(FFTContext *s, FFTComplex *z);

/**
 * Do the permutation needed BEFORE calling ff_fft_calc().
 */
static inline void ff_fft_permute(FFTContext *s, FFTComplex *z)
{
    s->fft_permute(s, z);
}
/**
 * Do a complex FFT with the parameters defined in ff_fft_init(). The
 * input data must be permuted before. No 1.0/sqrt(n) normalization is done.
 */
static inline void ff_fft_calc(FFTContext *s, FFTComplex *z)
{
    s->fft_calc(s, z);
}
void ff_fft_end(FFTContext *s);

/**
 * Generate a sine window.
 * @param   window  pointer to half window
 * @param   n       size of half window
 */
void ff_sine_window_init(float *window, int n);
extern float ff_sine_128 [ 128];
extern float ff_sine_256 [ 256];
extern float ff_sine_512 [ 512];
extern float ff_sine_1024[1024];
extern float ff_sine_2048[2048];
extern float ff_sine_4096[4096];
extern float * const ff_sine_windows[6];

/* Real Discrete Fourier Transform */

enum RDFTransformType {
    RDFT,
    IRDFT,
    RIDFT,
    IRIDFT
};

typedef struct {
    int nbits;
    int inverse;
    int sign_convention;

    /* pre/post rotation tables */
    FFTSample *tcos;
    FFTSample *tsin;
    FFTContext fft;
} RDFTContext;

/**
 * Sets up a real FFT.
 * @param nbits           log2 of the length of the input array
 * @param trans           the type of transform
 */
int ff_rdft_init(RDFTContext *s, int nbits, enum RDFTransformType trans);
void ff_rdft_calc(RDFTContext *s, FFTSample *data);
void ff_rdft_end(RDFTContext *s);

/* Discrete Cosine Transform */

typedef struct {
    int nbits;
    int inverse;
    FFTComplex *data;
    FFTContext fft;
} DCTContext;

/**
 * Sets up (Inverse)DCT.
 * @param nbits           log2 of the length of the input array
 * @param inverse         >0 forward transform, <0 inverse transform
 */
int ff_dct_init(DCTContext *s, int nbits, int inverse);
void ff_dct_calc(DCTContext *s, FFTSample *data);
void ff_dct_end(DCTContext *s);

#endif /* AVCODEC_DSPUTIL_H */
