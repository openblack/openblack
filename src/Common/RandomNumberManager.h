/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#pragma once

#include <mutex>
#include <optional>
#include <random>

namespace openblack
{
class RandomNumberManagerInterface
{
public:
	template <typename T, typename E = typename std::enable_if<std::is_arithmetic_v<T>>::type>
	T NextValue(T min, T max)
	{
		using dist_t =
		    std::conditional_t<std::is_integral_v<T>, std::uniform_int_distribution<T>, std::uniform_real_distribution<T>>;
		dist_t dist(min, max);
		std::optional<std::reference_wrapper<std::mutex>> lock(lockAccess());
		if (lock)
		{
			std::lock_guard<std::mutex> contextLock(*lock);
			return dist(generator());
		}
		return dist(generator());
	}
	virtual ~RandomNumberManagerInterface() = default;

protected:
	virtual std::mt19937& generator() = 0;
	virtual std::optional<std::reference_wrapper<std::mutex>> lockAccess() = 0;
	virtual bool lockCheck() = 0;
};
} // namespace openblack
