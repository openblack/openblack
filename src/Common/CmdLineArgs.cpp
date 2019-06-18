/* OpenBlack - A reimplementation of Lionhead's Black & White.
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

#include "CmdLineArgs.h"
#include <algorithm>

using namespace OpenBlack;

CmdLineArgs::CmdLineArgs(int argc, char* argv[])
{
	// leave out the first argument, it's not needed
	for (int i = 1; i < argc; ++i)
	{
		std::string argString(argv[i]);

		if (argString.empty())
		{
			// would this ever happen? why would an arg be empty?
			continue;
		}

		// args are prefixed with -
		if (argString[0] == '-')
		{
			_args.push_back(std::make_pair(argString.substr(1), ArgValue()));
		}
		// if an arg wasn't prefixed with - assume this is the last args value, if there is any args
		else if (!_args.empty())
		{
			_args.back().second.value = argString;
		}
	}
}

bool CmdLineArgs::Has(const std::string& name) const
{
	return find_if(_args.begin(), _args.end(), FirstEquals<std::string>(name)) != _args.end();
}

CmdLineArgs::ArgValue CmdLineArgs::Get(const std::string& name) const
{
	Args::const_iterator it = find_if(_args.begin(), _args.end(), FirstEquals<std::string>(name));

	// return an empty arg value if an arg by this name doesn't exist
	return (it != _args.end()) ? it->second : ArgValue();
}

CmdLineArgs::ArgValues CmdLineArgs::GetArgValues(const std::string& name) const
{
	ArgValues argValues;

	for (Args::const_iterator it = _args.begin();;)
	{
		it = find_if(it, _args.end(), FirstEquals<std::string>(name));
		if (it == _args.end())
			break;

		argValues.push_back(it->second);

		++it;
	}

	return argValues;
}
