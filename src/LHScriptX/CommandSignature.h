/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <functional>
#include <vector>

namespace openblack
{
class Game;
}

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
	ScriptCommandParameter(ParameterType type = ParameterType::None): _type(type) {}
	ScriptCommandParameter(const std::string value): _type(ParameterType::String)
	{
		// TODO: Hacky, avoid new
		_value._string = new std::string();
		*_value._string = value;
	}
	ScriptCommandParameter(float value): _type(ParameterType::Float) { SetFloat(value); }
	ScriptCommandParameter(int32_t value): _type(ParameterType::Number) { SetNumber(value); }
	ScriptCommandParameter(float x, float y, float z) { SetVector(x, y, z); }

	[[nodiscard]] ParameterType GetType() const { return _type; };

	[[nodiscard]] std::string& GetString() const { return *_value._string; }
	[[nodiscard]] float GetFloat() const { return _value._float; }
	[[nodiscard]] int32_t GetNumber() const { return _value._number; }
	void GetVector(float& x, float& y, float& z) const;

	void SetString(const std::string& value) { *_value._string = value; }
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
		std::string* _string;
		float _float;
		int32_t _number;
		float _vector[3];
	} _value;
};

typedef std::vector<ScriptCommandParameter> ScriptCommandParameters;

class ScriptCommandContext
{
public:
	ScriptCommandContext(Game* game, const ScriptCommandParameters* parameters): _game(game), _parameters(parameters) {}

	[[nodiscard]] Game& GetGame() const { return *_game; }

	[[nodiscard]] const ScriptCommandParameters& GetParameters() const { return *_parameters; }

	const ScriptCommandParameter& operator[](unsigned int arg) const { return _parameters->at(arg); }

	template <class T>
	T GetParameter(unsigned int arg) const;

private:
	Game* _game;
	const ScriptCommandParameters* _parameters;
};

typedef std::function<void(const ScriptCommandContext&)> ScriptCommand;

struct ScriptCommandSignature
{
	const char name[128];
	ScriptCommand command;
	std::vector<ParameterType> parameters;
};
} // namespace openblack::lhscriptx
