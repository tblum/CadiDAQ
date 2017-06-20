cmake_minimum_required(VERSION 2.8)

project(CADiDAQ CXX)

FIND_PACKAGE( Boost
  COMPONENTS program_options
  REQUIRED)

include_directories("${PROJECT_SOURCE_DIR}/include")

ADD_EXECUTABLE( cadidaq src/main.cpp )

# enable c+11 and make it a requirement
set_property(TARGET cadidaq PROPERTY CXX_STANDARD 11)
set_property(TARGET cadidaq PROPERTY CXX_STANDARD_REQUIRED)

TARGET_LINK_LIBRARIES( cadidaq Boost::program_options )
