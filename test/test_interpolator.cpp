/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <array>
#include <numeric>

#include <Common/ZoomInterpolator.h>
#include <gtest/gtest.h>

template <typename T>
    requires std::is_floating_point_v<T>
constexpr std::array<T, 5> ExpTaylorSeries(std::chrono::duration<T> time)
{
	std::array<T, 5> res;
	res[0] = static_cast<T>(1.0);
	res[1] = time.count() * res[0] * static_cast<T>(1.0) / static_cast<T>(1.0);
	res[2] = time.count() * res[1] * static_cast<T>(1.0) / static_cast<T>(2.0);
	res[3] = time.count() * res[2] * static_cast<T>(1.0) / static_cast<T>(3.0);
	res[4] = time.count() * res[3] * static_cast<T>(1.0) / static_cast<T>(6.0);
	return res;
}

template <typename T, size_t N>
constexpr T SumOfArray(const std::array<T, N>& array)
{
	return std::accumulate(array.cbegin(), array.cend(), static_cast<T>(0.0));
}

TEST(TestInterpolator, TestExpTaylorSeries)
{
	ASSERT_FLOAT_EQ(SumOfArray(ExpTaylorSeries(std::chrono::duration<float>(0.001f))), 1.0010005f);
	ASSERT_FLOAT_EQ(SumOfArray(ExpTaylorSeries(std::chrono::duration<float>(0.015f))), 1.0151131f);
	ASSERT_FLOAT_EQ(SumOfArray(ExpTaylorSeries(std::chrono::duration<float>(0.016f))), 1.0161288f);
	ASSERT_FLOAT_EQ(SumOfArray(ExpTaylorSeries(std::chrono::duration<float>(0.032f))), 1.0325174f);
}

TEST(TestInterpolator, TestExpTaylorSeriesStd)
{
	const float epsilon = 5e-5f;
	ASSERT_NEAR(SumOfArray(ExpTaylorSeries(std::chrono::duration<float>(0.001f))), std::exp(0.001f), epsilon);
	ASSERT_NEAR(SumOfArray(ExpTaylorSeries(std::chrono::duration<float>(0.015f))), std::exp(0.015f), epsilon);
	ASSERT_NEAR(SumOfArray(ExpTaylorSeries(std::chrono::duration<float>(0.016f))), std::exp(0.016f), epsilon);
	ASSERT_NEAR(SumOfArray(ExpTaylorSeries(std::chrono::duration<float>(0.032f))), std::exp(0.032f), epsilon);
}

TEST(TestInterpolator, ExpValues)
{
	const openblack::ZoomInterpolator interpolator(0.0f, 1.0f, 0.0f, 1.0f);

	const std::array<float, 102> expectedValues = {
	    0.0f,        0.0002970099f, 0.0011761596f, 0.0026198092f, 0.0046105585f, 0.007131247f, 0.010164955f, 0.013695003f,
	    0.01770495f, 0.022178598f,  0.027099982f,  0.032453388f,  0.038223337f,  0.044394586f, 0.050952133f, 0.05788123f,
	    0.06516734f, 0.07279619f,   0.08075373f,   0.08902619f,   0.09759998f,   0.10646179f,  0.115598544f, 0.12499741f,
	    0.13464576f, 0.14453122f,   0.15464173f,   0.16496536f,   0.1754905f,    0.18620574f,  0.19709992f,  0.2081621f,
	    0.21938165f, 0.23074806f,   0.24225119f,   0.2538811f,    0.26562795f,   0.2774824f,   0.28943512f,  0.3014772f,
	    0.31359977f, 0.32579434f,   0.33805263f,   0.3503667f,    0.3627286f,    0.3751309f,   0.3875662f,   0.40002745f,
	    0.41250777f, 0.42500058f,   0.43749958f,   0.44999853f,   0.46249175f,   0.47497338f,  0.4874381f,   0.4998808f,
	    0.51229644f, 0.52468055f,   0.53702843f,   0.549336f,     0.5615994f,    0.57381487f,  0.58597875f,  0.59808797f,
	    0.6101395f,  0.6221305f,    0.6340587f,    0.6459215f,    0.65771705f,   0.6694435f,   0.6810993f,   0.6926831f,
	    0.7041937f,  0.71563053f,   0.72699296f,   0.73828053f,   0.749493f,     0.7606305f,   0.7716938f,   0.7826829f,
	    0.7935991f,  0.80444336f,   0.815217f,     0.8259213f,    0.83655834f,   0.8471303f,   0.8576393f,   0.86808753f,
	    0.8784783f,  0.88881457f,   0.899099f,     0.90933526f,   0.91952777f,   0.9296801f,   0.93979585f,  0.94988f,
	    0.95993733f, 0.96997285f,   0.97999096f,   0.98999786f,   0.9999989f,    1.0f,
	};

	const float epsilon = 1e-2f;
	for (uint32_t i = 0; i < expectedValues.size(); ++i)
	{
		ASSERT_NEAR(interpolator.PositionAt(i / static_cast<float>(expectedValues.size() - 1)), expectedValues.at(i), epsilon);
	}
}

TEST(TestInterpolator, RealWorldValues)
{
	auto duration = 0.3f;
	const openblack::ZoomInterpolator interpolator(3.00015593f, 3.0f, -0.00141694723f * duration, 0.0f);

	const float epsilon = 1e-8f;
	ASSERT_NEAR(interpolator.PositionAt(0.016f / duration), 3.00013423f, epsilon);
}

TEST(TestInterpolator, RealWorldVelocities)
{
	const auto value0 = 1000.0f;
	const auto value1 = -368.455078f;
	const auto value2 = -769.054199f;
	const auto duration1 = 1.39680004f;
	const auto duration2 = 1.36159992f;
	const auto position1 = 961.825683f;
	const auto velocity1 = -725.472473f;
	const auto position2 = 961.096191f;
	const auto velocity2 = -733.466064f;
	const auto dt1 = 0.1f;
	const auto dt2 = 0.001f;
	const float epsilon = 1e-3f;

	const openblack::ZoomInterpolator interpolator1(value0, value1, 0.0f, 0.0f);
	ASSERT_NEAR(interpolator1.PositionAt(dt1 / duration1), position1, epsilon);
	ASSERT_NEAR(interpolator1.VelocityAt(dt1 / duration1), velocity1 * duration1, epsilon);

	const openblack::ZoomInterpolator interpolator2(position1, value2, velocity1 * duration2, 0.0f);
	ASSERT_NEAR(interpolator2.PositionAt(dt2 / duration2), position2, epsilon);
	ASSERT_NEAR(interpolator2.VelocityAt(dt2 / duration2), velocity2 * duration2, epsilon);
}
