# Calls `message(VERBOSE msg)` if and only if VERBOSE is available (since CMake 3.15).
# Call CMake with --loglevel=VERBOSE to view those messages.
function(message_verbose msg)
  if(NOT ${CMAKE_VERSION} VERSION_LESS "3.15")
    message(VERBOSE ${msg})
  endif()
endfunction()

function(try_linker linker)
  string(TOUPPER ${linker} upper_linker)
  find_program(HAS_LD_${upper_linker} "ld.${linker}")
  if(HAS_LD_${upper_linker})
    set(CMAKE_REQUIRED_LIBRARIES "-fuse-ld=${linker}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/main.c" "int main() { return 0; }")
    try_compile(
      HAVE_LD_${upper_linker}
      ${CMAKE_CURRENT_BINARY_DIR}
      "${CMAKE_CURRENT_BINARY_DIR}/main.c"
      LINK_LIBRARIES "-fuse-ld=${linker}"
    )
  endif()
endfunction()

function(use_fastest_linker)
  if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(WARNING "use_fastest_linker() disabled, as it is not called at the project top level")
    return()
  endif()

  set(use_default_linker 1)
  try_linker(lld)
  if (HAVE_LD_LLD)
    link_libraries("-fuse-ld=lld")
    set(use_default_linker 0)
    message_verbose("Using lld linker for faster linking")
  else()
    try_linker(gold)
    if(HAVE_LD_GOLD)
      link_libraries("-fuse-ld=gold")
      set(use_default_linker 0)
      message_verbose("Using gold linker for faster linking")
    endif()
  endif()
  if(use_default_linker)
    message_verbose("Using default linker")
  endif()
endfunction()

option(USE_FASTER_LINKER "Use the lld or gold linker instead of the default for faster linking" TRUE)
if(USE_FASTER_LINKER)
  use_fastest_linker()
endif()
