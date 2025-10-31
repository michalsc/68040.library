# Copyright © 2020 Michal Schulz <michal.schulz@gmx.de>
# https://github.com/michalsc
#
# This Source Code Form is subject to the terms of the
# Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed
# with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

function(bin_to_header target)

    find_program(XXD xxd REQUIRED)

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${target}.h
        COMMAND ${XXD} -i $<TARGET_FILE:${target}> > ${CMAKE_CURRENT_BINARY_DIR}/${target}.h
        DEPENDS ${target}
        COMMENT "Generating ${target}.h from binary"
        VERBATIM
    )

    add_custom_target(
        ${target}_header ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${target}.h
    )

endfunction(bin_to_header)
