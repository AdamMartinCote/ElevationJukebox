###############################################################################
#                    CMAKE SCRIPT SPECIFIC TO THE SSL DAEMON                  #
###############################################################################

set(DEPS_DIR "${CMAKE_BINARY_DIR}/dependencies")

include(FilesToCompile.cmake)  # Call CMake script "FilesToCompile.cmake"
include(FilesToGenerate.cmake) # Call CMake script "FilesToGenerate.cmake"

# Configure compiler
set(CMAKE_CXX_STANDARD 14)                 # Equivalent to the --std=c++<version> flag in g++
set(CMAKE_CXX_STANDARD_REQUIRED ON)        # C++ version 14 is required, not optional
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall")
include_directories(                       # Equivalent to the -I flag in g++
    "${DEPS_DIR}/include"
    "${COMMON_INCLUDE_DIR}"
    "${SSL_DAEMON_GENERATED_INCLUDE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
link_directories("${DEPS_DIR}/lib")        # Equivalent to the -L flag in g++
link_libraries(ssl crypto atomic common)   # Equivalent to the -l flag in g++

# Create targets
add_executable(ssl-daemon     main.cpp "${SSL_DAEMON_HEADER_SOURCES}")
add_library   (ssl-daemon-lib          "${SSL_DAEMON_CPP_SOURCES}" "${SSL_DAEMON_HEADER_SOURCES}") # Static library used for unit tests
set_target_properties(ssl-daemon-lib PROPERTIES OUTPUT_NAME ssl-daemon)
target_link_libraries(ssl-daemon ssl-daemon-lib)

# Testing
add_subdirectory(test)
