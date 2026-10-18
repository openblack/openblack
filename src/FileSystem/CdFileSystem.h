/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <cdio++/iso9660.hpp>

#include "FileSystemInterface.h"

struct dirent;
typedef struct _Unshield Unshield;

namespace
{
struct IsoEmbeddedFile;
struct IsoCabDir;
} // namespace

namespace openblack::filesystem
{

class CdFileSystem: public FileSystemInterface
{
	class UnshieldDeleter
	{
	public:
		void operator()(Unshield* unshield) const;
	};

public:
	[[nodiscard]] std::filesystem::path FindPath(const std::filesystem::path& path) const override;
	std::unique_ptr<std::istream> GetData(const std::filesystem::path& path) override;
	[[nodiscard]] bool IsPathValid(const std::filesystem::path& path) override;
	std::unique_ptr<Stream> Open(const std::filesystem::path& path, Stream::Mode mode) override;
	[[nodiscard]] bool Exists(const std::filesystem::path& path) override;
	void SetGamePath(const std::filesystem::path& path) override;
	[[nodiscard]] const std::filesystem::path& GetGamePath() const override;
	void AddAdditionalPath(const std::filesystem::path& path) override;
	std::vector<uint8_t> ReadAll(const std::filesystem::path& path) override;
	void Iterate(const std::filesystem::path& path, bool recursive,
	             const std::function<void(const std::filesystem::path&)>& function) override;

	IsoEmbeddedFile* IsoOpen(const std::filesystem::path& path, const char* modes);
	int IsoSeek(IsoEmbeddedFile* file, long int offset, int whence);
	long int IsoTell(IsoEmbeddedFile* file);
	size_t IsoRead(uint8_t* buf, size_t size, size_t n, IsoEmbeddedFile* file);
	size_t IsoWrite(const void* ptr, size_t size, size_t n, IsoEmbeddedFile* file);
	int IsoClose(IsoEmbeddedFile* file);
	IsoCabDir* IsoOpenDir(const std::filesystem::path& path);
	int IsoCloseDir(IsoCabDir* dir);
	struct dirent* IsoReadDir(IsoCabDir* dir);

private:
	struct CabFile
	{
		std::filesystem::path installPath;
		int index;
	};

	[[nodiscard]] static std::filesystem::path GroupInstallDirectory(std::string_view groupName);
	/// Lower case path relative to the game directory, used to look files up in the cabinet index
	[[nodiscard]] std::string ToKey(const std::filesystem::path& path) const;

	std::filesystem::path _isoFilePath;
	ISO9660::IFS _isoFilesystem;
	std::unique_ptr<Unshield, UnshieldDeleter> _dataCab;
	std::unordered_map<std::string, CabFile> _files;
	std::unordered_set<std::string> _directories;
	std::unordered_map<std::string, std::string> _directoryInstallPaths;
	std::vector<uint8_t> _cabExtractBuffer;
};

} // namespace openblack::filesystem
