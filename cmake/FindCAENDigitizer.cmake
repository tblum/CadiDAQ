# - Try to find CAENDigitizer library
# Once done this will define
#  CAENDigitizer_FOUND - System has LibXml2
#  CAENDigitizer_INCLUDE_DIRS - The LibXml2 include directories
#  CAENDigitizer_LIBRARIES - The libraries needed to use LibXml2
#  CAENDigitizer_DEFINITIONS - Compiler switches required for using LibXml2

#find_package(PkgConfig)
#pkg_check_modules(PC_CAENDigitizer QUIET CAENDigitizer)
#set(CAENDigitizer_DEFINITIONS ${PC_CAENDigitizer_CFLAGS_OTHER})

# in case we are shipping a version in the /extern/ subdirectory of our source distribution
file(GLOB_RECURSE extern_file ${PROJECT_SOURCE_DIR}/extern/*CAENDigitizerlib.h)
if (extern_file)
  get_filename_component(extern_lib_path ${extern_file} PATH)
#  MESSAGE(STATUS "Found CAENDigitizer library in 'extern' subfolder: ${extern_lib_path}")
endif(extern_file)


find_path(CAENDigitizer_INCLUDE_DIR CAENDigitizer.h
  HINTS
  /usr/local/include
  /usr/include
  /opt/local/include
  $ENV{HOME}/include
  ${extern_lib_path}/include
#  ${PC_CAENDigitizer_INCLUDEDIR} ${PC_CAENDigitizer_INCLUDE_DIRS}
PATH_SUFFIXES CAENDigitizerLib )

# library might be installed in either or both 32/64bit, need to figure out which one to use
if(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
  # using 32 bit compiler
  find_library(CAENDigitizer_LIBRARY NAMES CAENDigitizer
    HINTS
    /usr/local/lib
    /usr/lib
    /opt/local/lib
    $ENV{HOME}/lib
    ${extern_lib_path}/lib/x86
    #  ${PC_CAENDigitizer_LIBDIR} ${PC_CAENDigitizer_LIBRARY_DIRS}
    )
else(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
  # using 64 bit compiler
  find_library(CAENDigitizer_LIBRARY NAMES CAENDigitizer
    HINTS
    # focus on default 64-bit paths used e.g. by RedHat systems
    /usr/local/lib64
    /usr/lib64
    /opt/local/lib64
    $ENV{HOME}/lib64
    ${extern_lib_path}/lib/x64
    # else, try the usual suspects:
    /usr/local/lib
    /usr/lib
    /opt/local/lib
    $ENV{HOME}/lib
    #  ${PC_CAENDigitizer_LIBDIR} ${PC_CAENDigitizer_LIBRARY_DIRS}
    )
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CAENDigitizer_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args("CAENDigitizer library" DEFAULT_MSG
                                  CAENDigitizer_INCLUDE_DIR CAENDigitizer_LIBRARY)

mark_as_advanced( CAENDigitizer_INCLUDE_DIR CAENDigitizer_LIBRARY)

set(CAENDigitizer_LIBRARIES ${CAENDigitizer_LIBRARY} )
set( CAENDigitizer_INCLUDE_DIRS ${CAENDigitizer_INCLUDE_DIR} )
