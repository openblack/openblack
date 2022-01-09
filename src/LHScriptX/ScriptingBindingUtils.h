/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "CommandSignature.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace openblack::lhscriptx
{

template <class T>
static ParameterType ParameterTypeStaticLookUpRegular;

template <>
inline ParameterType ParameterTypeStaticLookUpRegular<glm::vec3> = ParameterType::Vector;

template <>
inline ParameterType ParameterTypeStaticLookUpRegular<const std::string&> = ParameterType::String;

template <>
inline ParameterType ParameterTypeStaticLookUpRegular<float> = ParameterType::Float;

template <>
inline ParameterType ParameterTypeStaticLookUpRegular<int32_t> = ParameterType::Number;

template <typename T>
inline typename std::enable_if_t<!std::is_enum<T>::value, ParameterType> GetParamType()
{
	return ParameterTypeStaticLookUpRegular<T>;
}

template <typename T>
inline typename std::enable_if_t<std::is_enum<T>::value, ParameterType> GetParamType()
{
	return ParameterTypeStaticLookUpRegular<std::underlying_type_t<T>>;
}

template <class T>
T GetParamValueRegular(const ScriptCommandContext& ctx, int index);

template <>
inline const std::string& GetParamValueRegular(const ScriptCommandContext& ctx, int index)
{
	return ctx[index].GetString();
}

template <>
inline float GetParamValueRegular(const ScriptCommandContext& ctx, int index)
{
	return ctx[index].GetFloat();
}

template <>
inline int32_t GetParamValueRegular(const ScriptCommandContext& ctx, int index)
{
	return ctx[index].GetNumber();
}

template <typename T>
inline typename std::enable_if_t<!std::is_enum<T>::value, T> GetParamValue(const ScriptCommandContext& ctx, int index)
{
	return GetParamValueRegular<T>(ctx, index);
}

template <typename T>
inline typename std::enable_if_t<std::is_enum<T>::value, T> GetParamValue(const ScriptCommandContext& ctx, int index)
{
	return static_cast<T>(GetParamValueRegular<std::underlying_type_t<T>>(ctx, index));
}

/// Base case which is triggered when RemainingTypes is void (see remaining non-parsed params case)
template <typename... ParsedParamTypes, typename... ArgTypes>
void InvokeCallableFromContext([[maybe_unused]] const ScriptCommandContext& ctx, [[maybe_unused]] int paramIndex,
                               /// This case is why the original function is passed along
                               [[maybe_unused]] std::function<void(ArgTypes...)> originalFunction,
                               /// Empty working function type which means all the parameter types have been extracted
                               const std::function<void()>&,
                               /// All the parameter types extracted at compile time with all the parameter values at runtime
                               [[maybe_unused]] ParsedParamTypes... params)
{
	originalFunction(std::forward<ParsedParamTypes>(params)...);
}

/// Remaining non-parsed params case
template <typename PoppedArgType, typename... ArgTypes, typename... RemainingTypes, typename... ParsedParamTypes,
          typename = std::enable_if_t<!std::is_array<PoppedArgType>::value>>
void InvokeCallableFromContext(
    /// Pass along the script context which contains runtime parameter values
    const ScriptCommandContext& ctx,
    /// Keep track of which runtime parameter to fetch from context
    int paramIndex,
    /// Keep track of the original function for later call
    std::function<void(ArgTypes...)> originalFunction,
    /// Template declaration of function extracts from RemainingTypes a PoppedArgType from the left
    /// RemainingTypes will have one value taken at every recursive template call to InvokeCallableFromContext
    /// PoppedArgType is used with paramIndex to get the runtime value from the context parameter list
    std::function<void(PoppedArgType, RemainingTypes...)>,
    /// Growing list of parameters whose types have been extracted by the template and whose values have been extracted from ctx
    /// In the end, all types from RemainingTypes will be transferred to ParsedParamTypes, at which point the recursion ends
    ParsedParamTypes... params)
{
	std::function<void(RemainingTypes...)> remainingTypesFunction;
	InvokeCallableFromContext(ctx,
	                          // Increment the parameter index for the next call to this function to fetch the right index
	                          paramIndex + 1,
	                          // Keep track of the original function for later call
	                          originalFunction,
	                          // List of remaining parameter types (popped from the left)
	                          remainingTypesFunction,
	                          // Growing list of parameter values taken from previous template-calls
	                          std::forward<ParsedParamTypes>(params)...,
	                          // Get parameter from the scripting context and append it to template-parsed parameters for call
	                          // above If the template fails, it could be that the template ScriptCommandContext::GetParameter
	                          // is undefined for that particular type
	                          GetParamValue<PoppedArgType>(ctx, paramIndex));
}

/// Helper entry point to extract the parameter types and values
template <typename... ArgTypes>
void InvokeCallableFromContext(const ScriptCommandContext& ctx, std::function<void(ArgTypes...)> fn)
{
	InvokeCallableFromContext(ctx, 0, fn, fn);
}

/// Base case
constexpr void GetScriptCommandParameters([[maybe_unused]] std::array<ParameterType, 9>& parameters, int, void (*)()) {}

/// Non-base case, fetch ParameterType
template <typename PoppedArgType, typename... RemainingTypes>
constexpr void GetScriptCommandParameters(std::array<ParameterType, 9>& parameters, int index,
                                          void (*)(PoppedArgType, RemainingTypes...))
{
	void (*remainingTypesFunction)(RemainingTypes...) = nullptr;
	parameters[index] = GetParamType<PoppedArgType>();
	GetScriptCommandParameters(parameters, index + 1, remainingTypesFunction);
}

/// Helper entry point to extract the parameter types
template <typename... ArgTypes>
constexpr std::array<ParameterType, 9> GetScriptCommandParameters(void (&fn)(ArgTypes...))
{
	std::array<ParameterType, 9> parameters = {};
	GetScriptCommandParameters(parameters, 0, &fn);
	return parameters;
}

#define CREATE_COMMAND_BINDING(NAME, FUNCTION)                                                                  \
	{                                                                                                           \
		NAME, [](const ScriptCommandContext& ctx) { InvokeCallableFromContext(ctx, std::function(FUNCTION)); }, \
		    GetScriptCommandParameters(FUNCTION)                                                                \
	}

} // namespace openblack::lhscriptx
