#pragma once
#ifndef OPENBLACK_COMMON_CMDLINEARGS_H
#define OPENBLACK_COMMON_CMDLINEARGS_H

#include <vector>
#include <sstream>
#include <string>

namespace OpenBlack
{
	class CmdLineArgs {
	public:
		struct ArgValue
		{
			ArgValue() {}

			bool AsBool() const
			{
				// assume everything but 0 and false is true
				if (value == "0") return false;
				else if (value == "false") return false;
				else return true;
			}

			template <typename T>
			T ValueAs() const
			{
				T v;
				std::istringstream stream(value);
				stream >> v;

				return v;
			}

			std::string value;
		};

		typedef std::vector<ArgValue> ArgValues;

		CmdLineArgs() {}
		CmdLineArgs(int argc, char* argv[]);

		bool Has(const std::string& key) const;
		ArgValue Get(const std::string& key) const;
		ArgValues GetArgValues(const std::string& key) const;

		// convenience function
		template <typename T>
		void Get(const std::string& name, T& v) const
		{
			// could avoid this extra find but there's a problem using find_if here on linux
			if (Has(name))
			{
				v = Get(name).ValueAs<T>();
			}
		}
	private:
		template<typename T>
		struct FirstEquals
		{
			FirstEquals(const T& x) : x(x) {}
			template<typename S> bool operator()(const S& v) { return v.first == x; }
			T x;
		};

		typedef std::pair<std::string, ArgValue> ArgValuePair;
		typedef std::vector<ArgValuePair> Args;

		Args _args;
	};

	// template specialization for strings because the value is already a string, it doesn't need to go through a stream
	template <>
	inline std::string CmdLineArgs::ArgValue::ValueAs<std::string>() const { return value; }

	// template specialization for bools because I'm not sure if getting a bool from stream would work correctly
	template <>
	inline bool CmdLineArgs::ArgValue::ValueAs<bool>() const { return AsBool(); }
}

#endif
