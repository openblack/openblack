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
