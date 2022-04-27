/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#include <gtest/gtest.h>

#include <Common/RNGManager.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#define N_TEST 100000
#define TEST_SEED 15000
#define N_THREAD 10


TEST(RngManager, debug_predictable_mono)
{
    openblack::RNGManager& rngManager = openblack::RNGManager::instance();
    ASSERT_TRUE(rngManager.SetDebugMode(true, TEST_SEED));

    std::vector<uint32_t> fstResult(N_TEST);
    for (auto it=fstResult.begin(); it!=fstResult.end(); ++it)
    {
        *it = rngManager.NextValue(1,500);
    }

    // Two run with the same seed mush give the same vector.
    ASSERT_TRUE(rngManager.SetDebugMode(true, TEST_SEED));
    std::vector<uint32_t> sndResult(N_TEST);
    for (auto it=sndResult.begin(); it!=sndResult.end(); ++it)
    {
        *it = rngManager.NextValue(1,500);
    }
    ASSERT_TRUE(fstResult==sndResult);
}

void randomThread(int totalIteration, int& counter, std::vector<uint32_t>& result, std::mutex& generatorLock, std::mutex& writeLock) 
{
    openblack::RNGManager& rngManager = openblack::RNGManager::instance();
    int current(0);
    uint32_t random(0);

    for (int i = 0; i < totalIteration; i++)
    {
        {
            std::lock_guard<std::mutex> generatorLocked(generatorLock);
            random = rngManager.NextValue(1,500);
            current = counter;
            counter++;
        }
        
        {
            std::lock_guard<std::mutex> locked(writeLock);
            result[current] = random;
        }
    }
};

TEST(RngManager, debug_predictable_multi)
{
    openblack::RNGManager& rngManager = openblack::RNGManager::instance();
    ASSERT_TRUE(rngManager.SetDebugMode(true, TEST_SEED));

    std::vector<uint32_t> fstResult(N_TEST);
    for (auto it=fstResult.begin(); it!=fstResult.end(); ++it)
    {
        *it = rngManager.NextValue(1,500);
    }

    /*
     * TODO must guarentee that with multiple thread using RNGManager in debug
     * mode will use the same sequence of number.
     */
    
    ASSERT_TRUE(rngManager.SetDebugMode(true, TEST_SEED));
    int index = 0;
    int itemPerThread = N_TEST / N_THREAD;
    std::vector<uint32_t> sndResult(N_TEST);
    std::vector<std::thread> threadVector;
    std::mutex generatorLock, writeLock;
    for (int i = 0; i < N_THREAD; i++)
    {
        auto l = [itemPerThread, &index, &sndResult, &generatorLock, &writeLock] { randomThread(itemPerThread, index, sndResult, generatorLock, writeLock); };
        threadVector.push_back(std::thread(l));
    }
    for (auto t = threadVector.begin(); t != threadVector.end(); ++t)
    {
        t ->join();
    }

    ASSERT_TRUE(fstResult==sndResult);
}
