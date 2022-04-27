/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <mutex>
#include <random>

namespace openblack
{
class RNGManager
{
public:
	static RNGManager& instance();
	template <typename T, typename E = typename std::enable_if<std::is_arithmetic_v<T>>::type>
	T NextValue(T min, T max)
	{
		using dist_t =
		    std::conditional_t<std::is_integral_v<T>, std::uniform_int_distribution<T>, std::uniform_real_distribution<T>>;
		dist_t dist(min, max);
		if (_debugRng)
		{
			std::lock_guard<std::mutex> safe_lock(_generatorLock);
			return dist(_generator);
		}
		thread_local std::mt19937 generator;
		return dist(generator);
	}
	bool SetDebugMode(bool is_debug, int seed);
	RNGManager(const RNGManager&) = delete;
	RNGManager& operator=(const RNGManager&) = delete;

private:
	std::mt19937 _generator;
	std::mutex _generatorLock;
	bool _debugRng;
	RNGManager();
};
} // namespace openblack
