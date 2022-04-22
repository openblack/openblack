/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "StringUtils.h"

bool openblack::string_utils::EndsWith(const std::string& string, const std::string& ending)
{
	if (string.length() < ending.length())
	{
		return false;
	}
	return string.compare(string.length() - ending.length(), ending.length(), ending) == 0;
}

bool openblack::string_utils::BeginsWith(const std::string& string, const std::string& beginning)
{
	if (string.length() < beginning.length())
	{
		return false;
	}
	return string.compare(0, beginning.length(), beginning) == 0;
}

std::string openblack::string_utils::UpperCase(const std::string& string)
{
	auto uppercase = string;
	std::transform(uppercase.begin(), uppercase.end(), uppercase.begin(),
	               [](auto c) { return static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(c))); });
	return uppercase;
}

std::string openblack::string_utils::LowerCase(const std::string& string)
{
	auto lowercase = string;
	std::transform(lowercase.begin(), lowercase.end(), lowercase.begin(),
	               [](auto c) { return static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(c))); });
	return lowercase;
}

std::string openblack::string_utils::Capitalise(const std::string& string)
{
	auto capitalised = string;
	capitalised[0] = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(string[0])));
	return capitalised;
}
