/*
 * rational numbers
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
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
 * @file libavutil/rational.c
 * rational numbers
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#include <assert.h>
#include <limits.h>

#include "common.h"
#include "rational.h"

static inline int64_t av_gcd(int64_t a, int64_t b){
    if(b) return av_gcd(b, a%b);
    return a;
}

int av_reduce(int &dst_num, int &dst_den, int64_t num, int64_t den, int64_t max) {
    AVRational a0={0,1}, a1={1,0};
    int sign= (num<0) ^ (den<0);
    int64_t gcd= av_gcd(FFABS(num), FFABS(den));

    if(gcd){
        num = FFABS(num)/gcd;
        den = FFABS(den)/gcd;
    }
    if(num<=max && den<=max){
      a1.num = num;
      a1.den = den;      
      den=0;
    }

    while(den){
        uint64_t x      = num / den;
        int64_t next_den= num - den*x;
        int64_t a2n= x*a1.num + a0.num;
        int64_t a2d= x*a1.den + a0.den;

        if(a2n > max || a2d > max){
            if(a1.num) x= (max - a0.num) / a1.num;
            if(a1.den) x= FFMIN(x, (max - a0.den) / a1.den);

            if (den*(2*x*a1.den + a0.den) > num*a1.den) {
                a1.den  =x*a1.den + a0.den;
                a1.num  =x*a1.num + a0.num;
            }
            break;
        }

        a0 = a1;
        a1.den = a2d;
        a1.num = a2n;
        num = den;
        den = next_den;
    }
    assert(av_gcd(a1.num, a1.den) <= 1U);

    dst_num = sign ? -a1.num : a1.num;
    dst_den = a1.den;

    return den==0;
}
