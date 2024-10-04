cmake_minimum_required(VERSION 3.0)
project(LunaSpy VERSION 1.0 LANGUAGES CXX)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#set(CMAKE_INSTALL_DIR ~/.config/LunaDAQ)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOUIC_SEARCH_PATHS ${UI_DIR})

find_package(ROOT REQUIRED COMPONENTS RIO Net)
find_package(Boost COMPONENTS thread system serialization REQUIRED)

include_directories(
  ${HEADER_DIR}
  ${ROOT_INCLUDE_DIRS}
)

add_executable(LunaSpy
  ${SRC_DIR}/main.cpp
  ${SRC_DIR}/SpyServerRU.cc
  ${SRC_DIR}/XDAQSpy.cc
  ${SRC_DIR}/DataFrame.cc
  ${SRC_DIR}/DataFrameBuilderPHA.cc
  ${SRC_DIR}/DataFrameBuilderPSD.cc
  ${SRC_DIR}/Utils.cc
)

target_compile_features(LunaSpy PUBLIC cxx_std_14)

target_link_libraries(LunaSpy PRIVATE ${Boost_LIBRARIES} ${ROOT_LIBRARIES} )