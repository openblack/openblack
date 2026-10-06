/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdlib>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <ParticleFile.h>
#include <gtest/gtest.h>

#include "Common/Zip.h"

using namespace openblack::psys;

namespace
{
/// A made up file with one property of every type, with the editor's line endings
constexpr std::string_view k_Sample = "BEGINPROPERTIES\r\n"
                                      "PROPERTY DeleteOnCloseDown BOOL 0\r\n"
                                      "PROPERTY Hierarchies ARRAY SIZE 3 1 0 1 \r\n"
                                      "PROPERTY MaxSpellAge FLOAT -1\r\n"
                                      "ENDPROPERTIES\r\n"
                                      "BEGINCLASS DiskEmitter Emitter0\r\n"
                                      "BEGINPROPERTIES\r\n"
                                      "PROPERTY Condition PERSIS_PNTR NULL_STRING\r\n"
                                      "PROPERTY EmissionFreq FLOAT 2.5\r\n"
                                      "PROPERTY Maximum FLOAT 1e+006\r\n"
                                      "PROPERTY Group INTEGER 1\r\n"
                                      "PROPERTY PCreator PERSIS_PNTR Sprite0\r\n"
                                      "PROPERTY KeyPoints ARRAY SIZE 4 0 0.5 1.25 2 \r\n"
                                      "PROPERTY MeshEnum ENUM MSH_INVALID\r\n"
                                      "PROPERTY SoundEmission SOUND_ACTION SOUND_FIZZ LOOPING 1 ONLYONE 0 SOFTRELEASE 1 "
                                      "USESURFACE 0\r\n"
                                      "ENDPROPERTIES\r\n"
                                      "ENDCLASS\r\n"
                                      "BEGINCLASS ParticleSpriteCreator Sprite0\r\n"
                                      "BEGINPROPERTIES\r\n"
                                      "PROPERTY TextureFileName STRING .\\Data\\Textures\\S_SpriteSheet3.raw\r\n"
                                      "ENDPROPERTIES\r\n"
                                      "ENDCLASS\r\n"
                                      "BEGINCLASS ParticleSpriteCreator Sprite0\r\n"
                                      "BEGINPROPERTIES\r\n"
                                      "PROPERTY InitialScale FLOAT 9\r\n"
                                      "ENDPROPERTIES\r\n"
                                      "ENDCLASS\r\n";
} // namespace

TEST(ParticleFile, ParsesTheHeaderAndObjects)
{
	const auto file = ParticleFile::Parse(k_Sample);
	ASSERT_TRUE(file.has_value());
	EXPECT_FALSE(file->header.Bool("DeleteOnCloseDown", true));
	EXPECT_EQ(file->header.IntArray("Hierarchies"), (std::vector<int> {1, 0, 1}));
	EXPECT_FLOAT_EQ(file->header.Float("MaxSpellAge", 0.0f), -1.0f);
	ASSERT_EQ(file->objects.size(), 3u);
	EXPECT_EQ(file->objects[0].className, "DiskEmitter");
	EXPECT_EQ(file->objects[0].name, "Emitter0");
}

TEST(ParticleFile, ReadsEveryPropertyType)
{
	const auto file = ParticleFile::Parse(k_Sample);
	ASSERT_TRUE(file.has_value());
	const auto& emitter = file->objects[0];
	// NULL_STRING is no reference at all
	EXPECT_TRUE(emitter.Has("Condition"));
	EXPECT_EQ(emitter.String("Condition"), "");
	EXPECT_EQ(emitter.String("PCreator"), "Sprite0");
	EXPECT_FLOAT_EQ(emitter.Float("EmissionFreq", 0.0f), 2.5f);
	EXPECT_FLOAT_EQ(emitter.Float("Maximum", 0.0f), 1e6f);
	EXPECT_EQ(emitter.Int("Group", -1), 1);
	// Ints widen to floats and floats truncate to ints
	EXPECT_FLOAT_EQ(emitter.Float("Group", 0.0f), 1.0f);
	EXPECT_EQ(emitter.Int("EmissionFreq", 0), 2);
	// Arrays keep their fractions
	const auto keys = emitter.FloatArray("KeyPoints");
	ASSERT_EQ(keys.size(), 4u);
	EXPECT_FLOAT_EQ(keys[2], 1.25f);
	EXPECT_EQ(emitter.String("MeshEnum"), "MSH_INVALID");
	const auto sound = emitter.Sound("SoundEmission");
	EXPECT_EQ(sound.sound, "SOUND_FIZZ");
	EXPECT_TRUE(sound.looping);
	EXPECT_FALSE(sound.onlyOne);
	EXPECT_TRUE(sound.softRelease);
	EXPECT_FALSE(sound.useSurface);
}

TEST(ParticleFile, MissingPropertiesTakeTheFallback)
{
	const auto file = ParticleFile::Parse(k_Sample);
	ASSERT_TRUE(file.has_value());
	const auto& emitter = file->objects[0];
	EXPECT_EQ(emitter.Int("Nothing", 7), 7);
	EXPECT_TRUE(emitter.Bool("Nothing", true));
	EXPECT_TRUE(emitter.IntArray("Nothing").empty());
	EXPECT_EQ(emitter.Sound("Nothing").sound, "NO_SOUND");
}

TEST(ParticleFile, FindsTheFirstOfARepeatedName)
{
	const auto file = ParticleFile::Parse(k_Sample);
	ASSERT_TRUE(file.has_value());
	const auto* sprite = file->Find("Sprite0");
	ASSERT_NE(sprite, nullptr);
	EXPECT_TRUE(sprite->Has("TextureFileName"));
	EXPECT_EQ(file->Find(""), nullptr);
	EXPECT_EQ(file->Find("Nobody"), nullptr);
}

TEST(ParticleFile, RejectsWhatIsNotOne)
{
	EXPECT_FALSE(ParticleFile::Parse("").has_value());
	EXPECT_FALSE(ParticleFile::Parse("hello").has_value());
	// An unknown type, an unfinished block, a bad number and a short array
	EXPECT_FALSE(ParticleFile::Parse("BEGINPROPERTIES PROPERTY A COLOUR 1 ENDPROPERTIES").has_value());
	EXPECT_FALSE(ParticleFile::Parse("BEGINPROPERTIES ENDPROPERTIES BEGINCLASS A B BEGINPROPERTIES ENDPROPERTIES").has_value());
	EXPECT_FALSE(ParticleFile::Parse("BEGINPROPERTIES PROPERTY A INTEGER x ENDPROPERTIES").has_value());
	EXPECT_FALSE(ParticleFile::Parse("BEGINPROPERTIES PROPERTY A ARRAY SIZE 3 1 2").has_value());
	// A header alone is a file with no objects
	const auto empty = ParticleFile::Parse("BEGINPROPERTIES ENDPROPERTIES");
	ASSERT_TRUE(empty.has_value());
	EXPECT_TRUE(empty->objects.empty());
}

TEST(ParticleFile, SplitsTheCompressedForm)
{
	const std::vector<uint8_t> data {0x34, 0x12, 0x00, 0x00, 0x78, 0x9C};
	const auto compressed = SplitCompressed(data);
	ASSERT_TRUE(compressed.has_value());
	EXPECT_EQ(compressed->textSize, 0x1234u);
	EXPECT_EQ(compressed->deflated.size(), 2u);
	EXPECT_FALSE(SplitCompressed(std::vector<uint8_t> {1, 2, 3, 4}).has_value());
}

TEST(ParticleFile, ReadsEveryFileOfTheGame)
{
	const char* gamePath = std::getenv("OPENBLACK_GAME_PATH");
	if (gamePath == nullptr)
	{
		GTEST_SKIP() << "OPENBLACK_GAME_PATH is not set";
	}
	const auto directory = std::filesystem::path(gamePath) / "Data" / "Spells" / "ZSpellFiles";
	if (!std::filesystem::is_directory(directory))
	{
		GTEST_SKIP() << "No particle files in the game folder";
	}
	size_t count = 0;
	for (const auto& entry : std::filesystem::directory_iterator(directory))
	{
		if (!entry.path().filename().string().ends_with("_txt.zzz"))
		{
			continue;
		}
		std::ifstream stream(entry.path(), std::ios::binary);
		const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
		const auto compressed = SplitCompressed(bytes);
		ASSERT_TRUE(compressed.has_value()) << entry.path();
		const auto text = openblack::zip::Inflate(
		    std::vector<uint8_t>(compressed->deflated.begin(), compressed->deflated.end()), compressed->textSize);
		const auto file = ParticleFile::Parse(std::string_view(reinterpret_cast<const char*>(text.data()), text.size()));
		ASSERT_TRUE(file.has_value()) << entry.path();
		EXPECT_EQ(file->header.IntArray("InitiallyCreated").size(), 25u) << entry.path();
		++count;
	}
	EXPECT_GT(count, 100u);
}
