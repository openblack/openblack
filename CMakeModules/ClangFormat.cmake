# https://github.com/yuzu-emu/yuzu/blob/f190547bbb930dacb3a7ee01784188f3bfe375ce/CMakeLists.txt#L769-L815

# Setup a custom clang-format target (if clang-format can be found) that will run
# against all the src files. This should be used before making a pull request.
# =======================================================================

set(CLANG_FORMAT_POSTFIX "-12")
find_program(CLANG_FORMAT
    NAMES clang-format${CLANG_FORMAT_POSTFIX}
          clang-format
    PATHS ${PROJECT_BINARY_DIR}/externals
          /usr/local/opt/llvm/bin) # macos
# if find_program doesn't find it, try to download from externals
if (NOT CLANG_FORMAT)
    if (WIN32 AND NOT CMAKE_CROSSCOMPILING)
        message(STATUS "Clang format not found! Downloading...")
        set(CLANG_FORMAT "${PROJECT_BINARY_DIR}/externals/clang-format${CLANG_FORMAT_POSTFIX}.exe")
        file(DOWNLOAD
            https://github.com/yuzu-emu/ext-windows-bin/raw/master/clang-format${CLANG_FORMAT_POSTFIX}.exe
            "${CLANG_FORMAT}" SHOW_PROGRESS
            STATUS DOWNLOAD_SUCCESS)
        if (NOT DOWNLOAD_SUCCESS EQUAL 0)
            message(WARNING "Could not download clang format! Disabling the clang format target")
            file(REMOVE ${CLANG_FORMAT})
            unset(CLANG_FORMAT)
        endif()
    else()
        message(WARNING "Clang format not found! Disabling the clang format target")
    endif()
endif()

if (CLANG_FORMAT)
    set(SRCS ${PROJECT_SOURCE_DIR}/src)
    set(COMPS ${PROJECT_SOURCE_DIR}/components)
    set(APPS ${PROJECT_SOURCE_DIR}/apps)
    set(TESTS ${PROJECT_SOURCE_DIR}/test)
    set(CCOMMENT "Running clang format against all the .h and .cpp files in src/")
    if (WIN32)
        add_custom_target(clang-format
            COMMAND powershell.exe -Command "Get-ChildItem '${SRCS}/*'  -Include *.cpp,*.h -Recurse | Foreach {&'${CLANG_FORMAT}' -i $_.fullname}"
            COMMAND powershell.exe -Command "Get-ChildItem '${COMPS}/*' -Include *.cpp,*.h -Recurse | Foreach {&'${CLANG_FORMAT}' -i $_.fullname}"
            COMMAND powershell.exe -Command "Get-ChildItem '${APPS}/*'  -Include *.cpp,*.h -Recurse | Foreach {&'${CLANG_FORMAT}' -i $_.fullname}"
            COMMAND powershell.exe -Command "Get-ChildItem '${TESTS}/*' -Include *.cpp,*.h -Recurse | Foreach {&'${CLANG_FORMAT}' -i $_.fullname}"
        COMMENT ${CCOMMENT})
    elseif(MINGW)
        add_custom_target(clang-format
            COMMAND find `cygpath -u ${SRCS}`  -iname *.h -o -iname *.cpp | grep -vFf ${CMAKE_SOURCE_DIR}/.clang-format-ignore | xargs `cygpath -u ${CLANG_FORMAT}` -i
            COMMAND find `cygpath -u ${COMPS}` -iname *.h -o -iname *.cpp | grep -vFf ${CMAKE_SOURCE_DIR}/.clang-format-ignore | xargs `cygpath -u ${CLANG_FORMAT}` -i
            COMMAND find `cygpath -u ${APPS}`  -iname *.h -o -iname *.cpp | grep -vFf ${CMAKE_SOURCE_DIR}/.clang-format-ignore | xargs `cygpath -u ${CLANG_FORMAT}` -i
            COMMAND find `cygpath -u ${TESTS}` -iname *.h -o -iname *.cpp | grep -vFf ${CMAKE_SOURCE_DIR}/.clang-format-ignore | xargs `cygpath -u ${CLANG_FORMAT}` -i
        COMMENT ${CCOMMENT})
    else()
        add_custom_target(clang-format
            COMMAND find ${SRCS}  -iname *.h -o -iname *.cpp | grep -vFf ${CMAKE_SOURCE_DIR}/.clang-format-ignore | xargs ${CLANG_FORMAT} -i
            COMMAND find ${COMPS} -iname *.h -o -iname *.cpp | grep -vFf ${CMAKE_SOURCE_DIR}/.clang-format-ignore | xargs ${CLANG_FORMAT} -i
            COMMAND find ${APPS}  -iname *.h -o -iname *.cpp | grep -vFf ${CMAKE_SOURCE_DIR}/.clang-format-ignore | xargs ${CLANG_FORMAT} -i
            COMMAND find ${TESTS} -iname *.h -o -iname *.cpp | grep -vFf ${CMAKE_SOURCE_DIR}/.clang-format-ignore | xargs ${CLANG_FORMAT} -i
            COMMENT ${CCOMMENT})
    endif()
    unset(SRCS)
    unset(CCOMMENT)
endif()
