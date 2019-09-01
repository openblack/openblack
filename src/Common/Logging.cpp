#include <filesystem>

#include <Common/FileSystem.h>
#include <Common/Logging.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> OpenBlack::Logging::CreateLogger(std::string sourceFilePath)
{
	return spdlog::stdout_color_mt<spdlog::async_factory>(std::filesystem::path(sourceFilePath).filename().generic_string());
}
