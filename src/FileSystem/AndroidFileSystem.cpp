/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#if __ANDROID__

#include "AndroidFileSystem.h"

#include <cctype>
#include <cstddef>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include "MemoryStream.h"

using namespace openblack::filesystem;

namespace
{
// Adapted from https://stackoverflow.com/a/13059195/10604387
//          and https://stackoverflow.com/a/46069245/10604387
struct membuf: std::streambuf
{
	membuf(char const* base, size_t size)
	{
		char* p(const_cast<char*>(base));
		this->setg(p, p, p + size);
	}
	std::streampos seekoff(off_type off, std::ios_base::seekdir way, [[maybe_unused]] std::ios_base::openmode which) override
	{
		if (way == std::ios_base::cur)
		{
			gbump(static_cast<int>(off));
		}
		else if (way == std::ios_base::end)
		{
			setg(eback(), egptr() + off, egptr());
		}
		else if (way == std::ios_base::beg)
		{
			setg(eback(), eback() + off, egptr());
		}
		return gptr() - eback();
	}

	std::streampos seekpos([[maybe_unused]] pos_type pos, [[maybe_unused]] std::ios_base::openmode which) override
	{
		return seekoff(pos - static_cast<off_type>(0), std::ios_base::beg, which);
	}
};
struct imemstream: virtual membuf, std::istream
{
	imemstream(std::vector<uint8_t> data)
	    : membuf(reinterpret_cast<const char*>(data.data()), data.size())
	    , std::istream(dynamic_cast<std::streambuf*>(this))
	    , _data(data)
	{
		auto p = reinterpret_cast<char*>(_data.data());
		this->setg(p, p, p + data.size());
	}

	imemstream(char const* base, size_t size)
	    : membuf(base, size)
	    , std::istream(dynamic_cast<std::streambuf*>(this))
	{
	}

private:
	std::vector<uint8_t> _data;
};

JNIEnv* Env()
{
	return static_cast<JNIEnv*>(SDL_GetAndroidJNIEnv());
}

// Frees a JNI local reference on scope exit, since native threads never return to Java to free them
template <typename T>
class LocalRef
{
public:
	LocalRef(JNIEnv* env, T ref)
	    : _env(env)
	    , _ref(ref)
	{
	}
	~LocalRef()
	{
		if (_ref != nullptr)
		{
			_env->DeleteLocalRef(_ref);
		}
	}
	LocalRef(const LocalRef&) = delete;
	LocalRef& operator=(const LocalRef&) = delete;
	[[nodiscard]] T get() const { return _ref; }

private:
	JNIEnv* _env;
	T _ref;
};

bool ClearException(JNIEnv* env)
{
	if (env->ExceptionCheck() == JNI_FALSE)
	{
		return false;
	}
	env->ExceptionDescribe();
	env->ExceptionClear();
	return true;
}
} // namespace

AndroidFileSystem::AndroidFileSystem()
{
	auto* env = Env();
	auto* activity = static_cast<jobject>(SDL_GetAndroidActivity());
	_jniActivity = env->NewGlobalRef(activity);
	env->DeleteLocalRef(activity);
	auto* interopClass = env->FindClass("org/openblack/app/FileSystemInterop");
	_jniInteropClass = static_cast<jclass>(env->NewGlobalRef(interopClass));
	env->DeleteLocalRef(interopClass);

	_jniGetDirectoryFromPathMid =
	    env->GetStaticMethodID(_jniInteropClass, "getDirectoryFromPath",
	                           "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Landroid/net/Uri;");
	_jniExistsFromPathMid = env->GetStaticMethodID(_jniInteropClass, "existsFromPath",
	                                               "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Z");
	_jniReadFileFromPathMid = env->GetStaticMethodID(_jniInteropClass, "readFileFromPath",
	                                                 "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)[B");
	_jniListFilesFromPathMid =
	    env->GetStaticMethodID(_jniInteropClass, "listFilesFromPath",
	                           "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;Z)[Ljava/lang/String;");
}

AndroidFileSystem::~AndroidFileSystem()
{
	auto* env = Env();
	env->DeleteGlobalRef(_jniInteropClass);
	env->DeleteGlobalRef(_jniActivity);
}

std::filesystem::path AndroidFileSystem::FindPath(const std::filesystem::path& path) const
{
	if (path.empty())
	{
		throw std::invalid_argument("empty path");
	}
	if (!Exists(path))
	{
		throw std::runtime_error("File " + path.string() + " not found");
	}
	return path;
}

bool AndroidFileSystem::IsPathValid(const std::filesystem::path& path)
{
	auto* env = Env();
	const LocalRef jgamePath {env, env->NewStringUTF(path.c_str())};
	const LocalRef jpath {env, env->NewStringUTF("/")};
	const LocalRef juri {env, env->CallStaticObjectMethod(_jniInteropClass, _jniGetDirectoryFromPathMid, _jniActivity,
	                                                      jgamePath.get(), jpath.get())};
	return !ClearException(env) && juri.get() != nullptr;
}

std::unique_ptr<Stream> AndroidFileSystem::Open(const std::filesystem::path& path, [[maybe_unused]] Stream::Mode mode)
{
	auto* env = Env();
	const LocalRef jgamePath {env, env->NewStringUTF(_gamePath.c_str())};
	const LocalRef jpath {env, env->NewStringUTF(path.c_str())};
	const LocalRef jbytes {env, static_cast<jbyteArray>(env->CallStaticObjectMethod(
	                                _jniInteropClass, _jniReadFileFromPathMid, _jniActivity, jgamePath.get(), jpath.get()))};
	if (ClearException(env) || jbytes.get() == nullptr)
	{
		throw std::runtime_error("Could not read file " + path.string());
	}

	std::vector<uint8_t> bytes(static_cast<size_t>(env->GetArrayLength(jbytes.get())));
	env->GetByteArrayRegion(jbytes.get(), 0, static_cast<jsize>(bytes.size()), reinterpret_cast<jbyte*>(bytes.data()));
	return std::make_unique<MemoryStream>(std::move(bytes));
}

bool AndroidFileSystem::Exists(const std::filesystem::path& path) const
{
	if (path.empty())
	{
		return false;
	}
	auto* env = Env();
	const LocalRef jgamePath {env, env->NewStringUTF(_gamePath.c_str())};
	const LocalRef jpath {env, env->NewStringUTF(path.c_str())};
	const bool exists = env->CallStaticBooleanMethod(_jniInteropClass, _jniExistsFromPathMid, _jniActivity, jgamePath.get(),
	                                                 jpath.get()) != JNI_FALSE;
	return !ClearException(env) && exists;
}

std::vector<uint8_t> AndroidFileSystem::ReadAll(const std::filesystem::path& path)
{
	auto file = Open(path, Stream::Mode::Read);
	std::size_t size = file->Size();

	std::vector<uint8_t> data(size);
	file->Read(data.data(), size);

	return data;
}

void AndroidFileSystem::Iterate(const std::filesystem::path& path, bool recursive,
                                const std::function<void(const std::filesystem::path&)>& function) const
{
	auto* env = Env();
	const LocalRef jgamePath {env, env->NewStringUTF(_gamePath.c_str())};
	const LocalRef jpath {env, env->NewStringUTF(path.c_str())};
	const LocalRef jfilePaths {env, static_cast<jobjectArray>(env->CallStaticObjectMethod(
	                                    _jniInteropClass, _jniListFilesFromPathMid, _jniActivity, jgamePath.get(), jpath.get(),
	                                    static_cast<jboolean>(recursive)))};
	if (ClearException(env) || jfilePaths.get() == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Could not list directory {}", path.string());
		return;
	}

	// Copy the names out first so the callback can make its own JNI calls
	std::vector<std::filesystem::path> filePaths(static_cast<size_t>(env->GetArrayLength(jfilePaths.get())));
	for (size_t i = 0; i < filePaths.size(); ++i)
	{
		const LocalRef filePath {env,
		                         static_cast<jstring>(env->GetObjectArrayElement(jfilePaths.get(), static_cast<jsize>(i)))};
		const char* rawString = env->GetStringUTFChars(filePath.get(), nullptr);
		filePaths[i] = path / rawString;
		env->ReleaseStringUTFChars(filePath.get(), rawString);
	}
	for (const auto& filePath : filePaths)
	{
		function(filePath);
	}
}

std::unique_ptr<std::istream> AndroidFileSystem::GetData(const std::filesystem::path& path)
{
	return std::make_unique<imemstream>(ReadAll(path));
}

#endif
