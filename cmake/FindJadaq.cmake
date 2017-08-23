# - Try to find Jadaq
# Once done this will define
#  JADAQ_FOUND - System has LibXml2
#  JADAQ_INCLUDE_DIRS - The LibXml2 include directories
#  JADAQ_LIBRARIES - The libraries needed to use LibXml2
#  JADAQ_DEFINITIONS - Compiler switches required for using LibXml2

#find_package(PkgConfig)
#pkg_check_modules(PC_JADQ QUIET caen)
#set(JADAQ_DEFINITIONS ${PC_JADAQ_CFLAGS_OTHER})

find_path(JADAQ_INCLUDE_DIR caen.hpp
  HINTS
  /usr/local/include
  /usr/include
  /opt/local/include
  $ENV{HOME}/include
  $ENV{HOME}/src
#  ${PC_JADAQ_INCLUDEDIR} ${PC_JADAQ_INCLUDE_DIRS}
PATH_SUFFIXES jadaq )

find_library(JADAQ_LIBRARY NAMES caen
  HINTS
  /usr/local/lib
  /usr/lib
  /opt/local/lib
  $ENV{HOME}/lib  
#  ${PC_JADAQ_LIBDIR} ${PC_JADAQ_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set JADAQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Jadaq  DEFAULT_MSG
                                  JADAQ_INCLUDE_DIR JADAQ_LIBRARY )

mark_as_advanced( JADAQ_INCLUDE_DIR JADAQ_LIBRARY )

set(JADAQ_LIBRARIES ${JADAQ_LIBRARY} )
set( JADAQ_INCLUDE_DIRS ${JADAQ_INCLUDE_DIR} )
