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

/// The game's own random numbers, exactly as runblack.exe draws them.
///
/// The arithmetic is float, one operation per statement: the game runs its FPU at 24 bits (fn_007DEE00 masks the
/// precision bits), so every x87 fmul / fadd rounds as a float operation does, and keeping one operation per
/// statement stops the compiler contracting them.
namespace game_random
{
/// GGame::Init 0x54F4AF: both seeds start here
constexpr uint32_t k_InitialSeed = 0x88F89F;
/// The range of the draw behind every float: LHRand(0xFFFF)
constexpr uint32_t k_FloatRandRange = 0xFFFF;
/// [0x8D6050] (and PSys's [0xD4E0B8], set at 0x672A94): about 1/65535, from its bits
inline constexpr float k_FloatRandScale = std::bit_cast<float>(0x37800080u);
/// [0x9A3700]: about 1/32767, from its bits
inline constexpr float k_CrtRandomScale = std::bit_cast<float>(0x38000100u);

/// _LHRand 0x7DB600: the seed becomes ror32(seed * 9377 + 0x24DF, 13), and the draw is it modulo n (unsigned). The
/// game divides by zero for n == 0; its callers all test for it first
[[nodiscard]] constexpr uint32_t LHRand(uint32_t n, uint32_t& seed) noexcept
{
	seed = std::rotr(seed * 9377u + 0x24DFu, 13);
	return seed % n;
}

/// GData::FloatRand 0x5106B0: 0 for +-0 or NaN without a draw, else (u * x) * k with u = LHRand(0xFFFF), so it has the
/// sign of x and |r| <= 65534/65535 |x|
[[nodiscard]] float FloatRand(float x, uint32_t& seed) noexcept;

/// The CRT's _rand 0x7C8837 on its seed (_tiddata +0x14): seed * 0x343FD + 0x269EC3, bits 16..30
[[nodiscard]] constexpr int32_t CrtRand(uint32_t& seed) noexcept
{
	seed = seed * 0x343FDu + 0x269EC3u;
	return static_cast<int32_t>((seed >> 16u) & 0x7FFFu);
}
} // namespace game_random

/// GData +8 and +0xC: the seed every machine of a game shares, and this machine's own
struct GameRandomSeeds
{
	uint32_t synced {game_random::k_InitialSeed};
	uint32_t local {game_random::k_InitialSeed};

	bool operator==(const GameRandomSeeds&) const = default;
};

/// Which of the seeds particle effects draw from: PSysManager points [0xD4E0C0] / [0xD4E0BC] at the synced or the
/// local draws for each effect's step, and at ones that give 0 outside them
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

	/// GRand::GameRand 0x6DE510 -> GData::Rand 0x510650: 0 for 0 without a draw, else LHRand(n) on the synced seed
	virtual uint32_t GameRand(uint32_t n) = 0;
	/// GRand::GameFloatRand 0x6DE530: FloatRand(x) on the synced seed
	virtual float GameFloatRand(float x) = 0;
	/// GRand::LocalRand 0x6DE570: 0 for 0 without a draw, else LHRand(n) on the local seed (a negative n divides as a
	/// large unsigned one)
	virtual uint32_t LocalRand(int32_t n) = 0;
	/// GRand::LocalFloatRand 0x6DE590: FloatRand(x) on the local seed
	virtual float LocalFloatRand(float x) = 0;
	/// The game thread's _rand 0x7C8837, whose seed starts at 1 (__initptd)
	virtual int32_t CrtRand() = 0;
	/// _srand 0x7C882A
	virtual void CrtSrand(uint32_t seed) = 0;

	[[nodiscard]] virtual GameRandomSeeds GetSeeds() const = 0;
	/// GGame::Init sets both to k_InitialSeed, GData::Reset 0x510750 (on every map cleared, GGame::ClearMap) both to 0,
	/// and saved games keep and restore them (WriteSafe 0x563440, ReadSafe 0x563620)
	virtual void SetSeeds(GameRandomSeeds seeds) = 0;

	[[nodiscard]] virtual ParticleRandomStream GetParticleStream() const = 0;
	virtual void SetParticleStream(ParticleRandomStream stream) = 0;

	/// 0x5E1CE0: GameFloatRand(b - a) + a, b - a rounded to a float first
	float GameFloatRange(float a, float b);
	/// ?Random@@YAMMM@Z 0x81D180: ((CrtRand() * k) * (b - a)) + a
	float CrtRandom(float a, float b);

	/// PSysFloatRand 0x6729B0: 0 outside an effect's step, else (u * k) * x with u = LHRand(0xFFFF) on the stream's seed.
	/// Unlike GameFloatRand it draws even for x == 0
	float ParticleFloatRand(float x);
	/// 0x6729C0: ParticleFloatRand(b - a) + a, b - a rounded to a float first
	float ParticleFloatRand(float a, float b);
	/// PSysRand 0x6729E0: 0 outside an effect's step, else GameRand(n) or LocalRand(n) by the stream
	int32_t ParticleRand(int32_t n);
	/// PSysRandR3 0x6729F0: a point in the unit ball, x, y and z drawn in that order as ParticleFloatRand(2) - 1 until
	/// (z^2 + y^2) + x^2 <= 1. Outside an effect's step the game would loop forever, which it never does; here it gives
	/// (-1, -1, -1)
	glm::vec3 ParticleRandR3();
};

/// An effect's step drawing from the synced or local seed (fn_00673340, by the effect's NET_GAME_TYPE, +0xAC), and then
/// from neither: the game puts back the draws that give 0 rather than whichever were set before
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
