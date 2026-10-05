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

#include <bit>

#include <glm/vec3.hpp>

namespace openblack
{

/// The game's own random numbers, drawn exactly as the game draws them.
///
/// The arithmetic is float, one operation per statement: the game computes at single precision, so each of its
/// operations rounds as a float operation does, and keeping one operation per statement stops the compiler contracting
/// them.
namespace game_random
{
/// Both seeds start here when the game starts
constexpr uint32_t k_InitialSeed = 0x88F89F;
/// The range of the draw behind every float
constexpr uint32_t k_FloatRandRange = 0xFFFF;
/// About 1/65535, from the game's bits
inline constexpr float k_FloatRandScale = std::bit_cast<float>(0x37800080u);
/// About 1/32767, from the game's bits
inline constexpr float k_CrtRandomScale = std::bit_cast<float>(0x38000100u);

/// The game's generator: the seed becomes rotr(seed * 9377 + 0x24DF, 13), and the draw is it modulo n (unsigned). n must
/// not be 0
[[nodiscard]] constexpr uint32_t LHRand(uint32_t n, uint32_t& seed) noexcept
{
	seed = std::rotr(seed * 9377u + 0x24DFu, 13);
	return seed % n;
}

/// A float draw up to x: 0 for +-0 or NaN without a draw, else (u * x) * k with u = LHRand(0xFFFF), so it has the sign of
/// x and |r| <= 65534/65535 |x|
[[nodiscard]] float FloatRand(float x, uint32_t& seed) noexcept;

/// The C runtime's rand() on its seed: seed * 0x343FD + 0x269EC3, bits 16..30
[[nodiscard]] constexpr int32_t CrtRand(uint32_t& seed) noexcept
{
	seed = seed * 0x343FDu + 0x269EC3u;
	return static_cast<int32_t>((seed >> 16u) & 0x7FFFu);
}
} // namespace game_random

/// The seed every machine of a game shares, and this machine's own
struct GameRandomSeeds
{
	uint32_t synced {game_random::k_InitialSeed};
	uint32_t local {game_random::k_InitialSeed};

	bool operator==(const GameRandomSeeds&) const = default;
};

/// Which of the seeds particle effects draw from: the synced or the local one during each effect's step, and none outside
/// them, where every draw gives 0
enum class ParticleRandomStream : uint8_t
{
	None,
	Local,
	Synced,
};

class GameRandomInterface
{
public:
	virtual ~GameRandomInterface() = default;

	/// 0 for 0 without a draw, else LHRand(n) on the synced seed
	virtual uint32_t GameRand(uint32_t n) = 0;
	/// FloatRand(x) on the synced seed
	virtual float GameFloatRand(float x) = 0;
	/// 0 for 0 without a draw, else LHRand(n) on the local seed (a negative n divides as a large unsigned one)
	virtual uint32_t LocalRand(int32_t n) = 0;
	/// FloatRand(x) on the local seed
	virtual float LocalFloatRand(float x) = 0;
	/// The game thread's C runtime rand(), whose seed starts at 1
	virtual int32_t CrtRand() = 0;
	/// The C runtime's srand()
	virtual void CrtSrand(uint32_t seed) = 0;

	[[nodiscard]] virtual GameRandomSeeds GetSeeds() const = 0;
	/// Both are k_InitialSeed when the game starts and 0 from every map loaded, and saved games keep and restore them
	virtual void SetSeeds(GameRandomSeeds seeds) = 0;

	[[nodiscard]] virtual ParticleRandomStream GetParticleStream() const = 0;
	virtual void SetParticleStream(ParticleRandomStream stream) = 0;

	/// GameFloatRand(b - a) + a, b - a rounded to a float first
	float GameFloatRange(float a, float b);
	/// ((CrtRand() * k) * (b - a)) + a
	float CrtRandom(float a, float b);

	/// 0 outside an effect's step, else (u * k) * x with u = LHRand(0xFFFF) on the stream's seed.
	/// Unlike GameFloatRand it draws even for x == 0
	float ParticleFloatRand(float x);
	/// ParticleFloatRand(b - a) + a, b - a rounded to a float first
	float ParticleFloatRand(float a, float b);
	/// 0 outside an effect's step, else GameRand(n) or LocalRand(n) by the stream
	int32_t ParticleRand(int32_t n);
	/// A point in the unit ball, x, y and z drawn in that order as ParticleFloatRand(2) - 1 until
	/// (z^2 + y^2) + x^2 <= 1. Outside an effect's step the game would loop forever, which it never does; here it gives
	/// (-1, -1, -1)
	glm::vec3 ParticleRandR3();
};

/// An effect's step drawing from the synced or local seed, by whether the effect is synced across the network, and then
/// from neither: the draws that give 0 are put back rather than whichever were set before
class ParticleRandomStep
{
public:
	ParticleRandomStep(GameRandomInterface& random, bool synced) noexcept;
	~ParticleRandomStep();
	ParticleRandomStep(const ParticleRandomStep&) = delete;
	ParticleRandomStep& operator=(const ParticleRandomStep&) = delete;
	ParticleRandomStep(ParticleRandomStep&&) = delete;
	ParticleRandomStep& operator=(ParticleRandomStep&&) = delete;

private:
	GameRandomInterface& _random;
};

} // namespace openblack
