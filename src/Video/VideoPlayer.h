/* OpenBlack - A reimplementation of Lionheads's Black & White engine
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

#pragma once
#ifndef OPENBLACK_VIDEO_VIDEOPLAYER_H
#define OPENBLACK_VIDEO_VIDEOPLAYER_H

#include <stdint.h>
#include <memory>
#include <string>

#include <Common/OSFile.h>
#include <Graphics/Texture2D.h>
// #include <Video/Bink.h>

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
			//std::unique_ptr<Bink> _bink;
			std::shared_ptr<Graphics::Texture2D> _texture;

			// Create a surface for video of these dimensions.
			void createTexture();
		};
	}
}

#endif
