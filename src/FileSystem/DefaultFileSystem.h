/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <iosfwd>
#include <vector>

#include "FileSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp"
#endif

namespace openblack::filesystem
{

class DefaultFileSystem: public FileSystemInterface
{
public:
	[[nodiscard]] std::filesystem::path FindPath(const std::filesystem::path& path) const override;
	[[nodiscard]] bool IsPathValid(const std::filesystem::path& path) override;
	std::unique_ptr<Stream> Open(const std::filesystem::path& path, Stream::Mode mode) override;
	std::unique_ptr<std::istream> GetData(const std::filesystem::path& path) override;
	[[nodiscard]] bool Exists(const std::filesystem::path& path) const override;
	void SetGamePath(const std::filesystem::path& path) override;
	[[nodiscard]] const std::filesystem::path& GetGamePath() const override { return _gamePath; }
	void AddAdditionalPath(const std::filesystem::path& path) override { _additionalPaths.push_back(path); }
	std::vector<uint8_t> ReadAll(const std::filesystem::path& path) override;
	void Iterate(const std::filesystem::path& path, bool recursive,
	             const std::function<void(const std::filesystem::path&)>& function) const override;

private:
	std::filesystem::path _gamePath;
	std::vector<std::filesystem::path> _additionalPaths;
};

} // namespace openblack::filesystem
