# Lists the script API's natives that openblack hasn't written yet, for the editor's Scripts panel.
#
# A native is unwritten while its function's body still logs that it is not implemented. The natives are numbered by
# the order the API binds them in, so the list follows the source as natives get written.
#
# Usage: cmake -DSOURCE=<CHLApi.cpp> -DOUTPUT=<header> -P GenerateUnimplementedNatives.cmake

# Run as a script, it gets the policies of this version rather than the oldest, for if (IN_LIST)
cmake_minimum_required(VERSION 3.18)

file(STRINGS "${SOURCE}" lines)

set(stubs "")
set(current "")
set(bindings "")
foreach (line IN LISTS lines)
  if (line MATCHES "^void ([A-Za-z0-9_]+)\\(\\)")
    set(current "${CMAKE_MATCH_1}")
  elseif (line MATCHES "^}")
    set(current "")
  elseif (NOT current STREQUAL "" AND line MATCHES "not implemented\\.")
    list(APPEND stubs "${current}")
    set(current "")
  endif ()
  if (line
      MATCHES
      "CREATE_FUNCTION_BINDING\\(\"[A-Z0-9_]+\", *-?[0-9]+, *-?[0-9]+, *([A-Za-z0-9_]+)\\)"
  )
    list(APPEND bindings "${CMAKE_MATCH_1}")
  endif ()
endforeach ()

set(numbers "")
set(index 0)
set(count 0)
foreach (function IN LISTS bindings)
  if (function IN_LIST stubs)
    string(APPEND numbers "    ${index},\n")
    math(EXPR count "${count} + 1")
  endif ()
  math(EXPR index "${index} + 1")
endforeach ()

set(content "")
string(
  APPEND
  content
  "// Generated from the script API's source by GenerateUnimplementedNatives.cmake: do not edit.\n"
)
string(APPEND content
       "#pragma once\n\n#include <array>\n#include <cstdint>\n\n"
)
string(APPEND content "namespace openblack::editor::scripts\n{\n")
string(
  APPEND content
  "/// The numbers of the natives whose functions are still stubs, in order\n"
)
string(APPEND content
       "constexpr std::array<uint32_t, ${count}> k_UnimplementedNatives {{\n"
)
string(APPEND content "${numbers}")
string(APPEND content "}};\n")
string(APPEND content "/// How many natives the script API binds\n")
string(APPEND content "constexpr uint32_t k_NativeCount = ${index};\n")
string(APPEND content "} // namespace openblack::editor::scripts\n")
file(WRITE "${OUTPUT}.tmp" "${content}")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OUTPUT}.tmp" "${OUTPUT}"
)
file(REMOVE "${OUTPUT}.tmp")
