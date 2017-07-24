# - Try to find CAEN VME library
# Once done this will define
#  CAENVME_FOUND - System has LibXml2
#  CAENVME_INCLUDE_DIRS - The LibXml2 include directories
#  CAENVME_LIBRARIES - The libraries needed to use LibXml2
#  CAENVME_DEFINITIONS - Compiler switches required for using LibXml2

#find_package(PkgConfig)
#pkg_check_modules(PC_CAENVME QUIET CAENVME)
#set(CAENVME_DEFINITIONS ${PC_CAENVME_CFLAGS_OTHER})

# in case we are shipping a version in the /extern/ subdirectory of our source distribution
file(GLOB_RECURSE extern_file ${PROJECT_SOURCE_DIR}/extern/*CAENVMElib.h)
if (extern_file)
  get_filename_component(extern_lib_path ${extern_file} PATH)
#  MESSAGE(STATUS "Found CAENVME library in 'extern' subfolder: ${extern_lib_path}")
endif(extern_file)


find_path(CAENVME_INCLUDE_DIR CAENVMElib.h
  HINTS
  /usr/local/include
  /usr/include
  /opt/local/include
  $ENV{HOME}/include
  ${extern_lib_path}/include
#  ${PC_CAENVME_INCLUDEDIR} ${PC_CAENVME_INCLUDE_DIRS}
PATH_SUFFIXES CAENVMELib )

# library might be installed in either or both 32/64bit, need to figure out which one to use
if(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
  # using 32 bit compiler
  find_library(CAENVME_LIBRARY NAMES CAENVME
    HINTS
    /usr/local/lib
    /usr/lib
    /opt/local/lib
    $ENV{HOME}/lib
    ${extern_lib_path}/lib/x86
    #  ${PC_CAENVME_LIBDIR} ${PC_CAENVME_LIBRARY_DIRS}
    )
else(NOT "${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
  # using 64 bit compiler
  find_library(CAENVME_LIBRARY NAMES CAENVME
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
    #  ${PC_CAENVME_LIBDIR} ${PC_CAENVME_LIBRARY_DIRS}
    )
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CAENVME_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args("CAENVME library" DEFAULT_MSG
                                  CAENVME_INCLUDE_DIR CAENVME_LIBRARY)

mark_as_advanced( CAENVME_INCLUDE_DIR CAENVME_LIBRARY)

set(CAENVME_LIBRARIES ${CAENVME_LIBRARY} )
set( CAENVME_INCLUDE_DIRS ${CAENVME_INCLUDE_DIR} )
