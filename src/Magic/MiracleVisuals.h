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

#include <optional>
#include <vector>

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// How the miracles look outside their effects: the one-shot globes and the miracle floating in each, the rings round
/// an extreme miracle, the bands and bracelets on the hand that holds a miracle and its glow. Pure rules, which the
/// systems step and the renderer draws.
namespace openblack::magic::visuals
{

// The globe

/// The glint on the globe's dome runs through the 16 cells of a 4 by 4 sheet, 18 a second, round and round
inline constexpr float k_GlintFrameRate = 18.0f;
inline constexpr int k_GlintCells = 16;
/// The globe is added over what is behind it at 150 of 255
inline constexpr uint8_t k_GlobeAlpha = 150;
/// The miracle in a globe is drawn at this share of the globe's size
inline constexpr float k_GlobeSeedScale = 0.6f;

/// The glint's frame after some seconds more
[[nodiscard]] float StepGlint(float frame, float seconds);
/// The corner of the glint's cell in the sheet: a quarter across for each of its column and row
[[nodiscard]] glm::vec2 GlintUvOffset(float frame);

// The rings round an extreme miracle in a globe or above a worship icon

/// They spin at 10.3 radians a second
inline constexpr float k_RingSpin = 10.3f;
/// Their alpha is 60 out of 256 of what they are drawn in
inline constexpr uint8_t k_RingAlpha = 60;
/// The ring's model is this much of the miracle's size
inline constexpr float k_RingScale = 0.2f;
/// And it is grey in its specular, 20 of 255 in each colour
inline constexpr uint32_t k_RingSpecular = 0x141414u;

/// One ring for the first power-up, two for the second; none for a plain miracle
[[nodiscard]] int RingCount(int powerUp);
/// A ring's alpha in something drawn at the given alpha: 35 in a globe, 59 above an icon
[[nodiscard]] uint8_t RingAlpha(uint8_t drawnAlpha);
/// The ring's turn before the camera's: laid flat, turned about the vertical by the spin (and half a radian more for the
/// second ring), tipped by 0.3, turned back or on by one radian, and tipped by 0.2 more
[[nodiscard]] glm::mat3 RingTurn(int ring, float spin);
/// Where a ring is drawn round a point: its turn, then turned to face the camera from where it is, scaled by the ring's
/// share times the band scale times the miracle's size
[[nodiscard]] glm::mat4 RingModel(int ring, float spin, const glm::vec3& point, float scale, const glm::vec3& camera);
/// The turn that faces something at a point towards the camera, from the way from the camera to it and world up made
/// square to that way. The rings' first axis points to the camera, their second up and their third across; the globe's
/// first is across, its second points to the camera and its third up.
enum class Facing : uint8_t
{
	Ring,
	Globe,
};
[[nodiscard]] glm::mat3 FacingCamera(const glm::vec3& point, const glm::vec3& camera, Facing facing);

// The creature spell phials in a globe

/// Their texture runs through the 32 cells of a sheet of 8 by 4, an eighth each way, backwards, 15 a second
inline constexpr float k_PhialFrameRate = -15.0f;
inline constexpr int k_PhialCells = 32;
[[nodiscard]] float StepPhialFrame(float frame, float seconds);
/// The corner of the phial texture's cell, an eighth across for each of its column and row
[[nodiscard]] glm::vec2 PhialUvOffset(float frame);
/// How fast a phial pulses, in pulses a second: the freeze's slower
[[nodiscard]] float PhialPulseSpeed(int receiveType);
/// How far through its pulse a phial is at a phase: 0 to 1 and back over a turn
[[nodiscard]] float PhialPulse(float phase);
/// A phial's shape at a phase of its pulse, by how its creature takes the spell: swelling across and through to two and a
/// half times for one kind, squashing across and through for another, never changing its height; no spell the game
/// ships takes either kind
[[nodiscard]] glm::vec3 PhialScale(int receiveType, float phase);
/// The ice colour a frozen phial is tinted towards by its pulse, 0xRRGGBB
[[nodiscard]] uint32_t FreezeTint(float pulse);
/// A phial's draws: how big, of its size, and how faint, of 255, each of its one or two draws is, by how its creature
/// takes the spell, at a phase and pulse of its animation, in a globe drawn at an alpha
struct PhialDraw
{
	float size;
	uint8_t alpha;
	/// Lit by the land, else drawn in plain white
	bool lit;
};
[[nodiscard]] std::vector<PhialDraw> PhialDraws(int receiveType, float phase, float pulse, uint8_t globeAlpha);

// The hand that holds a miracle

/// The bands that fly onto the hand, the bracelets that stay on it and the one that flies off
enum class BandKind : uint8_t
{
	/// Stays round the hand while it holds the miracle: one more for each power-up
	Bracelet,
	/// Flies from in front of the camera onto the hand, and goes
	FlyIn,
	/// Flies from the hand back to the camera, and goes
	FlyOff,
};

struct HandBand
{
	BandKind kind {BandKind::Bracelet};
	/// Its place along the hand: the further on, the further up the arm
	int index {0};
	float delay {0.0f};
	float age {0.0f};
	float duration {0.85f};
	uint8_t alphaFrom {20};
	uint8_t alphaTo {130};
	/// Its turn about the hand
	float spin {0.0f};
	/// It has reached the end of its flight
	bool done {false};
};

/// A band's radius round the hand, and its spin a second (a little faster the further up the arm)
inline constexpr float k_BandRadius = 10.0f;
inline constexpr float k_BandSpin = 12.0f;
inline constexpr float k_BandSpinPerIndex = 0.2f;
/// Where along the hand the bands sit, in the hand's own units
inline constexpr float k_BandFirstOffset = 10.0f;
inline constexpr float k_BandOffsetPerIndex = 40.0f;
/// A band flies from this far in front of the camera, at this share of its size
inline constexpr float k_BandFlyDistance = 4.0f;
inline constexpr float k_BandFlyScale = 0.5f;
/// The most bracelets the hand wears
inline constexpr int k_MostBracelets = 5;
/// The bands that fly in come this far apart, the first after this long when they wait for the miracle to settle
inline constexpr float k_FlyInSpacing = 0.1f;
inline constexpr float k_BandSettleDelay = 2.4f;

struct HandBands
{
	std::vector<HandBand> bracelets;
	std::vector<HandBand> flying;
};

/// The hand wears so many bracelets, adding or taking off the newest; new ones wait for the miracle to settle when
/// asked
void SetBracelets(HandBands& bands, int count, bool delayed);
/// Five bands fly onto the hand a tenth of a second apart
void AddFlyIn(HandBands& bands, bool delayed);
/// One band flies off the hand to the camera over a second
void AddFlyOff(HandBands& bands);
/// The bands age and spin; those that have flown go
void StepBands(HandBands& bands, float seconds);

/// Where a band is in its flight and how it looks
struct BandPose
{
	/// Shown at all: it has stopped waiting
	bool shown;
	/// 0 in front of the camera, 1 round the hand
	float t;
	uint8_t alpha;
};
[[nodiscard]] BandPose PoseOf(const HandBand& band);
/// The band round the hand, in the hand's frame: about its length, at its place along it, turned by its spin
[[nodiscard]] glm::mat4 BandOnHand(const HandBand& band);
/// The band round the hand before it starts spinning, where the bands fly to
[[nodiscard]] glm::mat4 BandOnHandUnspun(const HandBand& band);
/// The band in front of the camera, in the camera's frame (right, up, forward), the near plane permitting
[[nodiscard]] glm::mat4 BandAtCamera(float nearPlane);
/// A band flying between the camera and the hand: turned as the camera is, its size and place eased between the two
[[nodiscard]] glm::mat4 BandBetween(const glm::mat4& atCamera, const glm::mat4& onHand, float t);
/// A bracelet settling: every part of its matrix eased between the two
[[nodiscard]] glm::mat4 BandBlended(const glm::mat4& atCamera, const glm::mat4& onHand, float t);

/// The hand's glow while it holds a miracle: the flow's sheet over the hand at this share, its 32 cells backwards at 20
/// a second
inline constexpr float k_GlowShare = 0.8f;
inline constexpr float k_GlowFrameRate = -20.0f;
inline constexpr int k_GlowCells = 32;
/// The flow's texture is an eighth of the sheet across and down
inline constexpr float k_GlowCellSize = 0.125f;
[[nodiscard]] float StepGlowFrame(float frame, float seconds);
[[nodiscard]] glm::vec2 GlowUvOffset(float frame);

/// The announcer's voice as a miracle comes to the hand or powers up: the first, second or third power-up's, by its
/// level; none for a plain miracle. The sample's number in the spell dialogue bank.
[[nodiscard]] std::optional<int> PowerUpVoiceSample(int powerUp);

} // namespace openblack::magic::visuals
