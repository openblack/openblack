/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GestureTrail.h"

#include <cmath>

#include <array>
#include <utility>

using namespace openblack::particles;

namespace
{
/// The fewest points a path needs to be walked between them
constexpr size_t k_WalkedPoints = 3;
} // namespace

TrailPath::TrailPath(std::vector<glm::vec3> points)
    : _points(std::move(points))
{
	Measure();
}

void TrailPath::SetPoint(size_t index, glm::vec3 point)
{
	_points.at(index) = point;
	Measure();
}

void TrailPath::Measure()
{
	_distances.assign(_points.size(), 0.0f);
	for (size_t i = 1; i < _points.size(); ++i)
	{
		const auto step = _points[i] - _points[i - 1];
		const double length = std::sqrt((static_cast<double>(step.x) * step.x) + (static_cast<double>(step.y) * step.y) +
		                                (static_cast<double>(step.z) * step.z));
		_distances[i] = static_cast<float>(length + _distances[i - 1]);
	}
}

float TrailPath::Length() const
{
	return _distances.empty() ? 0.0f : _distances.back();
}

float gesture_trail::Transition(float t, float gain)
{
	const double eased = ((3.0 - (static_cast<double>(t) + t)) * t * t) - t;
	return static_cast<float>((eased * gain) + t);
}

uint8_t gesture_trail::RevealAlpha(float along, float grown, float maxAlpha)
{
	constexpr double k_ByteMax = 255.0;
	const auto fade = grown * maxAlpha;
	// Fading in from the start and from the end
	const auto fromStart = along < grown ? static_cast<float>((-static_cast<double>(fade) * (along / grown)) + fade) : 0.0f;
	const auto fromEnd = 1.0f - along;
	double alpha = fromEnd < grown ? (-static_cast<double>(fade) * (fromEnd / grown)) + fade : 0.0;
	alpha += fromStart;
	alpha = alpha <= 0.0 ? 0.0 : (alpha < k_ByteMax ? alpha : k_ByteMax);
	return static_cast<uint8_t>(static_cast<int>(alpha));
}

float gesture_trail::WiggleAmount(float age, float phaseSpeed, float dispersalTime, float shrinkTime)
{
	auto amount = static_cast<float>((std::cos(static_cast<double>(age) * phaseSpeed) + 1.0) * 0.5);
	const double dispersed = static_cast<double>(age) - dispersalTime;
	if (dispersed >= 0.0)
	{
		const auto left = static_cast<float>(1.0 - (dispersed / shrinkTime));
		amount *= left <= 0.0f ? 0.0f : (left < 1.0f ? left : 1.0f);
	}
	return amount;
}

glm::vec3 gesture_trail::Lift(glm::vec3 point, glm::vec3 camera, float size)
{
	auto towards = camera - point;
	float distance = 0.0f;
	if (towards != glm::vec3(0.0f))
	{
		distance = static_cast<float>(std::sqrt((static_cast<double>(towards.y) * towards.y) +
		                                        (static_cast<double>(towards.x) * towards.x) +
		                                        (static_cast<double>(towards.z) * towards.z)));
		const double inverse = 1.0 / distance;
		towards = {static_cast<float>(inverse * towards.x), static_cast<float>(inverse * towards.y),
		           static_cast<float>(inverse * towards.z)};
	}
	if (towards.y <= 0.0f)
	{
		return point;
	}
	const double steep = static_cast<double>(size) / towards.y;
	const auto half = distance * k_MostLift;
	const auto lift = static_cast<float>(steep <= half ? steep : half);
	return point + (towards * lift);
}

float gesture_trail::SheetStrength(float age, float lifetime)
{
	double share = static_cast<double>(age) / lifetime;
	share = share <= 0.0 ? 0.0 : (share < 1.0 ? share : 1.0);
	const double centred = share + share - 1.0;
	return static_cast<float>(1.0 - (centred * centred));
}

double gesture_trail::FlashLeft(float since, float duration)
{
	const double left = 1.0 - (static_cast<double>(since) / duration);
	return left <= 0.0 ? 0.0 : (left < 1.0 ? left : 1.0);
}

uint32_t gesture_trail::Dimmed(uint32_t argb, uint8_t level)
{
	uint32_t result = 0;
	for (uint32_t shift = 0; shift < 32; shift += 8)
	{
		const uint32_t channel = (argb >> shift) & 0xFFu;
		result |= ((channel * level) >> 8u) << shift;
	}
	return result;
}

float gesture_trail::ChainDistanceScale(float distance)
{
	constexpr std::array<float, 4> k_Distances {0.0f, 50.0f, 500.0f, 1500.0f};
	constexpr std::array<float, 4> k_Scales {0.2f, 1.0f, 1.0f, 1.5f};
	if (distance <= k_Distances.front())
	{
		return k_Scales.front();
	}
	if (distance >= k_Distances.back())
	{
		return k_Scales.back();
	}
	size_t i = 0;
	while (distance >= k_Distances.at(i + 1))
	{
		++i;
	}
	const double t =
	    (static_cast<double>(distance) - k_Distances.at(i)) / (static_cast<double>(k_Distances.at(i + 1)) - k_Distances.at(i));
	return static_cast<float>((t * (static_cast<double>(k_Scales.at(i + 1)) - k_Scales.at(i))) + k_Scales.at(i));
}

glm::vec3 TrailPath::At(float fraction) const
{
	if (_points.empty())
	{
		return glm::vec3(0.0f);
	}
	const double distance = static_cast<double>(fraction) * _distances.back();
	if (_distances.back() < distance)
	{
		return _points.back();
	}
	if (_points.size() < k_WalkedPoints)
	{
		return _points.front();
	}
	// The first segment that reaches the distance
	size_t i = 0;
	while (i + 2 < _points.size() && _distances[i + 1] < distance)
	{
		++i;
	}
	const auto& from = _points[i];
	const auto& to = _points[i + 1];
	const double span = static_cast<double>(_distances[i + 1]) - _distances[i];
	if (span == 0.0)
	{
		return from;
	}
	const auto t = static_cast<float>((distance - _distances[i]) / span);
	return (to - from) * t + from;
}
