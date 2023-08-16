/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <functional>
#include <string>
#include <vector>

namespace openblack::lhscriptx
{
enum class ParameterType
{
	None = 0,
	String, // L
	Float,  // F
	Number, // N
	Vector  // A
};

class ScriptCommandParameter
{
public:
	explicit ScriptCommandParameter(ParameterType type = ParameterType::None)
	    : _type(type)
	{
	}
	explicit ScriptCommandParameter(const std::string& value)
	    : _type(ParameterType::String)
	{
		_string = value;
	}
	explicit ScriptCommandParameter(float value)
	    : _type(ParameterType::Float)
	{
		SetFloat(value);
	}
	explicit ScriptCommandParameter(int32_t value)
	    : _type(ParameterType::Number)
	{
		SetNumber(value);
	}
	ScriptCommandParameter(float x, float y, float z)
	    : _type(ParameterType::Vector)
	{
		SetVector(x, y, z);
	}

	[[nodiscard]] ParameterType GetType() const { return _type; };

	[[nodiscard]] const std::string& GetString() const { return _string; }
	[[nodiscard]] float GetFloat() const { return _value._float; }
	[[nodiscard]] int32_t GetNumber() const { return _value._number; }
	void GetVector(float& x, float& y, float& z) const
	{
		x = _value._vector[0];
		y = _value._vector[1];
		z = _value._vector[2];
	}

	void SetString(const std::string& value) { _string = value; }
	void SetFloat(float value) { _value._float = value; };
	void SetNumber(int32_t value) { _value._number = value; }
	void SetVector(float x, float y, float z)
	{
		_value._vector[0] = x;
		_value._vector[1] = y;
		_value._vector[2] = z;
	}

private:
	ParameterType _type;

	union
	{
		// NOLINTNEXTLINE(readability-identifier-naming): float is a reserved name
		float _float;
		// NOLINTNEXTLINE(readability-identifier-naming)
		int32_t _number;
		// NOLINTNEXTLINE(modernize-avoid-c-arrays, readability-identifier-naming): array won't work in union
		float _vector[3];
	} _value;
	std::string _string;
};

using ScriptCommandParameters = std::vector<ScriptCommandParameter>;

using ScriptCommand = std::function<void(const ScriptCommandParameters&)>;

struct ScriptCommandSignature
{
	const std::array<char, 0x80> name;
	const ScriptCommand command;
	const std::array<ParameterType, 9> parameters;
};
} // namespace openblack::lhscriptx
