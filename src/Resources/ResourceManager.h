/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <entt/core/hashed_string.hpp>
#include <entt/entt.hpp>
#include <fmt/format.h>

#include "MeshId.h"

namespace openblack::resources
{

template <typename ResourceLoader, typename ResourceType = typename ResourceLoader::ResourceType>
class ResourceManager
{
public:
	template <typename... Args>
	[[maybe_unused]] decltype(auto) Load(entt::id_type identifier, Args&&... args)
	{
		return _resourceCache.template load<ResourceLoader>(identifier, std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	[[maybe_unused]] decltype(auto) Load(T identifier, Args&&... args)
	{
		entt::id_type id = entt::hashed_string(fmt::format("{}", identifier).c_str());
		return Load(id, std::forward<Args>(args)...);
	}

	[[nodiscard]] entt::resource_handle<ResourceType> Handle(entt::id_type identifier) const
	{
		return _resourceCache.handle(identifier);
	}

	template <typename T>
	[[nodiscard]] entt::resource_handle<ResourceType> Handle(T identifier) const
	{
		entt::id_type id = entt::hashed_string(fmt::format("{}", identifier).c_str());
		return Handle(id);
	}

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
		_resourceCache.each(func);
	}

	[[nodiscard]] entt::resource_cache<ResourceType>& GetCache() const { return _resourceCache; }

	[[nodiscard]] decltype(auto) Size() const { return _resourceCache.size(); }

	void Clear() { _resourceCache.clear(); }

private:
	entt::resource_cache<ResourceType> _resourceCache;
};
} // namespace openblack::resources
