/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <concepts>

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <cxxopts.hpp>
#include <nlohmann/json.hpp>

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
	nlohmann::json _config;

public:
	explicit StartupConfig(std::filesystem::path path)
	{
		if (std::filesystem::exists(path))
		{
			std::ifstream o(path.generic_string());
			o >> _config;
		}
	}

	template <details::Integer T>
	auto Get(const char* key, std::optional<T> fallback = std::nullopt) const
	{
		auto opt = cxxopts::value<T>();
		if (_config.contains(key))
		{
			// Config provided default
			return opt->default_value(std::to_string(_config[key].get<T>()));
		}
		if (fallback.has_value())
		{
			// Compile-time provided default
			return opt->default_value(std::to_string(*fallback));
		}
		// No default at all
		return opt;
	}

	template <details::FloatingPoint T>
	auto Get(const char* key, std::optional<T> fallback = std::nullopt) const
	{
		auto opt = cxxopts::value<T>();
		if (_config.contains(key))
		{
			// Config provided default
			return opt->default_value(std::to_string(_config[key].get<T>()));
		}
		if (fallback.has_value())
		{
			// Compile-time provided default
			return opt->default_value(std::to_string(*fallback));
		}
		// No default at all
		return opt;
	}

	template <details::String T>
	auto Get(const char* key, std::optional<std::string> fallback = std::nullopt) const
	{
		auto opt = cxxopts::value<T>();
		if (_config.contains(key))
		{
			// Config provided default
			return opt->default_value(_config[key].get<std::string>());
		}
		if (fallback.has_value())
		{
			// Compile-time provided default
			return opt->default_value(*fallback);
		}
		// No default at all
		return opt;
	}

	auto GetStrVec(const char* key, std::string fallback) const
	{
		if (_config.contains(key) && _config[key].is_array() && !_config[key].empty())
		{
			const auto& arr = _config[key];
			fallback = arr[0].get<std::string>();
			for (std::size_t i = 1; i < arr.size(); i++)
			{
				fallback += std::string(",") + arr[i].get<std::string>();
			}
		}
		return cxxopts::value<std::vector<std::string>>()->default_value(fallback);
	}
};

inline bool hasValue(const cxxopts::OptionValue& val)
{
	return val.has_default() || val.count() > 0;
}