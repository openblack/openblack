/*
 * (I)DCT Transforms
 * Copyright (c) 2009 Peter Ross (pross@xvid.org)
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file libavcodec/dct.c
 * (Inverse) Discrete Cosine Transforms
 */

#define _USE_MATH_DEFINES

#include <math.h>
#include "dsputil.h"

av_cold int ff_dct_init(DCTContext *s, int nbits, int inverse)
{
    int n = 1 << nbits;

    s->nbits    = nbits;
    s->inverse  = inverse;

    s->data = (struct FFTComplex *) av_malloc(sizeof(FFTComplex) * 2 * n);
    if (!s->data)
        return -1;

    if (ff_fft_init(&s->fft, nbits+1, inverse) < 0)
        return -1;

    return 0;
}

static void ff_dct_calc_c(DCTContext *s, FFTSample *data)
{
    int n = 1<<s->nbits;
    int i;

#define ROTATE(i,n) (-M_PI*((n)-0.5f)*(i)/(n))
    if (s->inverse) {
        for(i=0; i < n; i++) {
            s->data[i].re = (float) (2 * data[i] * cos(ROTATE(i,n)));
            s->data[i].im = (float) (2 * data[i] * sin(ROTATE(i,n)));
        }
        s->data[n].re = 0;
        s->data[n].im = 0;
        for(i=0; i<n-1; i++) {
            s->data[n+i+1].re = (float) (-2 * data[n - (i+1)] * cos(ROTATE(n+i+1,n)));
            s->data[n+i+1].im = (float) (-2 * data[n - (i+1)] * sin(ROTATE(n+i+1,n)));
        }
    }else{
        for(i=0; i < n; i++) {
            s->data[i].re = data[n - (i+1)];
            s->data[i].im = 0;
            s->data[n+i].re = data[i];
            s->data[n+i].im = 0;
        }
    }

    ff_fft_permute(&s->fft, s->data);
    ff_fft_calc(&s->fft, s->data);

    if (s->inverse) {
        for(i=0; i < n; i++)
            data[i] = s->data[n-(i+1)].re / (2 * n);
    }else {
        for(i=0; i < n; i++)
            data[i] =  (float) (s->data[i].re / (2 * cos(ROTATE(i,n))));
    }
#undef ROTATE
}

void ff_dct_calc(DCTContext *s, FFTSample *data)
{
    ff_dct_calc_c(s, data);
}

av_cold void ff_dct_end(DCTContext *s)
{
    ff_fft_end(&s->fft);
    av_freep((void **) &s->data);
}
