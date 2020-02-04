# Setup a custom clang-format target (if clang-format can be found) that will run
# against all the src files. This should be used before making a pull request.
# =======================================================================

# The clang-format tool is installed under a variety of different names.  Try
# to find a sensible one.  Only look for versions 6 to 8 explicitly - we don't
# know whether our clang-format file will work with newer versions of the
# tool
find_program(CLANG_FORMAT NAMES
  clang-format-8
  clang-format-7.0
  clang-format-6.0
  clang-format70
  clang-format60
  clang-format)
if (CLANG_FORMAT)
  set(SRCS ${PROJECT_SOURCE_DIR}/src)
  set(COMPS ${PROJECT_SOURCE_DIR}/components)
  set(APPS ${PROJECT_SOURCE_DIR}/apps)
  set(CCOMMENT "Running clang format against all the .h and .cpp files in src/")
  if (WIN32)
    add_custom_target(clang-format
      COMMAND powershell.exe -Command "Get-ChildItem '${SRCS}/*' -Include *.cpp,*.h -Recurse | Foreach {&'${CLANG_FORMAT}' -i $_.fullname}"
      COMMAND powershell.exe -Command "Get-ChildItem '${COMPS}/*' -Include *.cpp,*.h -Recurse | Foreach {&'${CLANG_FORMAT}' -i $_.fullname}"
      COMMAND powershell.exe -Command "Get-ChildItem '${APPS}/*' -Include *.cpp,*.h -Recurse | Foreach {&'${CLANG_FORMAT}' -i $_.fullname}"
      COMMENT ${CCOMMENT})
  elseif(MINGW)
    add_custom_target(clang-format
      COMMAND find `cygpath -u ${SRCS}` -iname *.h -o -iname *.cpp | xargs `cygpath -u ${CLANG_FORMAT}` -i
      COMMAND find `cygpath -u ${COMPS}` -iname *.h -o -iname *.cpp | xargs `cygpath -u ${CLANG_FORMAT}` -i
      COMMAND find `cygpath -u ${APPS}` -iname *.h -o -iname *.cpp | xargs `cygpath -u ${CLANG_FORMAT}` -i
      COMMENT ${CCOMMENT})
  else()
    add_custom_target(clang-format
      COMMAND find ${SRCS} -iname *.h -o -iname *.cpp | xargs ${CLANG_FORMAT} -i
      COMMAND find ${COMPS} -iname *.h -o -iname *.cpp | xargs ${CLANG_FORMAT} -i
      COMMAND find ${APPS} -iname *.h -o -iname *.cpp | xargs ${CLANG_FORMAT} -i
      COMMENT ${CCOMMENT})
  endif()
  unset(SRCS)
  unset(CCOMMENT)
else()
  message(WARNING "Not generating clang-format target, no clang-format tool found")
endif()
