/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <concepts>

#include <fstream>

#include <cxxopts.hpp>
#include <json/json.h>

namespace details
{
template <class T>
concept Integer = std::integral<T>;

template <typename T>
concept FloatingPoint = std::floating_point<T>;

template <typename T>
concept String = std::same_as<T, std::string> || std::same_as<T, std::filesystem::path>;
} // namespace details

class StartupConfig
{
private:
	Json::Value config;

public:
	StartupConfig(std::filesystem::path path)
	{
		if (std::filesystem::exists(path))
		{
			std::ifstream o(path.generic_string());
			o >> config;
		}
	}

	template <details::Integer T>
	auto get(const char* key, std::optional<T> fallback = std::nullopt) const
	{
		auto opt = cxxopts::value<T>();
		if (config.isMember(key))
		{
			// Config provided default
			return opt->default_value(std::to_string(config[key].asInt()));
		}
		if (fallback.has_value())
		{
			// Compile-time provided default
			return opt->default_value(std::to_string(*fallback));
		}
		// No default at all
		return opt;
	};

	template <details::FloatingPoint T>
	auto get(const char* key, std::optional<T> fallback = std::nullopt) const
	{
		auto opt = cxxopts::value<T>();
		if (config.isMember(key))
		{
			// Config provided default
			return opt->default_value(std::to_string(config[key].asFloat()));
		}
		if (fallback.has_value())
		{
			// Compile-time provided default
			return opt->default_value(std::to_string(*fallback));
		}
		// No default at all
		return opt;
	};

	template <details::String T>
	auto get(const char* key, std::optional<std::string> fallback = std::nullopt) const
	{
		auto opt = cxxopts::value<T>();
		if (config.isMember(key))
		{
			// Config provided default
			return opt->default_value(config[key].asString());
		}
		if (fallback.has_value())
		{
			// Compile-time provided default
			return opt->default_value(*fallback);
		}
		// No default at all
		return opt;
	};

	auto getStrVec(const char* key, std::string fallback) const
	{
		if (config.isMember(key) && config[key].isArray() && config[key].size() > 0)
		{
			const auto& arr = config[key];
			fallback = arr[0].asString();
			for (std::size_t i = 1; i < arr.size(); i++)
			{
				fallback += std::string(",") + arr.asString();
			}
		}
		return cxxopts::value<std::vector<std::string>>()->default_value(fallback);
	}
};

inline bool hasValue(const cxxopts::OptionValue& val)
{
	return val.has_default() || val.count() > 0;
}
