/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/core/hashed_string.hpp>
#include <entt/entt.hpp>
#include <fmt/format.h>

#include "MeshId.h"

namespace openblack::resources
{

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
		entt::id_type id = entt::hashed_string(fmt::format("{}", identifier).c_str());
		return Load(id, std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	[[maybe_unused]] decltype(auto) Erase(T identifier, Args&&... args)
	{
		entt::id_type id = entt::hashed_string(fmt::format("{}", identifier).c_str());
		return Erase(id, std::forward<Args>(args)...);
	}

	[[nodiscard]] decltype(auto) Handle(entt::id_type identifier) { return _resourceCache[identifier]; }

	[[nodiscard]] decltype(auto) Handle(entt::id_type identifier) const { return _resourceCache[identifier]; }

	[[nodiscard]] bool Contains(entt::id_type identifier) const { return _resourceCache.contains(identifier); }

	template <typename T>
	[[nodiscard]] bool Contains(T identifier) const
	{
		entt::id_type id = entt::hashed_string(fmt::format("{}", identifier).c_str());
		return Contains(id);
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
