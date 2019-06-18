/* OpenBlack - A reimplementation of Lionhead's Black & White.
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
#ifndef OPENBLACK_OSFILE_H
#define OPENBLACK_OSFILE_H

#define FILE_API_STDIO  0
#define FILE_API_POSIX  1
#define FILE_API_WIN32 2

#if defined(__linux) || defined(__unix) || defined(__posix)
#define FILE_API    FILE_API_POSIX
#elif defined(_WIN32)
#define FILE_API    FILE_API_WIN32
#else
#define FILE_API    FILE_API_STDIO
#endif

#if FILE_API == FILE_API_STDIO
#include <cstdio>
#elif FILE_API == FILE_API_POSIX
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#elif FILE_API == FILE_API_WIN32
#include <windows.h>
#else
#error Unsupported File API
#endif
#include <cstddef>
#include <cstring>
#include <string>

#include <Common/LHSegment.h>

namespace OpenBlack
{
	enum LH_FILE_MODE {
		Create,
		Write,
		Read,
	};

	enum LH_SEEK_MODE {
		Set,
		Current,
		End
	};

	class OSFile {
	public:
		OSFile();
		~OSFile();

		void Open(char const* filename, LH_FILE_MODE filemode); // lh: bool return & checks exists
		void Close();

		size_t Read(void* data, size_t size); // lh: Read((void *, ulong, ulong *))
		std::string ReadString();
		// LHOSFile::Write((void *, ulong, ulong *))

		void Seek(size_t position, LH_SEEK_MODE); // lh: void Seek(long, int, unsigned long*);
		size_t Position(); // lh: seek
		size_t Size(); // lh: void Length(*ulong);

		// Lionhead specific methods:

		bool GetSegment(const char* segment, OpenBlack::LHSegment* segmentOut, bool allocateMemory);

		static bool Delete(const char* filename);
		static bool Rename(const char* filename_before, const char* filename_after);
		static bool Exists(const char* filename);

		static char* ReadAll(const char* filename, size_t* size);

		// ConvertDirInfo((LHDir *))
		//int DirFindFirst(char * filename, LHDir *, ulong);
		//int DirFindNext(LHDir*);
		//int DirFindEnd(LHDir*);
	private:
#if FILE_API == FILE_API_STDIO
		FILE* mHandle;
#elif FILE_API == FILE_API_POSIX
		int mHandle;
#elif FILE_API == FILE_API_WIN32
		HANDLE mHandle;
#endif
	};

}

#endif
