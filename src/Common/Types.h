#pragma once
#ifndef OPENBLACK_TYPES_H
#define OPENBLACK_TYPES_H

#include <stdint.h>
#include <string>

#include <glm/glm.hpp>

typedef struct rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb_t;

typedef struct rgba {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
} rgba_t;

#endif
