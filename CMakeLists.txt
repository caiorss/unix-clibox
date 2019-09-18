cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(clibox)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

#============= Conan ===========================#

# Download automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.13/conan.cmake"
                 "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)
set(CONAN_PROFILE default)

conan_cmake_run( REQUIRES
                 CLI11/1.8.0@cliutils/stable
                 BASIC_SETUP
                 BUILD missing )

#============= Functions =======================#

# Copy target file to current directory whenerver it is rebuilt
function(copy_after_build TARGET_NAME )
    # Note: CMAKE_CURRENT_LIST_DIR is the directory where is this
    # CMakeLists.txt file.
    set(DESTDIR ${CMAKE_CURRENT_LIST_DIR}/bin/)
    file(MAKE_DIRECTORY ${DESTDIR})

    # Copy binary file to <CMakeLists.txt didctory>./bin
    # after target is compiled.
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
                               $<TARGET_FILE:${TARGET_NAME}> ${DESTDIR}
                               )
endfunction()


macro(Download_Single_Headerlib FILE URL)
    file(DOWNLOAD ${URL} ${CMAKE_BINARY_DIR}/include/${FILE})
    IF(NOT Download_Single_Headerlib_flag)
       include_directories(${CMAKE_BINARY_DIR}/include)
       set(Download_Single_Headerlib_flag TRUE)
    ENDIF()
endmacro()

#============= Target Settings =================#

find_package(CLI11 REQUIRED)

add_executable(cb.ls listdir.cpp)
target_link_libraries(cb.ls pthread stdc++fs)
copy_after_build(cb.ls)

# Command line tool for searching text in many files and directories
add_executable(cb.text-search text-search.cpp)
target_link_libraries(cb.text-search pthread stdc++fs)
copy_after_build(cb.text-search)

# Command line tool for bulk rename of files.
add_executable(cb.rename rename.cpp)
target_link_libraries(cb.rename stdc++fs)
copy_after_build(cb.rename)

# Command line tool for launching applications and daemons on Linux
add_executable(cb.launch launch.cpp)
target_link_libraries(cb.launch pthread stdc++fs)
copy_after_build(cb.launch)

# Command line tool for analysis of binary files and forensic
add_executable(cb.hextool hextool.cpp)
target_link_libraries(cb.hextool)
copy_after_build(cb.hextool)


