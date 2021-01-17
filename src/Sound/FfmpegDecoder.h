/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

#include "AudioDecoder.h"

#include <memory>

namespace openblack::audio
{
class FfmpegDecoder: public AudioDecoder
{
public:
	std::string GetName() const override { return "FFmpeg Loader"; };
	void ToPCM16(Sound& sound) override;

private:
	static void AvDeleteFormatContext(AVFormatContext* context);
	static void AvDeleteAvioContext(AVIOContext* context);
	static void SwrDeleteContext(SwrContext* context);
};
} // namespace openblack::audio