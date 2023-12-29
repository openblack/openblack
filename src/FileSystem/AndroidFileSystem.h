/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#if __ANDROID__

#include <vector>

#include <jni.h>

#include "FileSystemInterface.h"

namespace openblack::filesystem
{

class AndroidFileSystem: public FileSystemInterface
{
public:
	AndroidFileSystem();
	~AndroidFileSystem();

	// Android has a complicated permissions API, must call java code to read contents.
	[[nodiscard]] bool PreferBuffer() const override { return true; }
	[[nodiscard]] std::filesystem::path FindPath(const std::filesystem::path& path) const override;
	std::unique_ptr<Stream> Open(const std::filesystem::path& path, Stream::Mode mode) override;
	bool Exists(const std::filesystem::path& path) const override;
	void SetGamePath(const std::filesystem::path& path) override { _gamePath = path; }
	[[nodiscard]] const std::filesystem::path& GetGamePath() const override { return _gamePath; }
	void AddAdditionalPath(const std::filesystem::path& path) override { _additionalPaths.push_back(path); }
	std::vector<uint8_t> ReadAll(const std::filesystem::path& path) override;
	void Iterate(const std::filesystem::path& path, bool recursive,
	             const std::function<void(const std::filesystem::path&)>& function) const override;

private:
	JNIEnv* _jniEnv;
	jobject _jniActivity;
	jclass _jniInteropClass;
	jmethodID _jniReadFileFromPathMid;
	jmethodID _jniListFilesFromPathMid;

	std::filesystem::path _gamePath;
	std::vector<std::filesystem::path> _additionalPaths;
};

} // namespace openblack::filesystem

#endif
