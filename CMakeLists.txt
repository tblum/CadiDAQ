cmake_minimum_required(VERSION 3.3)

# set project name
project(CADiDAQ CXX)

# make CMake aware of any cmake modules we ship with the project
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# find boost library
FIND_PACKAGE( Boost
  COMPONENTS program_options log
  REQUIRED)

include_directories("${PROJECT_SOURCE_DIR}/include")

# Generate enum->string code for the CAEN library
cmake_policy(SET CMP0057 NEW) # introduced in CMake 3.3
include(enum2string)
include_directories("${PROJECT_SOURCE_DIR}/caen")
enum2str_generate(
   PATH       "${CMAKE_CURRENT_BINARY_DIR}"
   CLASS_NAME "CaenEnum2str"
   NAMESPACE  "cadidaq"
   FUNC_NAME  "toStr"
   INCLUDES   "CAENDigitizerType.h" # WITHOUT directory
   ENUMS      "CAEN_DGTZ_ConnectionType" "CAEN_DGTZ_BoardModel_t"
   BLACKLIST  ""  # any enums that cause trouble
#   USE_CONSTEXPR
#   USE_C_STRINGS
   )

# make the files generated above accessible
include_directories("${PROJECT_BINARY_DIR}")

# main executable
ADD_EXECUTABLE( cadidaq src/main.cpp ${PROJECT_BINARY_DIR}/CaenEnum2str.cpp)

# enable c+11 and make it a requirement
set_property(TARGET cadidaq PROPERTY CXX_STANDARD 11)
set_property(TARGET cadidaq PROPERTY CXX_STANDARD_REQUIRED)
# set dynamic linking for Boost::log (would otherwise result in linking errors e.g. on OSX, AppleClang 7.0.2.7000181, Boost 1.63)
set_target_properties(cadidaq PROPERTIES COMPILE_DEFINITIONS "BOOST_LOG_DYN_LINK")

TARGET_LINK_LIBRARIES( cadidaq Boost::program_options Boost::log)
