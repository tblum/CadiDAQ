# - Try to find CAENComm library
# Once done this will define
#  CAENComm_FOUND - System has LibXml2
#  CAENComm_INCLUDE_DIRS - The LibXml2 include directories
#  CAENComm_LIBRARIES - The libraries needed to use LibXml2
#  CAENComm_DEFINITIONS - Compiler switches required for using LibXml2

#find_package(PkgConfig)
#pkg_check_modules(PC_CAENComm QUIET CAENComm)
#set(CAENComm_DEFINITIONS ${PC_CAENComm_CFLAGS_OTHER})

# in case we are shipping a version in the /extern/ subdirectory of our source distribution
file(GLOB_RECURSE extern_file ${PROJECT_SOURCE_DIR}/extern/*CAENCommlib.h)
if (extern_file)
  get_filename_component(extern_lib_path ${extern_file} PATH)
#  MESSAGE(STATUS "Found CAENComm library in 'extern' subfolder: ${extern_lib_path}")
endif(extern_file)


find_path(CAENComm_INCLUDE_DIR CAENComm.h
  HINTS
  /usr/local/include
  /usr/include
  /opt/local/include
  $ENV{HOME}/include
  ${extern_lib_path}/include
#  ${PC_CAENComm_INCLUDEDIR} ${PC_CAENComm_INCLUDE_DIRS}
PATH_SUFFIXES CAENCommLib )

# library might be installed in either or both 32/64bit, need to figure out which one to use
if(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
  # using 32 bit compiler
  find_library(CAENComm_LIBRARY NAMES CAENComm
    HINTS
    /usr/local/lib
    /usr/lib
    /opt/local/lib
    $ENV{HOME}/lib
    ${extern_lib_path}/lib/x86
    #  ${PC_CAENComm_LIBDIR} ${PC_CAENComm_LIBRARY_DIRS}
    )
else(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
  # using 64 bit compiler
  find_library(CAENComm_LIBRARY NAMES CAENComm
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
    #  ${PC_CAENComm_LIBDIR} ${PC_CAENComm_LIBRARY_DIRS}
    )
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CAENComm_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args("CAENComm library" DEFAULT_MSG
                                  CAENComm_INCLUDE_DIR CAENComm_LIBRARY)

mark_as_advanced( CAENComm_INCLUDE_DIR CAENComm_LIBRARY)

set(CAENComm_LIBRARIES ${CAENComm_LIBRARY} )
set( CAENComm_INCLUDE_DIRS ${CAENComm_INCLUDE_DIR} )
