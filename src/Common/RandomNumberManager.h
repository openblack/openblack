/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <random>

namespace openblack
{
class RandomNumberManagerInterface
{
public:
	template <typename T>
	requires (std::is_arithmetic_v<T>)
	T NextValue(T min, T max)
	{
		using dist_t =
		    std::conditional_t<std::is_integral_v<T>, std::uniform_int_distribution<T>, std::uniform_real_distribution<T>>;
		dist_t dist(min, max);
		std::optional<std::reference_wrapper<std::mutex>> lock(LockAccess());
		if (lock)
		{
			std::lock_guard<std::mutex> const contextLock(*lock);
			return dist(Generator());
		}
		return dist(Generator());
	}

	template <typename T>
	    requires((std::is_same_v<T, std::array<typename T::value_type, std::tuple_size<T>::value>> &&
	              std::tuple_size<T>::value > 1) ||
	             std::is_same_v<T, std::vector<typename T::value_type>>)
	auto Choose(const T& container) -> const decltype(container[0])&
	{
		const auto size = container.size();
		if constexpr (std::is_same_v<T, std::vector<typename T::value_type>>)
		{
			assert(size > 0);
		}
#if __has_cpp_attribute(assume)
		[[assume(size > 0)]];
#endif

		const auto index = NextValue<std::size_t>(0, size - 1);
		return container[index];
	}

	virtual ~RandomNumberManagerInterface() = default;

protected:
	virtual std::mt19937& Generator() = 0;
	virtual std::optional<std::reference_wrapper<std::mutex>> LockAccess() = 0;
};
} // namespace openblack
