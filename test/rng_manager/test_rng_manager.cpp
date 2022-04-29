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
#include <mutex>
#include <thread>
#include <vector>

#define N_TEST 1000000
#define TEST_SEED 15000
#define N_THREAD 10

using namespace openblack;

template <class Manager>
struct TestFunctions
{
template <typename T>
static void DebugTest(T min, T max, unsigned int nTest, int seed, unsigned int nThread = 1)
{
    Manager rngManager;
    ASSERT_TRUE(rngManager.SetDebugMode(true, seed));

    std::vector<T> fstResult(nTest);
    for (auto it=fstResult.begin(); it!=fstResult.end(); ++it)
    {
        *it = rngManager.NextValue(1,500);
    }

    /*
     * TODO must guarentee that with multiple thread using RNGManager in debug
     * mode will use the same sequence of number.
     */
    
    ASSERT_TRUE(rngManager.SetDebugMode(true, seed));
    int index = 0;
    int itemPerThread = nTest / nThread;
    std::vector<T> sndResult(nTest);
    std::vector<std::thread> threadVector;
    std::mutex generatorLock, writeLock;
    for (int i = 0; i < nThread; i++)
    {
        auto l = [itemPerThread, &index, &sndResult, &generatorLock, &writeLock, &rngManager] { randomThread(itemPerThread, index, sndResult, generatorLock, writeLock, rngManager); };
        threadVector.push_back(std::thread(l));
    }
    for (auto t = threadVector.begin(); t != threadVector.end(); ++t)
    {
        t ->join();
    }

    ASSERT_TRUE(fstResult==sndResult);
}
template <typename T>
static void randomThread(int totalIteration, int& counter, std::vector<T>& result, std::mutex& generatorLock, std::mutex& writeLock, Manager& rngManager)
{
    int current(0);
    T random(0);

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
}
static void DebugTestSeries(unsigned int nTest, int seed, unsigned int nThread = 1)
{
    int min(1), max(500);

    // Test serie for Intergers
    DebugTest<unsigned short int>(min, max, nTest, seed, nThread);
    DebugTest<unsigned int>(min, max, nTest, seed, nThread);
    DebugTest<unsigned long int>(min, max, nTest, seed, nThread);
    DebugTest<short int>(min, max, nTest, seed, nThread);
    DebugTest<int>(min, max, nTest, seed, nThread);
    DebugTest<long int>(min, max, nTest, seed, nThread);

    // Test serie for Reals
    DebugTest<float>(min, max, nTest, seed, nThread);
    DebugTest<double>(min, max, nTest, seed, nThread);
}
};


TEST(RngManager, debug_predictable_mono)
{
    unsigned int nTest(N_TEST);
    int min(1), max(500), seed(TEST_SEED);
    TestFunctions<RNGManager>::DebugTest<unsigned short int>(min, max, nTest, seed);
}
TEST(RngManager, debug_predictable_multi)
{
    unsigned int nTest(N_TEST), nThread(N_THREAD);
    int min(1), max(500), seed(TEST_SEED);
    TestFunctions<RNGManager>::DebugTest<unsigned short int>(min, max, nTest, seed, nThread);
}
