/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#ifndef GIT_SHA1
#define GIT_SHA1 "UNDEFINED_GIT_SHA1"
#warning "Git sha-1 undefined. Make sure to set it properly"
#endif

static const char* kGitSHA1Hash = GIT_SHA1;
