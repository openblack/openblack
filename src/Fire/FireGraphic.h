/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <functional>
#include <optional>
#include <vector>

#include <glm/vec3.hpp>

/// How a fire looks: flames licking at random points of its object's model while it burns, steam puffing off it as it is
/// cooled while still hot, smoke from one point once it goes out, and a flickering light on the land under a large
/// burning building. These move on with the game time; the fire's turn says when steam and smoke start.
namespace openblack::fire::graphic
{

/// A flame lives this long, fading in over the first second
constexpr float k_FlameLife = 4.3f;
/// A graphic not drawn for more than this many turns, whose fire has changed meanwhile, catches up by a flame's life
constexpr uint32_t k_CatchUpTurns = 10;
constexpr float k_FlameFadeIn = 1.0f;
/// The brightest a flame is drawn, of 255
constexpr float k_FlameAlpha = 250.0f;
/// The flames' colour, 0xRRGGBB
constexpr uint32_t k_FlameColour = 0xFF713Cu;
/// Steam and smoke puff for this many turns, so many a second, each living this long
constexpr uint32_t k_PuffTurns = 30;
constexpr float k_PuffsPerSecond = 4.0f;
constexpr float k_PuffLife = 3.0f;
/// Steam starts as something hotter than this is cooled
constexpr float k_SteamTemperature = 75.0f;
/// The smoke's grey, 0xRRGGBB
constexpr uint32_t k_SmokeColour = 0x707070u;

/// Which of the four kinds an object shows, and whether its flames follow the land as it moves
struct Kinds
{
	bool flames {true};
	bool smoke {true};
	bool steam {true};
	bool lightMap {true};
	bool followsLand {false};
};

/// A flame, a puff of steam or of smoke
struct Sprite
{
	/// A flame's point in its object's model; a puff's in the world
	glm::vec3 position {0.0f};
	/// Half its width
	float scale {1.0f};
	float age {0.0f};
	float alpha {255.0f};
	glm::vec3 velocity {0.0f};
	/// A puff's size before it grows
	float baseScale {1.0f};
};

struct Graphic
{
	Kinds kinds;
	/// The most flames it keeps burning, and their size for its model
	int maxFlames {2};
	float localScale {1.0f};

	float flameAccumulator {0.0f};
	int flameCount {0};
	/// It flared once when it first became very hot
	bool flared {false};
	std::vector<Sprite> flames;

	/// The turn the steam started on, none while it doesn't puff; and the temperature it started at
	std::optional<uint32_t> steamStart;
	float steamAccumulator {0.0f};
	int steamCount {0};
	float steamTemperature {0.0f};
	std::vector<Sprite> steam;

	std::optional<uint32_t> smokeStart;
	float smokeAccumulator {0.0f};
	int smokeCount {0};
	glm::vec3 smokePoint {0.0f};
	std::vector<Sprite> smoke;

	/// The turn it was last drawn on, none before it first is, and how much of it burned then
	std::optional<uint32_t> drawnTurn;
	float drawnFraction {0.0f};
};

/// The most flames an object keeps: two on a tree, else two on something small and seven on something 3 m or bigger
[[nodiscard]] int MaxFlames(bool tree, float radius, float height);
/// What an object's flames are sized for: a tree; a model built of many jointed parts (a creature's body, a temple);
/// anything else, villagers and animals included
enum class FlameShape
{
	Tree,
	Jointed,
	Plain,
};
/// How big its flames are by its height: a fifth of it on a tree, three tenths on a jointed model, else half
[[nodiscard]] float LocalFlameScale(FlameShape shape, float height);

/// The fire this frame
struct Inputs
{
	/// How fierce it is, 0..1, its temperature and its turn's flags (fire::Flags)
	float fraction {0.0f};
	float temperature {0.0f};
	uint8_t flags {0};
	uint32_t turn {0};
	glm::vec3 wind {0.0f};
};

/// What the graphic needs of its object and of chance
struct Sampler
{
	/// A random point on its model, in the model's frame; none without a model
	std::function<std::optional<glm::vec3>()> localPoint;
	/// A point of its model in the world
	std::function<glm::vec3(const glm::vec3&)> toWorld;
	/// A random number up to a value, on this machine's own stream
	std::function<float(float)> random;
};

/// One frame of seconds: new flames and puffs, the old ones grow, fade and drift. True when the steam started this frame,
/// which hisses.
bool Update(Graphic& graphic, const Inputs& inputs, float seconds, const Sampler& sampler);

/// A flame's sheet cell at an age, running backwards through the second half of the sheet; a puff's forwards through
/// the first half
[[nodiscard]] int FlameCell(float age);
[[nodiscard]] int PuffCell(float age);

/// The grey a charred object is drawn in, each channel from 255 down to 80
[[nodiscard]] uint8_t CharredGrey(float charring);
/// The red-orange glow, 0xRRGGBB, added over a hot object as it is drawn, as the vertices' specular is: brighter the
/// hotter it is, full at 1000 degrees, flickering by a fifth with the noise, and fading as it chars
[[nodiscard]] uint32_t GlowColour(float temperature, float charring, float flicker);
/// How strongly a burning building's light shows on the land, 0..1, with its flicker
[[nodiscard]] float LightStrength(float fraction, float charring, float flicker);

/// How a tree with a fire on it is drawn: the land's light on it scaled by a grey of 256 (no brighter than the trees'
/// brightness of the frame), its foliage cut away at a higher alpha the hotter it is, and narrowed away (its height
/// kept) in the last of its life
struct TreeLook
{
	/// Of 256: 50, or less dark the more life it has left above nine tenths
	int grey;
	/// The alpha, of 255, below which its foliage isn't drawn
	float alphaReference;
	/// Its width across the ground, of its own; its height stays
	float scale;
};
[[nodiscard]] TreeLook BurningTree(float temperature, float combustion, float life);

} // namespace openblack::fire::graphic
