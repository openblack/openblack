#pragma once
#ifndef OPENBLACK_VIDEO_VIDEOPLAYER_H
#define OPENBLACK_VIDEO_VIDEOPLAYER_H

#include <stdint.h>
#include <memory>
#include <string>

#include <Common/OSFile.h>
#include <Graphics/Texture2D.h>
#include <Video/Bink.h>

namespace OpenBlack
{
	namespace Video
	{
		/** VideoPlayer based on LHVideoPlayer, used to play Bink movies */
		class VideoPlayer {
		public:
			VideoPlayer(const std::string& file);
			~VideoPlayer();

			void SetFrame(int frame);
			int CurrentFrame() const;

			/** Is the video currently playing? */
			bool IsPlaying() const;

			/** Returns the width of the video's frames. */
			uint32_t GetWidth() const;

			/** Returns the height of the video's frames. */
			uint32_t GetHeight() const;

		private:
			std::unique_ptr<OSFile> _file;
			std::unique_ptr<Bink> _bink;
			std::shared_ptr<Graphics::Texture2D> _texture;

			// Create a surface for video of these dimensions.
			void createTexture();
		};
	}
}

#endif
