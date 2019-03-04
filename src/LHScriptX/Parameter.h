/* OpenBlack - A reimplementation of Lionheads's Black & White engine
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef OPENBLACK_LHSCRIPTX_PARAMTER_H
#define OPENBLACK_LHSCRIPTX_PARAMTER_H

#include <Common/Types.h>

#include <map>
#include <vector>

namespace OpenBlack {
	namespace LHScriptX {
		/** The different types that a variable can be in command arguments */
		enum Type {
			TypeNone = 0,
			TypeString,     // L
			TypeFloat,		// F
			TypeNumber,		// N
			TypeVector		// A
		};

		class ScriptParameter {
		public:
			ScriptParameter(Type type = TypeNone) :_type(type) {}
			ScriptParameter(const std::string &value) : _type(TypeString) { SetString(value); }
			ScriptParameter(float value) : _type(TypeFloat) { SetFloat(value); }
			ScriptParameter(int32_t value) : _type(TypeNumber) { SetNumber(value); }
			ScriptParameter(float x, float y, float z) { SetVector(x, y, z); }

			Type GetType() const { return _type; };

			std::string &GetString() const { return *_value._string; }
			float GetFloat() const { return _value._float; }
			int32_t GetNumber() const { return _value._number; }
			void GetVector(float &x, float &y, float &z) const;

			void SetString(const std::string &value) { *_value._string = value; }
			void SetFloat(float value) { _value._float = value; };
			void SetNumber(int32_t value) { _value._number = value; }
			void SetVector(float x, float y, float z)
			{
				_value._vector[0] = x;
				_value._vector[1] = y;
				_value._vector[2] = z;
			}
		private:
			Type _type;

			union {
				std::string *_string;
				float _float;
				int32_t _number;
				float _vector[3];
			} _value;
		};

		typedef std::vector<ScriptParameter> ScriptParameters;
	}
}

#endif
