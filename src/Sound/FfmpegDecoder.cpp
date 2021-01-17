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

#include <fstream>
#include <functional>

#define BUFFER_SIZE 8192

namespace openblack::audio
{

int readSoundBytes(void* opaque, uint8_t* buffer, int bufferSize)
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

	// In normal FFempeg usage, the sound file path is given to the API where its opened and read
	// We cannot do that as the file does not exist
	auto avFormatCtx =
	    std::unique_ptr<AVFormatContext, decltype(&AvDeleteFormatContext)>(avformat_alloc_context(), AvDeleteFormatContext);

	auto avBufferFree = [&](uint8_t* buffer) { av_free(buffer); };
	auto avBuffer =
	    std::unique_ptr<uint8_t, decltype(avBufferFree)>(static_cast<uint8_t*>(av_malloc(BUFFER_SIZE)), avBufferFree);

	auto memoryStream = MemoryStream(soundBytes.data(), soundBytes.size());
	auto avioCtx = std::unique_ptr<AVIOContext, decltype(&AvDeleteAvioContext)>(
	    avio_alloc_context(avBuffer.get(), BUFFER_SIZE, 0, &memoryStream, readSoundBytes, nullptr, nullptr),
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
	avcodec_open2(codecContext, decoder, nullptr);

	// This is our input
	auto inChannels = codecContext->channel_layout == 0 ? AV_CH_LAYOUT_MONO : codecContext->channel_layout;
	auto inFormat = codecContext->sample_fmt;
	auto inSampleRate = codecContext->sample_rate;

	// This is our result. We need it as signed 16-bits non-planar for our audio players
	auto outChannels = codecContext->channel_layout == 0 ? AV_CH_LAYOUT_MONO : codecContext->channel_layout;
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
		avcodec_send_packet(codecContext, packet.get());
		avcodec_receive_frame(codecContext, frame.get());

		if (frame->nb_samples == 0)
			continue;

		auto outSamplesCount = swr_get_out_samples(swr.get(), frame->nb_samples);
		auto outBufferSize = av_samples_get_buffer_size(nullptr, frame->channels, outSamplesCount, outFormat, 0);
		auto outBuffer = std::vector<uint8_t>(outBufferSize);
		auto rawOutBuffer = outBuffer.data();
		memset(rawOutBuffer, 0, outBufferSize);
		swr_convert(swr.get(), &rawOutBuffer, outSamplesCount, (uint8_t const**)frame->data, frame->nb_samples);
		duration += frame->pkt_duration;

		if (frame->data[0] == nullptr)
		{
			throw std::runtime_error("No data in audio frame");
		}

		std::copy(rawOutBuffer, rawOutBuffer + outBufferSize, std::back_inserter(loaded));
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
