#include <LH3D/LHOSFile.h>

#include <stdexcept>
#include <sstream>
#include <cassert>

#if FILE_API == FILE_API_POSIX
LHOSFile::LHOSFile()
{
	mHandle = -1;
}

LHOSFile::~LHOSFile()
{
	if (mHandle != -1)
		close(mHandle);
}

void LHOSFile::Open(char const *filename, LH_FILE_MODE mode)
{
	assert(mHandle == -1);

	int handle = -1;
	if (mode == LH_FILE_MODE::Read)
		handle = open(filename, O_RDONLY);
	else if (mode == LH_FILE_MODE::Create)
		handle = open(filename, O_CREAT | O_EXCL | O_RDWR);

	if (handle == -1)
	{
		std::ostringstream os;
		os << "Failed to open '" << filename << "' for reading.";
		throw std::runtime_error(os.str());
	}

	mHandle = handle;
}

void LHOSFile::Close()
{
	assert(mHandle != -1);

	close(mHandle);
	mHandle = -1;
}

size_t LHOSFile::Read(void * data, size_t size)
{
	assert(mHandle != -1);

	ssize_t readb;

	readb = read(mHandle, data, size);
	if (readb == -1)
		throw std::runtime_error("A read operation on a file failed.");

	return (size_t)read;
}

void LHOSFile::Seek(size_t position, LH_SEEK_MODE mode)
{
	assert(mHandle != -1);

	int seekMode;
	switch (mode)
	{
	case LH_SEEK_MODE::Set:
		seekMode = SEEK_SET;
		break;
	case LH_SEEK_MODE::Current:
		seekMode = SEEK_CUR;
		break;
	case LH_SEEK_MODE::End:
		seekMode = SEEK_END;
		break;
	default:
		throw std::runtime_error("Invalid seek mode.");
		break;
	}

	if (lseek(mHandle, position, seekMode) == -1)
		throw std::runtime_error("A seek operation on a file failed.");
}

size_t LHOSFile::Position()
{
	assert(mHandle != -1);

	off_t value = lseek(mHandle, 0, SEEK_CUR);

	if (value == -1)
		throw std::runtime_error("A query operation on a file failed.");

	return value;
}

size_t LHOSFile::Size()
{
	assert(mHandle != -1);

 struct stat info;
 if(fstat(mHandle, &info) != 0)
		throw std::runtime_error("A query operation on a file failed.");

	// should be fine, no files are over 4GB..
	if (info.st_size > SIZE_MAX)
		throw std::runtime_error("Files greater that 4GB are not supported.");

	return info.st_size;
}

/* static */

bool LHOSFile::Delete(const char * filename)
{
	return !unlink(filename);
}

bool LHOSFile::Rename(const char * srcfile, const char * dstfile)
{
	return !rename(srcfile, dstfile);
}

bool LHOSFile::Exists(const char * filename)
{
	return !access(filename, F_OK);
}
#endif

#if FILE_API == FILE_API_WIN32
LHOSFile::LHOSFile()
{
	mHandle = INVALID_HANDLE_VALUE;
}

LHOSFile::~LHOSFile()
{
	if (mHandle != INVALID_HANDLE_VALUE)
		CloseHandle(mHandle);
}

void LHOSFile::Open(char const *filename, LH_FILE_MODE mode)
{
	assert(mHandle == INVALID_HANDLE_VALUE);

	HANDLE handle = INVALID_HANDLE_VALUE;
	if (mode == LH_FILE_MODE::Read)
		handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	else if (mode == LH_FILE_MODE::Create)
		handle = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);

	if (handle == INVALID_HANDLE_VALUE)
	{
		std::ostringstream os;
		os << "Failed to open '" << filename << "' for reading.";
		throw std::runtime_error(os.str());
	}

	mHandle = handle;
}

void LHOSFile::Close()
{
	assert(mHandle != INVALID_HANDLE_VALUE);

	CloseHandle(mHandle);
	mHandle = INVALID_HANDLE_VALUE;
}

size_t LHOSFile::Read(void * data, size_t size)
{
	assert(mHandle != INVALID_HANDLE_VALUE);

	DWORD read;

	if (!ReadFile(mHandle, data, size, &read, NULL))
		throw std::runtime_error("A read operation on a file failed.");

	return read;
}

void LHOSFile::Seek(size_t position, LH_SEEK_MODE mode)
{
	assert(mHandle != INVALID_HANDLE_VALUE);

	int seekMode;
	switch (mode)
	{
	case LH_SEEK_MODE::Set:
		seekMode = SEEK_SET;
		break;
	case LH_SEEK_MODE::Current:
		seekMode = SEEK_CUR;
		break;
	case LH_SEEK_MODE::End:
		seekMode = SEEK_END;
		break;
	default:
		throw std::runtime_error("Invalid seek mode.");
		break;
	}

	if (SetFilePointer(mHandle, position, NULL, seekMode) == INVALID_SET_FILE_POINTER)
		if (GetLastError() != NO_ERROR)
			throw std::runtime_error("A seek operation on a file failed.");
}

size_t LHOSFile::Position()
{
	assert(mHandle != INVALID_HANDLE_VALUE);

	DWORD value = SetFilePointer(mHandle, 0, NULL, SEEK_CUR);

	if (value == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		throw std::runtime_error("A query operation on a file failed.");

	return value;
}

size_t LHOSFile::Size()
{
	assert(mHandle != INVALID_HANDLE_VALUE);

	BY_HANDLE_FILE_INFORMATION info;

	if (!GetFileInformationByHandle(mHandle, &info))
		throw std::runtime_error("A query operation on a file failed.");

	// should be fine, no files are over 4GB..
	if (info.nFileSizeHigh != 0)
		throw std::runtime_error("Files greater that 4GB are not supported.");

	return info.nFileSizeLow;
}

/* static */

bool LHOSFile::Delete(const char * filename)
{
	return DeleteFileA(filename);
}

bool LHOSFile::Rename(const char * srcfile, const char * dstfile)
{
	return MoveFileA(srcfile, dstfile);
}

bool LHOSFile::Exists(const char * filename)
{
	DWORD result = GetFileAttributesA(filename);
	DWORD error = GetLastError();
	return !(result == INVALID_FILE_ATTRIBUTES && (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND));
}

#endif

char * LHOSFile::ReadAll(const char * filename, size_t* sizeOut)
{
    LHOSFile* file = new LHOSFile;
    file->Open(filename, LH_FILE_MODE::Read);

    size_t fileSize = file->Size();

    char* buffer = (char*)malloc(fileSize+1);

    file->Read(buffer, fileSize);

    file->Close();

    if (sizeOut != 0)
        *sizeOut = fileSize+1;

    buffer[fileSize] = 0;

	return buffer;
}
