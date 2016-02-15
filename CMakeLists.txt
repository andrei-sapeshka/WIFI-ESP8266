cmake_minimum_required(VERSION 3.3)
project(WIFI_ESP8266)

set(SDK_BASE /opt/Espressif/ESP8266_SDK/)

include_directories(include
                    driver
                    ${SDK_BASE}/include)

set(SOURCE_FILES
    driver/uart.c
    include/driver/uart.h
    include/driver/uart_register.h
    include/at.h
    user/at.c
    user/user_config.h
    user/user_main.c)

add_executable(WIFI_ESP8266 ${SOURCE_FILES})

#cmake_minimum_required(VERSION 3.2)
#project(WIFI_ESP8266)
#
##enable_language(CC)
#
## base directory for the compiler
#set(XTENSA_TOOLS_ROOT /opt/Espressif/crosstool-NG/builds/xtensa-lx106-elf/bin)
#
## select which tools to use as compiler, librarian and linker
#
##set(CMAKE_C_COMPILER ${XTENSA_TOOLS_ROOT}/xtensa-lx106-elf-gcc)
##set(CMAKE_LINKER ${XTENSA_TOOLS_ROOT}/xtensa-lx106-elf-gcc)
##set(CMAKE_AR ${XTENSA_TOOLS_ROOT}/xtensa-lx106-elf-ar)
#
#set(SDK_BASE /opt/Espressif/ESP8266_SDK/)
#
## Output directors to store intermediate compiled files
## relative to the project directory
##BUILD_BASE	= build
##FW_BASE		= firmware
#set(BUILD_BASE build)
#set(FW_BASE firmware)
#
#include_directories(include
#        driver
#        ${SDK_BASE}/include)
#
#
#file( GLOB APP_SOURCES user/*.c )
#foreach( testsourcefile ${APP_SOURCES} )
#    # I used a simple string replace, to cut off .cpp.
#    string( REPLACE ".cpp" "" testname ${testsourcefile} )
#    add_executable( ${testname} ${testsourcefile} )
#    # Make sure YourLib is linked to each app
#    target_link_libraries( ${testname} YourLib )
#endforeach( testsourcefile ${APP_SOURCES} )