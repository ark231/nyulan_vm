cmake_minimum_required(VERSION 3.16)

project(nyulan_VM LANGUAGES CXX )
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall -Wextra)

find_package(Boost COMPONENTS program_options log_setup log REQUIRED)

include(cmake/CPM.cmake)
CPMAddPackage(
    NAME magic_enum
    GITHUB_REPOSITORY Neargye/magic_enum
    GIT_TAG v0.7.3
)

add_library(core STATIC
    vm.cpp
    )
target_include_directories(core PRIVATE
    ${magic_enum_SOURCE_DIR}/include
    )
target_compile_options(core PRIVATE
    -pthread
    )
target_link_libraries(core PRIVATE
    pthread
    ${Boost_LIBRARIES}
    )

add_library(obj STATIC
    objectfile.cpp
    )
target_compile_options(obj PRIVATE
    -pthread
    )
target_link_libraries(obj PRIVATE
    utils
    pthread
    ${Boost_LIBRARIES}
    )
target_include_directories(obj PRIVATE
    ${Boost_INCLUDE_DIR}
    ${magic_enum_SOURCE_DIR}/include
    )
add_library(logging STATIC
    logging.cpp
    )
target_link_libraries(logging PUBLIC
    ${Boost_LIBRARIES}
    )
target_include_directories(logging PRIVATE
    ${Boost_INCLUDE_DIR}
    )
add_library(utils STATIC
    utils.cpp
    )
target_link_libraries(utils PRIVATE
    logging
    )
add_executable(nyulanVM 
    main.cpp
    )
target_link_libraries(nyulanVM PRIVATE
    core
    obj
    logging
    utils
    ${Boost_LIBRARIES}
    $<$<CXX_COMPILER_ID:GNU>:dl$<SEMICOLON>backtrace>
    )
target_include_directories(nyulanVM PRIVATE
    ${Boost_INCLUDE_DIR}
    )
target_compile_definitions(nyulanVM PRIVATE
    $<$<CONFIG:Debug>:BOOST_STACKTRACE_USE_ADDR2LINE$<SEMICOLON>BOOST_STACKTRACE_USE_BACKTRACE>
    )

add_executable(readobj
    readobj.cpp
    )
target_link_libraries(readobj PRIVATE
    obj
    logging
    utils
    ${Boost_LIBRARIES}
    )
target_include_directories(readobj PRIVATE
    ${Boost_INCLUDE_DIR}
    )

add_executable(ndb 
    ndb.cpp
    )
target_link_libraries(ndb PRIVATE
    core
    obj
    logging
    utils
    ${Boost_LIBRARIES}
    $<$<CXX_COMPILER_ID:GNU>:dl$<SEMICOLON>backtrace>
    )
target_include_directories(ndb PRIVATE
    ${Boost_INCLUDE_DIR}
    ${magic_enum_SOURCE_DIR}/include
    )
target_compile_definitions(ndb PRIVATE
    $<$<CONFIG:Debug>:BOOST_STACKTRACE_USE_ADDR2LINE$<SEMICOLON>BOOST_STACKTRACE_USE_BACKTRACE>
    )
