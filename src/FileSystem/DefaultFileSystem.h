/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <vector>
#include <expected>

#include "FileSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#warning "Locator interface implementations should only be included in Locator.cpp"
#endif

namespace openblack::filesystem
{

class DefaultFileSystem: public FileSystemInterface
{
public:
	[[nodiscard]] auto FindPath(const std::filesystem::path& path) const->std::expected<std::filesystem::path, std::invalid_argument> override;
	[[nodiscard]] bool IsPathValid(const std::filesystem::path& path) override;
	auto Open(const std::filesystem::path& path, Stream::Mode mode)
	    -> std::expected<std::unique_ptr<Stream>, std::invalid_argument> override;
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
