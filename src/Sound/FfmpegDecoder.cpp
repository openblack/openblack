/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "FfmpegDecoder.h"
#include "Common/MemoryStream.h"

#include <spdlog/spdlog.h>
#include <array>
#include <fstream>
#include <functional>

#define BUFFER_SIZE 8192

namespace openblack::audio
{

std::string GetErrorCode(int errorCode)
{
	std::array<char, 1024> buffer;

	if (av_strerror(errorCode, buffer.data(), buffer.size()) < 0)
	{
		return std::string("Unknown error code: ") + std::to_string(errorCode);
	}

	return std::string(buffer.begin(), buffer.end());
}


int ReadSoundBytes(void* opaque, uint8_t* buffer, int bufferSize)
{
	auto stream = reinterpret_cast<MemoryStream*>(opaque);
	memset(buffer, 0, bufferSize);

	if (stream->Position() + 1 == stream->Size())
		return AVERROR_EOF;

	stream->Read(reinterpret_cast<char*>(buffer), bufferSize);
	return bufferSize;
}

void FfmpegDecoder::ToPCM16(Sound& sound)
{
	auto& soundBytes = sound.bytes;
	int result;

	// In normal FFempeg usage, the sound file path is given to the API where its opened and read
	// We cannot do that as the file does not exist
	auto avFormatCtx =
	    std::unique_ptr<AVFormatContext, decltype(&AvDeleteFormatContext)>(avformat_alloc_context(), AvDeleteFormatContext);

	auto avBufferFree = [&](uint8_t* buffer) { av_free(buffer); };
	auto avBuffer =
	    std::unique_ptr<uint8_t, decltype(avBufferFree)>(static_cast<uint8_t*>(av_malloc(BUFFER_SIZE)), avBufferFree);

	auto memoryStream = MemoryStream(soundBytes.data(), soundBytes.size());
	auto avioCtx = std::unique_ptr<AVIOContext, decltype(&AvDeleteAvioContext)>(
	    avio_alloc_context(avBuffer.get(), BUFFER_SIZE, 0, &memoryStream, ReadSoundBytes, nullptr, nullptr),
	    AvDeleteAvioContext);

	// AVIO context holds a reference to the buffer. We can now discard this one
	avBuffer.release();
	avFormatCtx->pb = avioCtx.get();
	avFormatCtx->max_analyze_duration = 0;
	AVFormatContext* rawAvFormatCtx = avFormatCtx.get();

	// This call will free the context on failure
	auto error = avformat_open_input(&rawAvFormatCtx, sound.name.c_str(), nullptr, nullptr);
	if (rawAvFormatCtx == nullptr || error < 0)
	{
		throw std::runtime_error("Cannot open audio data");
	}

	error = avformat_find_stream_info(avFormatCtx.get(), nullptr);
	if (error < 0)
		throw std::runtime_error("Cannot find stream info");

	auto streamId = av_find_best_stream(avFormatCtx.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	auto stream = avFormatCtx->streams[streamId];
	auto codecContext = avcodec_alloc_context3(nullptr);
	avcodec_parameters_to_context(codecContext, stream->codecpar);
	auto decoder = avcodec_find_decoder(codecContext->codec_id);
	if ((result = avcodec_open2(codecContext, decoder, nullptr)) < 0)
	{
		throw std::runtime_error(GetErrorCode(result));
	}

	// This is our input
	auto inChannels = codecContext->channel_layout == 0 ? av_get_default_channel_layout(codecContext->channels) : codecContext->channel_layout;
	auto inFormat = codecContext->sample_fmt;
	auto inSampleRate = codecContext->sample_rate;

	// This is our result. We need it as signed 16-bits non-planar for our audio players
	auto outChannels = inChannels;
	auto outFormat = AV_SAMPLE_FMT_S16;
	auto outSampleRate = codecContext->sample_rate;

	auto swr = std::unique_ptr<SwrContext, decltype(&SwrDeleteContext)>(
	    swr_alloc_set_opts(nullptr, outChannels, outFormat, outSampleRate, inChannels, inFormat, inSampleRate, 0, nullptr),
	    SwrDeleteContext);
	swr_init(swr.get());

	auto packetDeleter = [](AVPacket* packet) { av_packet_free(&packet); };
	auto packet = std::unique_ptr<AVPacket, decltype(packetDeleter)>(av_packet_alloc(), packetDeleter);
	auto frameDeleter = [](AVFrame* frame) { av_frame_free(&frame); };
	auto frame = std::unique_ptr<AVFrame, decltype(frameDeleter)>(av_frame_alloc(), frameDeleter);
	auto loaded = std::vector<uint8_t>();
	double timeBase = av_q2d(stream->time_base);
	double duration = 0;

	while (av_read_frame(avFormatCtx.get(), packet.get()) >= 0)
	{
		if ((result = avcodec_send_packet(codecContext, packet.get())) < 0)
		{
			if (result == AVERROR_INVALIDDATA)
			{
				spdlog::warn("Invalid data found for " + sound.name);
				continue;
			}

			auto i = GetErrorCode(result);
			throw std::runtime_error("FFMPEG error: " + i);
		}

		while((result = avcodec_receive_frame(codecContext, frame.get())) == 0 || result == -11)
		{
			if (result < 0 && result != AVERROR(EAGAIN) && result != AVERROR_EOF)
			{
				throw std::runtime_error("FFMPEG error: " + GetErrorCode(result));
			} else if (result < 0)
			{
				// FFmpeg is telling us to feed it more packets
				break;
			}

			if (frame->channels <= 0 || frame->channels > 2 || frame->sample_rate <= 0)
			{
				throw std::runtime_error("Invalid audio properties detected");
			}

			auto outBuffer = std::vector<uint8_t>(8192 * 2 * 8);
			auto *out = outBuffer.data();

			if ((result = swr_convert(swr.get(), &out, outBuffer.size() / (2 * frame->channels), (const uint8_t**)frame->extended_data, frame->nb_samples)) < 0)
			{
				throw std::runtime_error("FFMPEG error: " + GetErrorCode(result));
			}

			if (frame->data[0] == nullptr)
			{
				throw std::runtime_error("No data in audio frame");
			}

			std::copy(outBuffer.begin(), outBuffer.begin() + (result * 2 * frame->channels), std::back_inserter(loaded));
		}

		if (result < 0 && result != AVERROR(EAGAIN))
		{
			throw std::runtime_error("FFMPEG error: " + GetErrorCode(result));
		}

		duration += frame->pkt_duration;
	}

	sound.bytes = loaded;
	sound.lengthInSeconds = duration * timeBase;

	switch (outChannels)
	{
	case AV_CH_LAYOUT_MONO:
		sound.channelLayout = ChannelLayout::Mono;
		break;
	case AV_CH_LAYOUT_STEREO:
		sound.channelLayout = ChannelLayout::Stereo;
		break;
	default:
		throw std::runtime_error("Unsupported channel layout for sound \"" + sound.name + "\"");
	}
}

void FfmpegDecoder::AvDeleteFormatContext(AVFormatContext* context)
{
	avformat_close_input(&context);
}

void FfmpegDecoder::AvDeleteAvioContext(AVIOContext* context)
{
	av_free(context->buffer);
	avio_context_free(&context);
}

void FfmpegDecoder::SwrDeleteContext(SwrContext* context)
{
	swr_free(&context);
}
} // namespace openblack::audio
