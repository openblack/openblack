/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GestureTrailBuilder.h"

#include <cmath>

#include <algorithm>

using namespace openblack;
using namespace openblack::gesture;

namespace
{
/// The symbol files' square runs from -100 to 100 on both axes
constexpr double k_SymbolHalfSize = 100.0;
constexpr float k_SymbolScale = 0.005f;
/// The furthest any box starts from
constexpr float k_FarAway = 1e7f;

struct Box
{
	float minX;
	float minY;
	float maxX;
	float maxY;
};

/// The screen box the symbol's 0..1 square is stretched over: as wide and tall as the path's box against the symbol's
/// own box, about the path's box's middle
Box FitSymbol(const ScreenBox& box, const TrailSymbol& symbol)
{
	const auto middleX = static_cast<float>((static_cast<double>(box.min.x) + box.max.x) * 0.5);
	const auto middleY = static_cast<float>((static_cast<double>(box.max.y) + box.min.y) * 0.5);
	const auto halfHeight = static_cast<float>(((static_cast<double>(box.max.y) - box.min.y) * 0.5) /
	                                           (static_cast<double>(symbol.maxZ) - symbol.minZ));
	const auto halfWidth = static_cast<float>(((static_cast<double>(box.max.x) - box.min.x) * 0.5) /
	                                          (static_cast<double>(symbol.maxX) - symbol.minX));
	return {
	    .minX = middleX - halfWidth, .minY = middleY - halfHeight, .maxX = halfWidth + middleX, .maxY = halfHeight + middleY};
}

/// The pixel a point of the symbol's square falls on, truncated towards zero
glm::ivec2 PixelOf(const Box& box, float x, float z)
{
	const double px = ((static_cast<double>(box.maxX) - box.minX + 1.0) * x) + box.minX;
	const double py = ((static_cast<double>(box.maxY) - box.minY + 1.0) * z) + box.minY;
	return {static_cast<int>(px), static_cast<int>(py)};
}
} // namespace

TrailSymbol gesture::MakeTrailSymbol(std::span<const glm::vec3> filePoints)
{
	TrailSymbol symbol {.minX = k_FarAway, .maxX = -k_FarAway, .minZ = k_FarAway, .maxZ = -k_FarAway};
	if (filePoints.empty())
	{
		return symbol;
	}
	std::vector<glm::vec3> square;
	square.reserve(filePoints.size());
	for (const auto& point : filePoints)
	{
		const auto x = static_cast<float>((static_cast<double>(point.x) + k_SymbolHalfSize) * k_SymbolScale);
		const auto z = static_cast<float>((k_SymbolHalfSize - point.z) * k_SymbolScale);
		square.emplace_back(x, point.y, z);
		symbol.maxX = std::max(x, symbol.maxX);
		symbol.minX = std::min(x, symbol.minX);
		symbol.maxZ = std::max(z, symbol.maxZ);
		symbol.minZ = std::min(z, symbol.minZ);
	}
	// As many points again, evenly along its length from its start, the last one step short of its end
	const particles::TrailPath path(std::move(square));
	const auto count = filePoints.size();
	const auto step = static_cast<float>(1.0 / static_cast<double>(static_cast<float>(count)));
	symbol.points.reserve(count);
	for (size_t i = 0; i < count; ++i)
	{
		symbol.points.push_back(path.At(static_cast<float>(static_cast<double>(i) * step)));
	}
	return symbol;
}

std::vector<glm::vec3> gesture::TrailLandPoints(const GestureRecorder& recorder)
{
	std::vector<glm::vec3> points;
	for (size_t i = 0; i < recorder.Count(); ++i)
	{
		const auto& world = recorder.At(i).world;
		const auto onLand = [](float value) { return std::abs(static_cast<double>(value)) > k_TrailOnLand; };
		if (onLand(world.x) || onLand(world.y) || onLand(world.z))
		{
			points.push_back(world);
		}
	}
	return points;
}

std::optional<particles::GestureTrail> gesture::BuildTrail(std::span<const glm::vec3> landPoints, const ScreenBox& boxInPixels,
                                                           const TrailSymbol& symbol, const TrailView& view)
{
	if (landPoints.size() < 2 || symbol.points.empty() || !view.pointUnder)
	{
		return std::nullopt;
	}
	const auto box = FitSymbol(boxInPixels, symbol);

	// Depth is measured along the way the camera looks across the land, width across it
	glm::vec3 forward(view.cameraForward.x, 0.0f, view.cameraForward.z);
	if ((forward.x * forward.x) + (forward.z * forward.z) < k_TrailLevelEpsilon)
	{
		forward = {1.0f, 0.0f, 0.0f};
	}
	const auto inverseLength = static_cast<float>(
	    1.0 / std::sqrt((static_cast<double>(forward.z) * forward.z) + (static_cast<double>(forward.x) * forward.x)));
	forward *= inverseLength;

	std::vector<glm::vec3> laid;
	float squash = 1.0f;
	for (int tries = 1;; ++tries)
	{
		laid.clear();
		for (const auto& point : symbol.points)
		{
			const auto z = static_cast<float>(((static_cast<double>(point.z) - 0.5) * squash) + 0.5);
			laid.push_back(view.pointUnder(PixelOf(box, point.x, z)));
		}
		float minAcross = k_FarAway;
		float maxAcross = -k_FarAway;
		float minDepth = k_FarAway;
		float maxDepth = -k_FarAway;
		for (const auto& point : laid)
		{
			const auto across = static_cast<float>((-static_cast<double>(forward.x) * point.z) + (forward.z * point.x));
			const auto depth =
			    static_cast<float>((static_cast<double>(forward.y) * point.y) + (forward.x * point.x) + (forward.z * point.z));
			minAcross = std::min(minAcross, across);
			maxAcross = std::max(maxAcross, across);
			minDepth = std::min(minDepth, depth);
			maxDepth = std::max(maxDepth, depth);
		}
		const double width = std::abs(static_cast<double>(maxAcross) - minAcross);
		const double depthOverWidth = width <= 0.0 ? 1.0 : std::abs(static_cast<double>(maxDepth) - minDepth) / width;
		squash *= k_TrailSquash;
		if (tries >= k_TrailSquashTries || depthOverWidth <= k_TrailMostDepth)
		{
			break;
		}
	}

	// The shape on the land, taken at as many points as the hand's path has, evenly through its points
	const auto count = landPoints.size();
	const auto step = static_cast<float>(1.0 / (static_cast<double>(count) - 1.0));
	const auto last = laid.size() - 1;
	std::vector<glm::vec3> ideal;
	ideal.reserve(count);
	for (size_t i = 0; i < count; ++i)
	{
		const double along = static_cast<double>(i) * step;
		if (along == 1.0)
		{
			ideal.push_back(laid.back());
			continue;
		}
		const auto at = static_cast<float>(along * static_cast<double>(last));
		const auto index = std::min(static_cast<size_t>(at), last);
		// The end of the path rounded up holds at its last point
		const auto& from = laid[index];
		const auto& to = laid[std::min(index + 1, last)];
		const auto t = static_cast<float>(static_cast<double>(at) - static_cast<double>(index));
		ideal.push_back(((to - from) * t) + from);
	}

	particles::GestureTrail trail;
	trail.drawn = particles::TrailPath(std::vector<glm::vec3>(landPoints.begin(), landPoints.end()));
	trail.ideal = particles::TrailPath(std::move(ideal));
	return trail;
}
