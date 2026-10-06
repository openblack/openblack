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

#include <array>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "3D/MapCoords.h"

/// The game's angles.
///
/// - A game angle is an 11-bit integer, 2048 to the circle: 0 is +x, 0x200 is +z, 0x400 is -x and 0x600 is -z. Angles
///   between points come from an arctangent table, so they can be up to a couple of steps off the true angle.
/// - A 3D angle is in radians, the same way round.
/// - A "Scawen" angle is a 3D angle plus a quarter turn, as creatures and a few objects keep it.
///
/// Every float operation is a single float operation, as the game's 24-bit FPU rounds it. Sines and cosines of float
/// angles are taken in double and rounded once by the product that follows them.
namespace openblack::gutils
{

/// Game angles to the circle
constexpr int32_t k_GameAngleCircle = 0x800;
constexpr int32_t k_GameAngleMask = 0x7FF;
/// 2 pi / 2048, the float the game converts game angles to radians with
constexpr float k_GameAngleTo3D = 0.0030679617f;
/// 2048 / 2 pi, the float the game converts radians to game angles with
constexpr float k_Angle3DToGame = 325.94931f;
/// pi / 2048
constexpr float k_HalfGameAngleTo3D = 0.0015339808f;
/// The quarter turn between a Scawen angle and a 3D angle
constexpr float k_ScawenOffset = 1.5707964f;

/// The game's arctangent table: entry i is trunc(atan(i / 256) * 1024 / pi), for 0 to 256
[[nodiscard]] const std::array<uint16_t, 257>& ArcTanTable();
/// The game's sine table, a circle and a quarter long so that cosines read it a quarter on: entry i is
/// trunc(65536 sin(i * 2 pi / 2048))
[[nodiscard]] const std::array<int32_t, 2560>& SinTable();
/// 65536 cos of a game angle
[[nodiscard]] int32_t Cos(uint16_t angle);
/// 65536 sin of a game angle
[[nodiscard]] int32_t Sin(uint16_t angle);

/// The game angle of a direction from the arctangent table, 0 for no direction. Each octant divides the smaller side by
/// the larger, scaled by 256, as unsigned 32-bit integers, and a tie between the sides falls to the first octant tried
[[nodiscard]] uint16_t LHArcTan(int32_t dx, int32_t dz);
[[nodiscard]] uint16_t GetAngleFromDXDZ(int32_t dx, int32_t dz);
/// The game angle from one map position to another
[[nodiscard]] uint16_t GetAngleFromXZ(const map_coords::MapCoords& from, const map_coords::MapCoords& to);
/// The same on map positions kept as (x, z)
[[nodiscard]] uint16_t GetAngleFromXZ(glm::ivec2 from, glm::ivec2 to);
/// The same on (x, z) points in metres, each made a map position first: the difference of two truncated positions can
/// be a unit off the truncated difference
[[nodiscard]] uint16_t GetAngleFromXZ(glm::vec2 from, glm::vec2 to);
/// The angle from one position to another in radians, through the game angle: quantised to 2048 steps
[[nodiscard]] float Get3DAngleFromXZ(const map_coords::MapCoords& from, const map_coords::MapCoords& to);
[[nodiscard]] float Get3DAngleFromXZ(glm::ivec2 from, glm::ivec2 to);
[[nodiscard]] float Get3DAngleFromXZ(glm::vec2 from, glm::vec2 to);

/// Radians to a game angle, truncated towards 0 and masked, so a negative angle wraps round (-0.5 rad is 1886)
[[nodiscard]] uint32_t ConvertAngle3DToGame(float radians);
/// A game angle, masked, to radians
[[nodiscard]] float ConvertGameAngleTo3D(int32_t angle);
[[nodiscard]] uint32_t ConvertScawenAngleToGameAngle(float radians);
/// Twice the angle times pi / 2048, plus a quarter turn: two roundings, and unlike the others not masked
[[nodiscard]] float ConvertGameAngleToScawenAngle(uint16_t angle);

/// The x and z of a distance along a game angle: (65536 cos * d) / 65536 in float
[[nodiscard]] float GetXByAngle(uint16_t angle, float distance);
[[nodiscard]] float GetZByAngle(uint16_t angle, float distance);
/// The x and z of a distance in map units along a game angle, the distance shifted down 4 bits first so that the
/// 32-bit product doesn't overflow, rounding towards minus infinity
[[nodiscard]] int32_t GetXByAngleBigDistance(uint16_t angle, int32_t whole);
[[nodiscard]] int32_t GetZByAngleBigDistance(uint16_t angle, int32_t whole);
/// The same with the distance shifted down 8 bits, for longer distances
[[nodiscard]] int32_t GetXByAngleHugeDistance(uint16_t angle, int32_t whole);
[[nodiscard]] int32_t GetZByAngleHugeDistance(uint16_t angle, int32_t whole);
/// The x and z of a distance in metres along a game angle, in map units truncated towards 0
[[nodiscard]] int32_t GetXByAngleMetersDistance(uint16_t angle, float metres);
[[nodiscard]] int32_t GetZByAngleMetersDistance(uint16_t angle, float metres);
/// The offset of a distance in map units along a game angle, at no altitude
[[nodiscard]] map_coords::MapCoords GetPosFromGameAngle(uint16_t angle, int32_t whole);
/// The same with the distance in metres, which loses its low 4 bits on the way
[[nodiscard]] map_coords::MapCoords GetPosFromGameAngle(uint16_t angle, float metres);

/// The offset of a distance in metres along an angle in radians, as a map position at no altitude
[[nodiscard]] map_coords::MapCoords GetPosFromAngle(float radians, float metres);
/// Moves a map position a distance in metres along an angle in radians, keeping its altitude
void AddDistanceFromAngle(map_coords::MapCoords& pos, float radians, float metres);
/// The offset of a distance in metres along an angle in radians, as a point
[[nodiscard]] glm::vec3 GetPointFromAngle(float radians, float metres);

/// How far apart two game angles are, the short way round: 0 to 0x400 for angles of 0 to 0x7FF
[[nodiscard]] uint32_t GetAngleDifference(int32_t a, int32_t b);
/// Which way is shorter from one game angle to another: 0 for none, -1 or +1. Exactly half a turn apart, it goes the
/// way the plain difference points
[[nodiscard]] int32_t GetAngleSign(int32_t from, int32_t to);

} // namespace openblack::gutils
