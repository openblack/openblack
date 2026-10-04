/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The scenarios in audio/scenarios were produced by running the original BW1 v1.20 code (runblack.exe and
// LHaudiodllR.dll) under an x86 emulator on synthetic inputs. These tests check that the port reproduces the
// original's results exactly.

#include <cstring>

#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <3D/LandIslandInterface.h>
#include <Audio/AtmosAudio.h>
#include <Audio/AtmosPlayer.h>
#include <Audio/SoundMap.h>
#include <InfoConstants.h>
#include <LNDFile.h>
#include <gtest/gtest.h>
#include <json.hpp>

using namespace openblack;
using namespace openblack::audio;
using json = nlohmann::json;

namespace
{
const auto k_ScenarioPath = std::filesystem::path(TEST_BINARY_DIR) / "audio" / "scenarios";

json LoadScenario(const std::string& name)
{
	std::ifstream stream(k_ScenarioPath / name);
	EXPECT_TRUE(stream.is_open()) << name;
	return json::parse(stream);
}

std::vector<uint8_t> FromHex(const std::string& hex)
{
	std::vector<uint8_t> bytes(hex.size() / 2);
	for (size_t i = 0; i < bytes.size(); ++i)
	{
		bytes[i] = static_cast<uint8_t>(std::stoul(hex.substr(i * 2, 2), nullptr, 16));
	}
	return bytes;
}

pack::AudioBankSampleHeader HeaderFromHex(const std::string& hex)
{
	const auto bytes = FromHex(hex);
	pack::AudioBankSampleHeader header {};
	EXPECT_EQ(bytes.size(), sizeof(header));
	std::memcpy(&header, bytes.data(), sizeof(header));
	return header;
}

/// Turns a one-shot plays for, as in the scenario generator
uint32_t Duration(size_t bank, int32_t sample)
{
	return 1 + ((static_cast<uint32_t>(sample) * 7 + static_cast<uint32_t>(bank) * 3) % 13);
}

/// Records voices in the same form as the emulated original
class RecordingBackend final: public VoiceBackend
{
public:
	uint32_t turn {0};
	std::map<std::string, size_t> bankIndices;
	std::vector<json> trace;

	Handle Start(const VoiceStart& start) override
	{
		const auto handle = _next++;
		const auto bank = bankIndices.at(start.bankName);
		_voices[handle] = {.loop = start.loopCount < 0, .end = turn + Duration(bank, start.sampleId), .active = true};
		trace.push_back({
		    {"turn", turn},
		    {"event", "play"},
		    {"voice", handle},
		    {"bank", bank},
		    {"sample", start.sampleId},
		    {"volume", start.volume},
		    {"pitch", start.pitchPercent},
		    {"loop", start.loopCount},
		    {"positional", start.positional},
		    {"x", std::bit_cast<uint32_t>(start.position.x)},
		    {"y", std::bit_cast<uint32_t>(start.position.y)},
		    {"z", std::bit_cast<uint32_t>(start.position.z)},
		    {"min", std::bit_cast<uint32_t>(start.minDistance)},
		    {"max", std::bit_cast<uint32_t>(start.maxDistance)},
		    {"scale", std::bit_cast<uint32_t>(start.distanceScale)},
		});
		return handle;
	}

	[[nodiscard]] bool IsPlaying(Handle handle) const override
	{
		const auto iter = _voices.find(handle);
		return iter != _voices.end() && iter->second.active && (iter->second.loop || turn < iter->second.end);
	}

	void SetVolume(Handle handle, uint32_t volume) override
	{
		trace.push_back({{"turn", turn}, {"event", "volume"}, {"voice", handle}, {"volume", volume}});
	}

	void Stop(Handle handle) override
	{
		if (IsPlaying(handle))
		{
			trace.push_back({{"turn", turn}, {"event", "stop"}, {"voice", handle}});
		}
		_voices[handle].active = false;
	}

private:
	struct Voice
	{
		bool loop;
		uint32_t end;
		bool active;
	};
	std::map<Handle, Voice> _voices;
	Handle _next {1};
};

/// Events of one turn in a canonical order: the original walks its sample slots, the port its voice list
std::map<uint32_t, std::vector<std::string>> ByTurn(const std::vector<json>& trace)
{
	std::map<uint32_t, std::vector<std::string>> turns;
	for (const auto& event : trace)
	{
		turns[event.at("turn").get<uint32_t>()].push_back(event.dump());
	}
	for (auto& [turn, events] : turns)
	{
		std::ranges::sort(events);
	}
	return turns;
}

float FloatFromBits(const json& value)
{
	return std::bit_cast<float>(value.get<uint32_t>());
}

/// Only what the ambience reads from a landscape: the block lookup and the blocks' cells
class SyntheticIsland final: public LandIslandInterface
{
public:
	SyntheticIsland(const std::vector<uint8_t>& lookup, const std::vector<std::vector<lnd::LNDCell>>& blocks)
	    : _lookup(lookup)
	    , _blocks(blocks)
	{
	}

	[[nodiscard]] const lnd::LNDCell* FindCell(const glm::u16vec2& coordinates) const override
	{
		if (coordinates.x > 511 || coordinates.y > 511)
		{
			return nullptr;
		}
		const auto block = _lookup.at(((coordinates.x >> 4) << 5) | (coordinates.y >> 4));
		if (block == 0)
		{
			return nullptr;
		}
		return &_blocks.at(block - 1).at(((coordinates.x & 0xF) * 17) + (coordinates.y & 0xF));
	}

	[[nodiscard]] float GetHeightAt(glm::vec2) const override { throw std::logic_error("unused"); }
	[[nodiscard]] glm::vec3 GetNormalAt(glm::vec2) const override { throw std::logic_error("unused"); }
	[[nodiscard]] const lnd::LNDCell& GetCell(const glm::u16vec2&) const override { throw std::logic_error("unused"); }
	void DumpTextures() const override {}
	void DumpMaps() const override {}
	[[nodiscard]] std::vector<LandBlock>& GetBlocks() override { throw std::logic_error("unused"); }
	[[nodiscard]] const std::vector<LandBlock>& GetBlocks() const override { throw std::logic_error("unused"); }
	[[nodiscard]] const std::vector<lnd::LNDCountry>& GetCountries() const override { throw std::logic_error("unused"); }
	[[nodiscard]] const graphics::Texture2D& GetAlbedoArray() const override { throw std::logic_error("unused"); }
	[[nodiscard]] const graphics::Texture2D& GetBump() const override { throw std::logic_error("unused"); }
	[[nodiscard]] const graphics::Texture2D& GetHeightMap() const override { throw std::logic_error("unused"); }
	[[nodiscard]] const graphics::FrameBuffer& GetFootprintFramebuffer() const override { throw std::logic_error("unused"); }
	[[nodiscard]] U16Extent2 GetIndexExtent() const override { throw std::logic_error("unused"); }
	[[nodiscard]] glm::mat4 GetOrthoView() const override { throw std::logic_error("unused"); }
	[[nodiscard]] glm::mat4 GetOrthoProj() const override { throw std::logic_error("unused"); }
	[[nodiscard]] Extent2 GetExtent() const override { throw std::logic_error("unused"); }
	uint8_t GetNoise(glm::u8vec2) override { throw std::logic_error("unused"); }

private:
	std::vector<uint8_t> _lookup;
	std::vector<std::vector<lnd::LNDCell>> _blocks;
};

void RunVolumeScenario(const std::string& name)
{
	const auto scenario = LoadScenario(name);
	const auto& expected = scenario.at("expected");

	std::vector<std::vector<lnd::LNDCell>> blocks;
	for (const auto& hex : scenario.at("blocks"))
	{
		const auto bytes = FromHex(hex.get<std::string>());
		auto& cells = blocks.emplace_back(bytes.size() / sizeof(lnd::LNDCell));
		std::memcpy(cells.data(), bytes.data(), bytes.size());
	}
	const SyntheticIsland island(FromHex(scenario.at("lookup").get<std::string>()), blocks);

	std::array<float, 15> infoValues {};
	for (size_t i = 0; i < infoValues.size(); ++i)
	{
		infoValues.at(i) = FloatFromBits(scenario.at("soundInfo").at(i));
	}
	GSoundInfo info {};
	static_assert(sizeof(info) == sizeof(infoValues));
	std::memcpy(&info, infoValues.data(), sizeof(info));

	SoundMap soundMap;
	const auto& queries = scenario.at("queries");
	for (size_t q = 0; q < queries.size(); ++q)
	{
		const auto& query = queries[q];
		const auto& weather = query.at("weather");
		const SoundMap::Inputs inputs {
		    .receiver = {FloatFromBits(query.at("x")), FloatFromBits(query.at("y")), FloatFromBits(query.at("z"))},
		    .weather =
		        {
		            .rain = weather.at(0).get<int8_t>(),
		            .snow = weather.at(1).get<int8_t>(),
		            .windX = weather.at(2).get<int8_t>(),
		            .windZ = weather.at(3).get<int8_t>(),
		        },
		    .skyType = FloatFromBits(query.at("sky")),
		};
		soundMap.Update(island, info, inputs);

		const auto& result = expected.at("soundMap").at(q);
		ASSERT_EQ(std::bit_cast<uint32_t>(soundMap.GetHeightAboveLand()), result.at("heightAboveLand").get<uint32_t>())
		    << "query " << q;
		for (size_t type = 0; type < k_AtmosTypeCount; ++type)
		{
			ASSERT_EQ(std::bit_cast<uint32_t>(soundMap.GetVolumes().at(type)), result.at("volumes").at(type).get<uint32_t>())
			    << "query " << q << " type " << k_AtmosTypeInfos.at(type).name;
		}
	}

	for (size_t i = 0; i < scenario.at("alignments").size(); ++i)
	{
		const auto alignment = FloatFromBits(scenario.at("alignments").at(i));
		ASSERT_EQ(std::bit_cast<uint32_t>(AtmosAudio::CalculateAlignmentValue(alignment)),
		          expected.at("alignments").at(i).get<uint32_t>())
		    << "alignment " << alignment;
	}

	for (size_t i = 0; i < scenario.at("sky").size(); ++i)
	{
		const auto& sky = scenario.at("sky").at(i);
		const auto& times = sky.at("times");
		const SkyInterface::DayNightTimes dayNight {
		    .nightFull = FloatFromBits(times.at(3)),
		    .duskStart = FloatFromBits(times.at(2)),
		    .duskEnd = FloatFromBits(times.at(1)),
		    .dayFull = FloatFromBits(times.at(0)),
		};
		ASSERT_EQ(std::bit_cast<uint32_t>(AtmosAudio::CalculateSkyType(FloatFromBits(sky.at("time")), dayNight)),
		          expected.at("sky").at(i).get<uint32_t>())
		    << "sky case " << i;
	}

	for (size_t i = 0; i < scenario.at("bankSteps").size(); ++i)
	{
		const auto& step = scenario.at("bankSteps").at(i);
		const auto& result = expected.at("bankSteps").at(i);
		for (size_t bank = 0; bank < k_AtmosTypeCount; ++bank)
		{
			const auto [current, sent] = AtmosAudio::StepBankVolume(FloatFromBits(step.at("current").at(bank)),
			                                                        FloatFromBits(step.at("targets").at(bank)));
			ASSERT_EQ(std::bit_cast<uint32_t>(current), result.at("current").at(bank).get<uint32_t>())
			    << "bank step " << i << " bank " << bank;
			ASSERT_EQ(sent, result.at("sent").at(bank).get<int32_t>()) << "bank step " << i << " bank " << bank;
		}
	}
}

void RunSchedulerScenario(const std::string& name)
{
	const auto scenario = LoadScenario(name);
	const auto time = scenario.at("time").get<uint32_t>();

	RecordingBackend backend;
	AtmosPlayer atmos(backend, [time] { return time; });

	std::vector<AtmosPlayer::BankId> banks;
	for (const auto& bank : scenario.at("banks"))
	{
		std::vector<pack::AudioBankSampleHeader> headers;
		for (const auto& hex : bank.at("headers"))
		{
			headers.push_back(HeaderFromHex(hex.get<std::string>()));
		}
		const auto bankName = bank.at("name").get<std::string>();
		backend.bankIndices[bankName] = banks.size();
		banks.push_back(atmos.RegisterBank(bankName, headers, bank.at("atmosCount").get<uint16_t>()));
	}

	uint32_t turn = 0;
	for (const auto& step : scenario.at("script"))
	{
		backend.turn = turn++;
		for (size_t i = 0; i < banks.size(); ++i)
		{
			atmos.SetGroup(banks[i], step.at("group").get<uint32_t>());
			atmos.SetBankVolume(banks[i], step.at("volumes").at(i).get<int32_t>());
		}
		atmos.Process(step.at("active").get<bool>());
	}

	auto expected = ByTurn(scenario.at("expected").get<std::vector<json>>());
	auto actual = ByTurn(backend.trace);
	for (uint32_t i = 0; i < turn; ++i)
	{
		ASSERT_EQ(expected[i], actual[i]) << "first difference at turn " << i;
	}
}
} // namespace

TEST(AtmosPlayer, MsvcRandomSequence)
{
	// srand(1) in the MSVC C runtime
	AudioRandom random;
	random.Seed(1);
	EXPECT_EQ(random.Next(), 41u);
	EXPECT_EQ(random.Next(), 18467u);
	EXPECT_EQ(random.Next(), 6334u);
	EXPECT_EQ(random.Next(), 26500u);
}

TEST(AtmosPlayer, MatchesOriginalScheduler1)
{
	RunSchedulerScenario("scheduler_1.json");
}

TEST(AtmosPlayer, MatchesOriginalScheduler2)
{
	RunSchedulerScenario("scheduler_2.json");
}

TEST(AtmosPlayer, MatchesOriginalScheduler3)
{
	RunSchedulerScenario("scheduler_3.json");
}

TEST(AtmosAudio, MatchesOriginalVolumes1)
{
	RunVolumeScenario("volumes_1.json");
}

TEST(AtmosAudio, MatchesOriginalVolumes2)
{
	RunVolumeScenario("volumes_2.json");
}
