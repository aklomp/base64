# Written in 2017 by Henrik Steffen Gaßmann henrik@gassmann.onl
#
# To the extent possible under law, the author(s) have dedicated all
# copyright and related and neighboring rights to this software to the
# public domain worldwide. This software is distributed without any warranty.
#
# You should have received a copy of the CC0 Public Domain Dedication
# along with this software. If not, see
#
#     http://creativecommons.org/publicdomain/zero/1.0/
#
########################################################################

set(TARGET_ARCHITECTURE_TEST_FILE "${CMAKE_CURRENT_LIST_DIR}/../test-arch.c")

function(detect_target_architecture OUTPUT_VARIABLE)
    message(STATUS "${CMAKE_CURRENT_LIST_DIR}")
    set(_TARGET_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}")
    if (DEFINED CACHE{CMAKE_SYSTEM_PROCESSOR})
        list(APPEND _TARGET_PROCESSOR "$CACHE{CMAKE_SYSTEM_PROCESSOR}")
    endif()
    string(TOLOWER "${_TARGET_PROCESSOR}" _TARGET_PROCESSOR)

    if (_TARGET_PROCESSOR MATCHES "(^|;)(riscv64|rv64)")
        set(_TARGET_ARCHITECTURE "riscv64")
    elseif (_TARGET_PROCESSOR MATCHES "(^|;)(riscv32|rv32|riscv)")
        set(_TARGET_ARCHITECTURE "riscv")
    else()
        try_compile(_IGNORED "${CMAKE_CURRENT_BINARY_DIR}"
            "${TARGET_ARCHITECTURE_TEST_FILE}"
            OUTPUT_VARIABLE _LOG
        )

        string(REGEX MATCH "##arch=([^#]+)##" _IGNORED "${_LOG}")
        set(_TARGET_ARCHITECTURE "${CMAKE_MATCH_1}")
    endif()

    set(${OUTPUT_VARIABLE} "${_TARGET_ARCHITECTURE}" PARENT_SCOPE)
    set("${OUTPUT_VARIABLE}_${_TARGET_ARCHITECTURE}" 1 PARENT_SCOPE)
    if (_TARGET_ARCHITECTURE STREQUAL "unknown")
        message(WARNING "could not detect the target architecture.")
    endif()
endfunction()
