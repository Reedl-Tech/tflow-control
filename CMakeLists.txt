# CMakeList.txt : CMake project for tflow-process, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.0)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()


project ("tflow-control")

find_package(PkgConfig REQUIRED)

# Add source to this project's executable.
add_executable (${PROJECT_NAME}
    "main.cpp"
    "tflow-common.hpp"
    "tflow-control.cpp"
    "tflow-control.hpp"
    "tflow-ctrl-cli.hpp"
    "tflow-ctrl-cli.cpp"
    "tflow-mg.cpp"
    "tflow-mg.hpp"
    "mongoose.c"
    "mongoose.h"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

pkg_check_modules(JSON11 REQUIRED IMPORTED_TARGET
    json11
)

pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET 
    glib-2.0
    gmodule-2.0
    gio-2.0
    gio-unix-2.0
)

pkg_check_modules(GLIBMM REQUIRED IMPORTED_TARGET 
    glibmm-2.4
    giomm-2.4
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0") # Add debug information

target_include_directories(${PROJECT_NAME} PRIVATE ${JSON11_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${GLIB_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${GLIBMM_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} PkgConfig::JSON11)
target_link_libraries(${PROJECT_NAME} PkgConfig::GLIB)
target_link_libraries(${PROJECT_NAME} PkgConfig::GLIBMM)

target_link_libraries(${PROJECT_NAME} -lrt)

install(TARGETS tflow-control DESTINATION bin)