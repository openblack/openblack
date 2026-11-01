/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CdFileSystem.h"

#include <cctype>
#include <cstring>

#include <algorithm>
#include <array>
#include <sstream>

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
	const auto fileSize = static_cast<size_t>(file->stat->p_stat->size);
	const auto offset = static_cast<size_t>(file->currentOffset);
	if (size == 0 || n == 0 || offset >= fileSize)
	{
		return 0;
	}
	// Only read the blocks covering the requested range; cabinets are read sequentially in small chunks
	const size_t wanted = std::min(size * n, fileSize - offset);
	const auto firstBlock = offset / ISO_BLOCKSIZE;
	const auto blockCount = (offset + wanted - 1) / ISO_BLOCKSIZE - firstBlock + 1;
	thread_local std::vector<uint8_t> blocks;
	blocks.resize(blockCount * ISO_BLOCKSIZE);

	const auto read = static_cast<size_t>(_isoFilesystem.seek_read(
	    blocks.data(), file->stat->p_stat->lsn + static_cast<lsn_t>(firstBlock), static_cast<long>(blockCount)));
	const auto offsetInBlock = offset % ISO_BLOCKSIZE;
	if (read <= offsetInBlock)
	{
		return 0;
	}
	const auto copied = std::min(wanted, read - offsetInBlock);
	std::memcpy(buf, &blocks[offsetInBlock], copied);
	file->currentOffset += static_cast<long>(copied);
	// Like fread, count whole items
	return copied / size;
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
	// Paths are resolved against the cabinet index in ToKey instead
	return path;
}

std::unique_ptr<Stream> CdFileSystem::Open(const std::filesystem::path& path, [[maybe_unused]] Stream::Mode mode)
{
	return std::make_unique<MemoryStream>(ReadAll(path));
}

std::unique_ptr<std::istream> CdFileSystem::GetData(const std::filesystem::path& path)
{
	const auto data = ReadAll(path);
	return std::make_unique<std::istringstream>(std::string(data.begin(), data.end()), std::ios::binary);
}

bool CdFileSystem::IsPathValid(const std::filesystem::path& path)
{
	std::error_code ec;
	return std::filesystem::is_regular_file(path, ec);
}

std::filesystem::path CdFileSystem::GroupInstallDirectory(std::string_view groupName)
{
	// InstallShield file groups and the game directory each one installs to, as on an installed copy of the game.
	// Groups not listed here install to Data/<group name>.
	constexpr std::array<std::pair<std::string_view, std::string_view>, 9> k_GroupDirectories = {{
	    {"Data", "Scripts"},
	    {"TextScript - USA", "Scripts"},
	    {"CreatureCave Html", "Scripts/html"},
	    {"Audio", "Audio"},
	    {"Audio - English", "Audio/Dialogue"},
	    {"Other Graphics", "Data"},
	    {"Font - Europe", "Data"},
	    {"PreIntro", "Data"},
	    {"logo - english", "Data"},
	}};
	for (const auto& [group, directory] : k_GroupDirectories)
	{
		if (groupName == group)
		{
			return directory;
		}
	}
	return std::filesystem::path("Data") / groupName;
}

std::string CdFileSystem::ToKey(const std::filesystem::path& path) const
{
	auto normal = FixPath(path).lexically_normal();
	// Callers may prefix paths with the game path, which here is the image itself
	if (!_isoFilePath.empty())
	{
		const auto relative = normal.lexically_relative(_isoFilePath.lexically_normal());
		if (!relative.empty() && !relative.string().starts_with(".."))
		{
			normal = relative;
		}
	}
	auto key = normal.generic_string();
	while (key.starts_with("./"))
	{
		key.erase(0, 2);
	}
	while (key.ends_with('/'))
	{
		key.pop_back();
	}
	if (key == ".")
	{
		key.clear();
	}
	// Game data comes from Windows, where file names are case-insensitive
	std::ranges::transform(key, key.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return key;
}

[[nodiscard]] bool CdFileSystem::Exists(const std::filesystem::path& path)
{
	const auto key = ToKey(path);
	return _files.contains(key) || _directories.contains(key);
}

void CdFileSystem::SetGamePath(const std::filesystem::path& path)
{
	cdio_log_set_handler(cdio_log_handler);

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

	_files.clear();
	_directories.clear();
	_directoryInstallPaths.clear();
	_dataCab.reset(unshield_open2("data1.hdr", &isoCabCallbacks, this));
	if (!_dataCab)
	{
		throw std::runtime_error("Failed to open data Cabinet File");
	}
	const auto groupCount = unshield_file_group_count(_dataCab.get());
	for (int groupIndex = 0; groupIndex < groupCount; ++groupIndex)
	{
		const auto* group = unshield_file_group_get(_dataCab.get(), groupIndex);
		const auto installDirectory = GroupInstallDirectory(group->name);
		SPDLOG_LOGGER_DEBUG(spdlog::get("filesystem"), "File group \"{}\" installs to \"{}\"", group->name,
		                    installDirectory.generic_string());
		for (auto fileIndex = group->first_file; fileIndex <= group->last_file; ++fileIndex)
		{
			if (unshield_file_is_valid(_dataCab.get(), static_cast<int>(fileIndex)) == 0)
			{
				continue;
			}
			const auto directoryIndex = unshield_file_directory(_dataCab.get(), static_cast<int>(fileIndex));
			const auto installPath = FixPath(installDirectory / unshield_directory_name(_dataCab.get(), directoryIndex) /
			                                 unshield_file_name(_dataCab.get(), static_cast<int>(fileIndex)))
			                             .lexically_normal();
			SPDLOG_LOGGER_TRACE(spdlog::get("filesystem"), "Cab file {}: {}", fileIndex, installPath.generic_string());
			_files.insert({ToKey(installPath), {installPath, static_cast<int>(fileIndex)}});
			for (auto parent = installPath.parent_path(); !parent.empty(); parent = parent.parent_path())
			{
				const auto parentKey = ToKey(parent);
				_directories.insert(parentKey);
				_directoryInstallPaths.emplace(parentKey, parent.generic_string());
			}
		}
	}
	SPDLOG_LOGGER_INFO(spdlog::get("filesystem"), "Indexed {} files from the data cabinet", _files.size());

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
	const auto file = _files.find(ToKey(path));
	if (file == _files.end())
	{
		throw std::runtime_error("File not found: " + path.generic_string());
	}
	const auto index = file->second.index;
	_cabExtractBuffer.clear();
	_cabExtractBuffer.resize(unshield_file_size(_dataCab.get(), index));
	// unshield writes through the IO callbacks, which IsoWrite redirects into _cabExtractBuffer
	if (!unshield_file_save(_dataCab.get(), index, unshield_file_name(_dataCab.get(), index)))
	{
		throw std::runtime_error("Failed to extract file: " + path.generic_string());
	}
	return std::move(_cabExtractBuffer);
}

void CdFileSystem::Iterate(const std::filesystem::path& path, bool recursive,
                           const std::function<void(const std::filesystem::path&)>& function)
{
	const auto key = ToKey(path);
	const auto prefix = key.empty() ? key : key + '/';
	// Keys are the lower case install paths, so the remainder after the prefix can be taken from the install path.
	// Children are reported under the path the caller asked for so they resolve the same way when read.
	const auto visit = [&](std::string_view childKey, const std::string& installPath) {
		if (!childKey.starts_with(prefix) || childKey.size() == prefix.size() ||
		    (!recursive && childKey.find('/', prefix.size()) != std::string_view::npos))
		{
			return;
		}
		function(path / installPath.substr(prefix.size()));
	};
	for (const auto& directoryKey : _directories)
	{
		visit(directoryKey, _directoryInstallPaths.at(directoryKey));
	}
	for (const auto& [fileKey, file] : _files)
	{
		visit(fileKey, file.installPath.generic_string());
	}
}
