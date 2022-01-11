cmake_minimum_required(VERSION 3.16)

project(nyulan_VM LANGUAGES CXX )
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall -Wextra)

find_package(Boost COMPONENTS program_options REQUIRED)

add_library(core STATIC
    vm.cpp
    )

add_library(obj STATIC
    objectfile.cpp
    )
add_executable(nyulanVM 
    main.cpp
    )
target_link_libraries(nyulanVM PRIVATE
    core
    obj
    ${Boost_LIBRARIES}
    )
target_include_directories(nyulanVM PUBLIC
    ${Boost_INCLUDE_DIR}
    )
