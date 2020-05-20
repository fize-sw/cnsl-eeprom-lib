cmake_minimum_required(VERSION 3.1)
project (cnsl-eeprom)
set(CMAKE_BUILD_TYPE Release)

file(GLOB libglv-eeprom-SRC "src/eeplib.c" "include/eeplib.h")
file(GLOB MAIN-SRC "src/main_test.c")

#set(CMAKE_CXX_STANDARD 14)
#set(SOURCES src/eeplib.c src/eeprom_test.c include/eeplib.h)

include_directories(include)

add_library(${PROJECT_NAME} STATIC ${libglv-eeprom-SRC})

add_executable(main_test ${MAIN-SRC})

target_link_libraries(main_test libglv-eeprom)

