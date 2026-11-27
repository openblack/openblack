/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <limits>

#include <AL/al.h>
#include <AL/alc.h>
#include <Audio/AudioManagerInterface.h>
#include <CHLApi.h>
#include <ECS/Components/Transform.h>
#include <ECS/Registry.h>
#include <Game.h>
#include <LHVM.h>
#include <Locator.h>
#include <PackFile.h>
#include <Resources/ResourcesInterface.h>
#include <gtest/gtest.h>

namespace
{
using namespace openblack;
using ecs::components::AudioEmitter;
using ecs::components::Transform;

class ScriptSoundTest: public ::testing::Test
{
protected:
	void SetUp() override
	{
		Arguments args {.graphicsBackend = GraphicsBackend::Noop,
		                .gamePath = (std::filesystem::path(TEST_BINARY_DIR) / "mock").string(),
		                .logFile = "stdout"};
		args.logLevels.fill(spdlog::level::critical);
		_game = std::make_unique<Game>(std::move(args));
		ASSERT_TRUE(_game->Initialize());
	}

	static void Call(int sample, int bank, glm::vec3 point, bool positional)
	{
		auto& vm = Locator::vm::value();
		vm.Pushi(736);
		vm.Pushi(sample);
		vm.Pushi(bank);
		vm.Pushv(point.x);
		vm.Pushv(point.y);
		vm.Pushv(point.z);
		vm.Pushb(positional);
		Locator::chlapi::value().GetFunctionsTable().at(43).impl();
		lhvm::DataType type {};
		EXPECT_EQ(vm.Pop(type).intVal, 736);
		EXPECT_EQ(type, lhvm::DataType::Int);
	}

	static std::vector<uint8_t> Wave()
	{
		// One second of 8 kHz, 16-bit mono PCM.
		std::vector<uint8_t> wave {'R', 'I', 'F', 'F', 0xa4, 0x3e, 0,   0,   'W', 'A',  'V',  'E',  'f', 'm',  't',
		                           ' ', 16,  0,   0,   0,    1,    0,   1,   0,   0x40, 0x1f, 0,    0,   0x80, 0x3e,
		                           0,   0,   2,   0,   16,   0,    'd', 'a', 't', 'a',  0x80, 0x3e, 0,   0};
		wave.resize(wave.size() + 16000, 0x14);
		return wave;
	}

	static entt::id_type LoadSample(const std::string& bank, uint32_t sample, const std::vector<uint8_t>& bytes,
	                                pack::AudioBankSampleHeader header = {})
	{
		auto& audio = Locator::audio::value();
		audio.CreateSoundGroup(bank);
		const auto id = resources::HashIdentifier(fmt::format("{}/{}", bank, sample));
		header.id = static_cast<int32_t>(sample);
		header.sampleRate = 8000;
		Locator::resources::value().GetSounds().Load(id, resources::SoundLoader::FromBufferTag {}, header,
		                                             std::vector<std::vector<uint8_t>> {bytes});
		audio.AddToSoundGroup(bank, id);
		return id;
	}

	std::unique_ptr<Game> _game;
};

TEST_F(ScriptSoundTest, PlaysBankSampleAtListenerOrAtFixedWorldPoint)
{
	if (alcGetCurrentContext() == nullptr)
	{
		GTEST_SKIP() << "No OpenAL device";
	}
	auto& registry = Locator::entitiesRegistry::value();
	auto& audio = Locator::audio::value();
	const auto id = LoadSample("InGaMe.sad", 17, Wave());
	const auto editorId = LoadSample("editor.sad", 17, Wave());
	const glm::vec3 position {9, 23, 41};
	Call(17, 1, {0, std::numeric_limits<float>::quiet_NaN(), 0}, true);
	EXPECT_EQ(registry.Size<AudioEmitter>(), 0u);
	Call(17, 1, position, false);
	ASSERT_EQ(registry.Size<AudioEmitter>(), 1u);
	const auto& sound = audio.GetSound(id);
	ASSERT_NE(sound.bufferId, 0u);
	EXPECT_EQ(sound.sizeInBytes, 16000u);
	EXPECT_FLOAT_EQ(sound.duration, 1.0f);
	const auto buffer = sound.bufferId;
	registry.Each<AudioEmitter, Transform>([&](entt::entity entity, const AudioEmitter& emitter, const Transform& transform) {
		EXPECT_EQ(emitter.soundId, id);
		EXPECT_EQ(audio.GetStatus(entity), audio::AudioStatus::Playing);
		EXPECT_TRUE(emitter.relative);
		EXPECT_EQ(transform.position, glm::vec3(0));
	});
	Call(17, 1, position, true);
	ASSERT_EQ(registry.Size<AudioEmitter>(), 1u);
	EXPECT_EQ(audio.GetSound(id).bufferId, buffer);
	registry.Each<AudioEmitter, Transform>([&](const AudioEmitter& emitter, const Transform& transform) {
		EXPECT_FALSE(emitter.relative);
		EXPECT_EQ(emitter.position, position);
		EXPECT_EQ(transform.position, position);
	});
	Call(17, 2, {}, false);
	EXPECT_EQ(registry.Size<AudioEmitter>(), 2u);
	EXPECT_NE(audio.GetSound(editorId).bufferId, buffer);
}

TEST_F(ScriptSoundTest, MissingBanksSamplesAndUnplayableDataLeaveCallerIntact)
{
	LoadSample("InGame.sad", 17, {0, 1, 2, 3});
	Call(17, 0, {}, true);
	Call(17, -1, {}, true);
	Call(17, 10, {}, false);
	Call(0, 1, {}, false);
	Call(16, 1, {}, false);
	Call(17, 1, {}, false);
	EXPECT_EQ(Locator::entitiesRegistry::value().Size<AudioEmitter>(), 0u);
}

TEST_F(ScriptSoundTest, KeepsBankGainAndPitchDuringAnUpdate)
{
	if (alcGetCurrentContext() == nullptr)
	{
		GTEST_SKIP() << "No OpenAL device";
	}
	pack::AudioBankSampleHeader header {};
	header.overrideFlags = 0x21;
	header.pitch = 125;
	header.volume = 64;
	LoadSample("InGame.sad", 18, Wave(), header);
	auto& registry = Locator::entitiesRegistry::value();
	auto& audio = Locator::audio::value();
	audio.SetSfxVolume(0.5f);
	Call(18, 1, {}, false);
	ASSERT_EQ(registry.Size<AudioEmitter>(), 1u);
	const auto source = registry.Get<AudioEmitter>(registry.Front<AudioEmitter>()).sourceId;
	const auto checkParameters = [source]() {
		ALfloat pitch {};
		ALfloat gain {};
		alGetSourcef(source, AL_PITCH, &pitch);
		alGetSourcef(source, AL_GAIN, &gain);
		EXPECT_FLOAT_EQ(pitch, 1.25f);
		EXPECT_FLOAT_EQ(gain, 32.0f / 127.0f);
	};
	checkParameters();
	audio.Update();
	checkParameters();
}
} // namespace
