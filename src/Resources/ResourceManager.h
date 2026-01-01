/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/core/hashed_string.hpp>
#include <entt/fwd.hpp>
#include <entt/resource/cache.hpp>
#include <fmt/format.h>

namespace openblack::resources
{
template <typename T>
[[nodiscard]] constexpr entt::id_type HashIdentifier(const T& identifier)
{
	if constexpr (std::is_same_v<T, std::string>)
	{
		return entt::hashed_string(identifier.c_str());
	}
	else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>)
	{
		return entt::hashed_string(identifier);
	}
	else
	{
		return entt::hashed_string(fmt::format("{}", static_cast<uint32_t>(identifier)).c_str());
	}
}

template <typename ResourceLoader>
class ResourceManager
{
public:
	using ResourceType = typename ResourceLoader::ResourceType;

	template <typename... Args>
	[[maybe_unused]] decltype(auto) Load(entt::id_type identifier, Args&&... args)
	{
		return _resourceCache.load(identifier, std::forward<Args>(args)...);
	}

	template <typename... Args>
	[[maybe_unused]] decltype(auto) Erase(entt::id_type identifier, Args&&... args)
	{
		return _resourceCache.erase(identifier, std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	[[maybe_unused]] decltype(auto) Load(T identifier, Args&&... args)
	{
		return Load(HashIdentifier(identifier), std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	[[maybe_unused]] decltype(auto) Erase(T identifier, Args&&... args)
	{
		return Erase(HashIdentifier(identifier), std::forward<Args>(args)...);
	}

	[[nodiscard]] decltype(auto) Handle(entt::id_type identifier) { return _resourceCache[identifier]; }

	[[nodiscard]] decltype(auto) Handle(entt::id_type identifier) const { return _resourceCache[identifier]; }

	[[nodiscard]] bool Contains(entt::id_type identifier) const { return _resourceCache.contains(identifier); }

	template <typename T>
	[[nodiscard]] bool Contains(T identifier) const
	{
		return Contains(HashIdentifier(identifier));
	}

	template <typename Func>
	void Each(Func func) const
	{
		for (const auto [i, r] : _resourceCache)
		{
			func(i, r);
		}
	}

	[[nodiscard]] entt::resource_cache<ResourceType>& GetCache() const
	{
		return static_cast<entt::resource_cache<ResourceType>&>(_resourceCache);
	}

	[[nodiscard]] decltype(auto) Size() const { return _resourceCache.size(); }

	void Clear() { _resourceCache.clear(); }

private:
	entt::resource_cache<ResourceType, ResourceLoader> _resourceCache;
};
} // namespace openblack::resources
