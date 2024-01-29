/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CdFileSystem.h"

#include <cdio/logging.h>
#include <dirent.h>
#include <libunshield.h>
#include <spdlog/spdlog.h>

#include "MemoryStream.h"

using namespace openblack::filesystem;

namespace
{

enum IsoEmbeddedFileMode : uint8_t
{
	ISO_EMBEDDED_FILE_MODE_BINARY = 1 << 0,
	ISO_EMBEDDED_FILE_MODE_READ = 1 << 1,
	ISO_EMBEDDED_FILE_MODE_WRITE = 1 << 2,
	ISO_EMBEDDED_FILE_MODE_AMEND = 1 << 3,
};

struct IsoEmbeddedFile
{
	std::unique_ptr<ISO9660::Stat> stat;
	uint8_t mode;
	long int currentOffset;
};

struct IsoCabDir
{
	ISO9660::IFS::stat_vector_t vector;
	stat_vector_iterator_t currentIterator;
	dirent currentDirent;
};

void* iso_open(const char* filename, const char* modes, void* userdata)
{
	auto* file = reinterpret_cast<CdFileSystem*>(userdata)->IsoOpen(filename, modes);
	return reinterpret_cast<void*>(file);
}

int iso_seek(void* file, long int offset, int whence, void* userdata)
{
	return reinterpret_cast<CdFileSystem*>(userdata)->IsoSeek(reinterpret_cast<IsoEmbeddedFile*>(file), offset, whence);
}

long int iso_tell(void* file, void* userdata)
{
	return reinterpret_cast<CdFileSystem*>(userdata)->IsoTell(reinterpret_cast<IsoEmbeddedFile*>(file));
}

size_t iso_read(void* ptr, size_t size, size_t n, void* file, void* userdata)
{
	return reinterpret_cast<CdFileSystem*>(userdata)->IsoRead(reinterpret_cast<uint8_t*>(ptr), size, n,
	                                                          reinterpret_cast<IsoEmbeddedFile*>(file));
}

size_t iso_write(const void* ptr, size_t size, size_t n, void* file, void* userdata)
{
	return reinterpret_cast<CdFileSystem*>(userdata)->IsoWrite(ptr, size, n, reinterpret_cast<IsoEmbeddedFile*>(file));
}

int iso_close(void* file, void* userdata)
{
	return reinterpret_cast<CdFileSystem*>(userdata)->IsoClose(reinterpret_cast<IsoEmbeddedFile*>(file));
}

void* iso_opendir(const char* name, void* userdata)
{
	auto* dir = reinterpret_cast<CdFileSystem*>(userdata)->IsoOpenDir(name);
	return reinterpret_cast<void*>(dir);
}

int iso_closedir(void* dir, void* userdata)
{
	return reinterpret_cast<CdFileSystem*>(userdata)->IsoCloseDir(reinterpret_cast<IsoCabDir*>(dir));
}

dirent* iso_readdir(void* dir, void* userdata)
{
	return reinterpret_cast<CdFileSystem*>(userdata)->IsoReadDir(reinterpret_cast<IsoCabDir*>(dir));
}

static UnshieldIoCallbacks isoCabCallbacks = {
    .fopen = iso_open,
    .fseek = iso_seek,
    .ftell = iso_tell,
    .fread = iso_read,
    .fwrite = iso_write,
    .fclose = iso_close,
    .opendir = iso_opendir,
    .closedir = iso_closedir,
    .readdir = iso_readdir,
};
} // namespace

void CdFileSystem::UnshieldDeleter::operator()(Unshield* unshield) const
{
	if (unshield)
	{
		unshield_close(unshield);
	}
}

void cdio_log_handler(cdio_log_level_t level, const char message[])
{
	spdlog::level::level_enum spdlogLevel;
	switch (level)
	{
	case CDIO_LOG_DEBUG:
		spdlogLevel = spdlog::level::debug;
		break;
	case CDIO_LOG_INFO:
		spdlogLevel = spdlog::level::info;
		break;
	case CDIO_LOG_WARN:
		spdlogLevel = spdlog::level::warn;
		break;
	case CDIO_LOG_ERROR:
		spdlogLevel = spdlog::level::err;
		break;
	case CDIO_LOG_ASSERT:
		spdlogLevel = spdlog::level::critical;
		break;
	}
	SPDLOG_LOGGER_CALL(spdlog::get("filesystem"), spdlogLevel, "cdio: {}", message);
}

void unshield_log_handler([[maybe_unused]] void* userdata, int level, const char* file, int line, const char* format, va_list args)
{
	spdlog::level::level_enum spdlogLevel;
	switch (level)
	{
	case UNSHIELD_LOG_LEVEL_TRACE:
		spdlogLevel = spdlog::level::trace;
		break;
	case UNSHIELD_LOG_LEVEL_WARNING:
		spdlogLevel = spdlog::level::warn;
		break;
	case UNSHIELD_LOG_LEVEL_ERROR:
		spdlogLevel = spdlog::level::err;
		break;
	}

	// Get the length of the message
	va_list args_copy;
	va_copy(args_copy, args);
	int length = vsnprintf(nullptr, 0, format, args_copy) + 1;
	va_end(args_copy);

	std::vector<char> message(length);
	vsnprintf(message.data(), message.size(), format, args);

	SPDLOG_LOGGER_CALL(spdlog::get("filesystem"), spdlogLevel, "unshield: {}", message.data());
}

IsoEmbeddedFile* CdFileSystem::IsoOpen(const std::filesystem::path& path, const char* modes)
{
	SPDLOG_LOGGER_TRACE(spdlog::get("filesystem"), "Opening {} with mode {}", path.c_str(), modes);
	auto file = std::make_unique<IsoEmbeddedFile>();
	file->mode = static_cast<IsoEmbeddedFileMode>(0);
	if (strchr(modes, 'b') != nullptr)
	{
		file->mode |= ISO_EMBEDDED_FILE_MODE_BINARY;
	}
	if (strchr(modes, 'r') != nullptr)
	{
		file->mode |= ISO_EMBEDDED_FILE_MODE_READ;
	}
	if (strchr(modes, 'w') != nullptr)
	{
		file->mode |= ISO_EMBEDDED_FILE_MODE_WRITE;
	}
	if (strchr(modes, 'a') != nullptr)
	{
		file->mode |= ISO_EMBEDDED_FILE_MODE_AMEND;
	}

	file->currentOffset = 0;

	if ((file->mode & ISO_EMBEDDED_FILE_MODE_READ) != 0)
	{
		file->stat.reset(_isoFilesystem.stat(path.c_str()));
		if (!file->stat->p_stat)
		{
			return nullptr;
		}
	}

	return file.release();
}

int CdFileSystem::IsoSeek(IsoEmbeddedFile* file, long int offset, int whence)
{
	switch (whence)
	{
	case SEEK_SET:
		file->currentOffset = offset;
		break;
	case SEEK_CUR:
		file->currentOffset += offset;
		break;
	case SEEK_END:
		file->currentOffset = file->stat->p_stat->size - offset;
		break;
	default:
		return -1;
	}
	return 0;
}

long int CdFileSystem::IsoTell(IsoEmbeddedFile* file)
{
	return file->currentOffset;
}

size_t CdFileSystem::IsoRead(uint8_t* buf, size_t size, size_t n, IsoEmbeddedFile* file)
{
	uint32_t whole_blocks = (file->currentOffset + size * n) / ISO_BLOCKSIZE;
	bool has_extra_block = (file->currentOffset + size * n) % ISO_BLOCKSIZE != 0;
	const auto newSize = ISO_BLOCKSIZE * (whole_blocks + has_extra_block);
	thread_local std::vector<uint8_t> buf_extra_blocks;
	if (buf_extra_blocks.size() < newSize)
		buf_extra_blocks.resize(newSize);

	const size_t read =
	    _isoFilesystem.seek_read(buf_extra_blocks.data(), file->stat->p_stat->lsn, whole_blocks + has_extra_block);
	if (read > static_cast<size_t>(file->currentOffset))
	{
		const size_t usefulRead = std::min(read - file->currentOffset, size * n);
		memcpy(buf, &buf_extra_blocks[file->currentOffset], usefulRead);
		file->currentOffset += usefulRead;
		return usefulRead;
	}

	return 0;
}

size_t CdFileSystem::IsoWrite(const void* ptr, size_t size, size_t n, IsoEmbeddedFile* file)
{
	assert(_cabExtractBuffer.size() >= file->currentOffset + size * n);
	memcpy(&_cabExtractBuffer[file->currentOffset], ptr, size * n);
	file->currentOffset += size * n;
	return size * n;
}

int CdFileSystem::IsoClose(IsoEmbeddedFile* file)
{
	delete file;
	return 0;
}

IsoCabDir* CdFileSystem::IsoOpenDir(const std::filesystem::path& path)
{
	auto dir = std::make_unique<IsoCabDir>();
	if (!_isoFilesystem.readdir(path.c_str(), dir->vector))
	{
		return nullptr;
	}
	dir->currentIterator = dir->vector.begin();
	return dir.release();
}

int CdFileSystem::IsoCloseDir(IsoCabDir* dir)
{
	for (auto* stat : dir->vector)
	{
		delete stat;
	}
	delete dir;
	return 0;
}

dirent* CdFileSystem::IsoReadDir(IsoCabDir* dir)
{
	if (dir->currentIterator == dir->vector.end())
	{
		return nullptr;
	}

	auto* stat = *dir->currentIterator;
	++dir->currentIterator;

	memset(&dir->currentDirent, 0, sizeof(dir->currentDirent));
	dir->currentDirent.d_ino = stat->p_stat->lsn;
	//	dir->currentDirent.d_off = stat->p_stat->lsn;
	dir->currentDirent.d_reclen = stat->p_stat->size;
	switch (stat->p_stat->type)
	{
	case 1: //_STAT_FILE:
		dir->currentDirent.d_type = DT_REG;
		break;
	case 2: //_STAT_DIR:
		dir->currentDirent.d_type = DT_DIR;
		break;
	default:
		dir->currentDirent.d_type = DT_UNKNOWN;
		break;
	}
	std::array<char, PATH_MAX> filename;
	iso9660_name_translate(stat->p_stat->filename, filename.data());
	strncpy(dir->currentDirent.d_name, filename.data(), sizeof(dir->currentDirent.d_name));

	SPDLOG_LOGGER_TRACE(spdlog::get("filesystem"), "Reading directory: {}", dir->currentDirent.d_name);

	return &dir->currentDirent;
}

[[nodiscard]] std::filesystem::path CdFileSystem::FindPath(const std::filesystem::path& path) const
{
	const auto scriptsRelative = std::filesystem::relative(path, GetPath<filesystem::Path::Scripts>());
	if (!scriptsRelative.string().starts_with(".."))
	{
		return GetPath<filesystem::Path::Data>() / "Data" / scriptsRelative;
	}
	return path;
}

std::unique_ptr<Stream> CdFileSystem::Open(const std::filesystem::path& path, Stream::Mode mode)
{
	return std::make_unique<MemoryStream>(ReadAll(path));
}

[[nodiscard]] bool CdFileSystem::Exists(const std::filesystem::path& path)
{
	const auto fixedPath = FindPath(path);

	auto isoRelativePath = std::filesystem::relative(fixedPath, _isoFilePath);
	if (isoRelativePath.empty() || isoRelativePath.string().starts_with(".."))
	{
		isoRelativePath = fixedPath;
	}

	const auto cabPathRoot = GetPath<filesystem::Path::Data>();
	auto cabRelativePath = std::filesystem::relative(isoRelativePath, cabPathRoot);
	if (cabRelativePath.empty() || cabRelativePath.string().starts_with(".."))
	{
		auto stat = std::unique_ptr<ISO9660::Stat>(_isoFilesystem.stat(isoRelativePath.c_str()));
		return stat && stat->p_stat != nullptr;
	}
	else
	{
		return _dataCabPathToIndexMap.contains(cabRelativePath.string());
	}

	return false;
}

void CdFileSystem::SetGamePath(const std::filesystem::path& path)
{
	cdio_log_set_handler(cdio_log_handler);
	unshield_set_log_handler(unshield_log_handler, nullptr);

	if (!_isoFilesystem.open(path.c_str()))
	{
		throw std::runtime_error("Failed to open iso");
	}

	const std::unique_ptr<ISO9660::PVD> pvd(_isoFilesystem.read_pvd());
	if (pvd)
	{
		auto trim = [](const char* str, size_t size) {
			std::string result(str, size);
			result.erase(std::find_if(result.rbegin(), result.rend(), [](int ch) { return std::isspace(ch) == 0; }).base(),
			             result.end());
			return result;
		};
		SPDLOG_LOGGER_INFO(spdlog::get("filesystem"), "Application: {}",
		                   trim(pvd->pvd.application_id, sizeof(pvd->pvd.application_id)));
		SPDLOG_LOGGER_INFO(spdlog::get("filesystem"), "Preparer: {}", trim(pvd->pvd.preparer_id, sizeof(pvd->pvd.preparer_id)));
		SPDLOG_LOGGER_INFO(spdlog::get("filesystem"), "Publisher: {}",
		                   trim(pvd->pvd.publisher_id, sizeof(pvd->pvd.publisher_id)));
		SPDLOG_LOGGER_INFO(spdlog::get("filesystem"), "System: {}", trim(pvd->pvd.system_id, sizeof(pvd->pvd.system_id)));
		SPDLOG_LOGGER_INFO(spdlog::get("filesystem"), "Volume: {}", trim(pvd->pvd.volume_id, sizeof(pvd->pvd.volume_id)));
		SPDLOG_LOGGER_INFO(spdlog::get("filesystem"), "Volume Set: {}",
		                   trim(pvd->pvd.volume_set_id, sizeof(pvd->pvd.volume_set_id)));
	}

	_dataCabPathToIndexMap.clear();
	_dataCab.reset(unshield_open2("data1.hdr", &isoCabCallbacks, this));
	if (!_dataCab)
	{
		throw std::runtime_error("Failed to open data Cabinet File");
	}
	uint32_t groupCount = unshield_file_group_count(_dataCab.get());
	for (uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
	{
		auto group = unshield_file_group_get(_dataCab.get(), groupIndex);
		auto fileCount = group->last_file - group->first_file + 1;
		SPDLOG_LOGGER_INFO(spdlog::get("filesystem"), "File group: {} with {} files",
		                   unshield_file_group_name(_dataCab.get(), groupIndex), fileCount);
		for (uint32_t j = 0; j < fileCount; ++j)
		{
			const auto groupDirectoryName = std::filesystem::path(group->name);
			const auto fileIndex = group->first_file + j;
			const auto directoryIndex = unshield_file_directory(_dataCab.get(), fileIndex);
			const auto directoryName = std::filesystem::path(unshield_directory_name(_dataCab.get(), directoryIndex));
			const auto filename = std::filesystem::path(unshield_file_name(_dataCab.get(), fileIndex));
			const auto filepath = FixPath(groupDirectoryName / directoryName / filename);
			SPDLOG_LOGGER_INFO(spdlog::get("filesystem"), "File: {} with index {}", filepath.c_str(), fileIndex);
			_dataCabPathToIndexMap.insert({filepath, fileIndex});
		}
	}

	// TODO: stat the path
	_isoFilePath = path;
}

[[nodiscard]] const std::filesystem::path& CdFileSystem::GetGamePath() const
{
	return _isoFilePath;
}

void CdFileSystem::AddAdditionalPath(const std::filesystem::path& path) {}

std::vector<uint8_t> CdFileSystem::ReadAll(const std::filesystem::path& path)
{
	const auto fixedPath = FindPath(path);
	if (!Exists(fixedPath))
	{
		throw std::runtime_error("File not found");
	}
	auto isoRelativePath = std::filesystem::relative(fixedPath, _isoFilePath);
	if (isoRelativePath.empty() || isoRelativePath.string().starts_with(".."))
	{
		isoRelativePath = fixedPath;
	}

	const auto cabPathRoot = GetPath<filesystem::Path::Data>();
	auto cabRelativePath = std::filesystem::relative(isoRelativePath, cabPathRoot);
	if (cabRelativePath.empty() || cabRelativePath.string().starts_with(".."))
	{
		auto stat = std::unique_ptr<ISO9660::Stat>(_isoFilesystem.stat(isoRelativePath.c_str()));
		if (!stat || stat->p_stat == nullptr)
		{
			throw std::runtime_error("File not found");
		}
		stat.reset();
		assert(false); // TODO: implement
	}
	else
	{
		int index = _dataCabPathToIndexMap.at(cabRelativePath.string());
		auto filename = unshield_file_name(_dataCab.get(), index);
		auto size = unshield_file_size(_dataCab.get(), index);
		_cabExtractBuffer.clear();
		_cabExtractBuffer.resize(size);
		if (!unshield_file_save(_dataCab.get(), index, filename))
		{
			throw std::runtime_error("Failed to extract file");
		}
		// unshield_file_save_old()
		return std::move(_cabExtractBuffer);
	}

	return {};
}

void CdFileSystem::Iterate(const std::filesystem::path& path, bool recursive,
                           const std::function<void(const std::filesystem::path&)>& function)
{
	const auto cabPathRoot = GetPath<filesystem::Path::Data>();
	auto cabRelativePath = std::filesystem::relative(path, cabPathRoot);
	if (cabRelativePath.empty() || cabRelativePath.string().starts_with(".."))
	{
		auto* dirIter = IsoOpenDir(path);
		if (dirIter == nullptr)
		{
			return;
		}
		for (dirent* entity = IsoReadDir(dirIter); entity != nullptr; entity = IsoReadDir(dirIter))
		{
			const auto currentPath = std::filesystem::path((*dirIter->currentIterator)->p_stat->filename);
			const auto relativeToPath = std::filesystem::relative(currentPath, path);
			const auto relativeParent = relativeToPath.parent_path();
			if (!recursive && !relativeParent.empty())
			{
				continue;
			}
			function(currentPath);
		}
	}
	else
	{
		for (auto& [cachedPath, _] : _dataCabPathToIndexMap)
		{
			const auto relativeToPath = std::filesystem::relative(cachedPath, cabRelativePath);
			// if the path is not a child of the current path, skip it (starts with ..)
			if (relativeToPath.empty() || relativeToPath.string().starts_with(".."))
			{
				continue;
			}
			const auto relativeParent = relativeToPath.parent_path();
			if (!recursive && !relativeParent.empty())
			{
				continue;
			}
			function(cabPathRoot / cachedPath);
		}
	}
}
