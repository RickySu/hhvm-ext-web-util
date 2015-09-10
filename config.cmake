set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g")

include(buildhttp_parser)
include(buildr3)

set(CLEAN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/build\;${CLEAN_FILES}")
set(CLEAN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/CMakeCache.txt\;${CLEAN_FILES}")
set(CLEAN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake_install.cmake\;${CLEAN_FILES}")
set(CLEAN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt\;${CLEAN_FILES}")

SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CLEAN_FILES})

CONFIGURE_FILE(${CMAKE_CURRENT_BINARY_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories(${R3_INCLUDE_DIR} ${HTTP_PARSER_INCLUDE_DIR})
set(EXT_LIBRARIES ${R3_LIB}/libr3.a ${HTTP_PARSER_LIB}/libhttp_parser.o)

HHVM_EXTENSION(web_util
    src/ext.cpp
    src/web_util_r3.cpp
    src/web_util_http_parser.cpp
)
HHVM_SYSTEMLIB(web_util ext_web_util.php)

target_link_libraries(web_util ${EXT_LIBRARIES})
