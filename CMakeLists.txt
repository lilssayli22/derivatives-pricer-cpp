cmake_minimum_required(VERSION 3.20)
project(pcpd CXX)

# Prefix
if (NOT PREFIX)
    set(PREFIX ${CMAKE_CURRENT_BINARY_DIR})
endif (NOT PREFIX)
set(CMAKE_INSTALL_PREFIX ${PREFIX} CACHE STRING "Installation prefix." FORCE)

if (CXX)
    set(CMAKE_CXX_COMPILER ${CXX})
endif (CXX)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Release or Debug
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
endif ()

# Detect PNL
find_package(Pnl REQUIRED)
set(LIBS ${LIBS} ${PNL_LIBRARIES})
include_directories(${PNL_INCLUDE_DIRS})
if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_definitions(-DPNL_RANGE_CHECK_OFF)
endif()

# Find nlohmann_json to parse json files
# Add nlohmann_json::nlohmann_json to target_link_libraries
find_package(nlohmann_json)
if (NOT nlohmann_json_FOUND)
    if (${CMAKE_VERSION} VERSION_GREATER "3.24.0")
        CMAKE_POLICY(SET CMP0135 NEW)
    endif()
    include(FetchContent)
    FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
    FetchContent_MakeAvailable(json)
endif()

### Ne rien modifier dans la partie au dessus de cette ligne.

add_executable(price0
    src/main.cpp
    src/MonteCarlo.cpp
    src/Option.cpp
    src/BlackScholesModel.cpp
    src/json_helper.cpp
    src/pricing_results.cpp
)
target_link_libraries(price0
    ${PNL_LIBRARIES}
    nlohmann_json::nlohmann_json
)
pnl_add_postbuild(price0)

enable_testing()

add_executable(hedge
    src/hedge.cpp
    src/MonteCarlo.cpp
    src/Option.cpp
    src/BlackScholesModel.cpp
    src/json_helper.cpp
    src/portfolio.cpp
)
target_link_libraries(hedge
    ${PNL_LIBRARIES}
    nlohmann_json::nlohmann_json
)
pnl_add_postbuild(hedge)


add_executable(test_option
    src/test_option.cpp
    src/Option.cpp
)
target_include_directories(test_option PRIVATE src)
target_link_libraries(test_option ${PNL_LIBRARIES})
pnl_add_postbuild(test_option)
add_test(NAME option_payoff COMMAND test_option)

add_executable(test_black_scholes
    src/test_black_scholes.cpp
    src/BlackScholesModel.cpp
)
target_include_directories(test_black_scholes PRIVATE src)
target_link_libraries(test_black_scholes ${PNL_LIBRARIES})
pnl_add_postbuild(test_black_scholes)
add_test(NAME black_scholes_asset COMMAND test_black_scholes)

add_executable(test_monte_carlo
    src/test_monte_carlo.cpp
    src/MonteCarlo.cpp
    src/Option.cpp
    src/BlackScholesModel.cpp
)
target_include_directories(test_monte_carlo PRIVATE src)
target_link_libraries(test_monte_carlo ${PNL_LIBRARIES})
pnl_add_postbuild(test_monte_carlo)
add_test(NAME monte_carlo_price COMMAND test_monte_carlo)

# Print compiler flags
get_directory_property(DirDefs COMPILE_DEFINITIONS)
message(STATUS "COMPILE_DEFINITIONS = ${DirDefs}")
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_LINKER_EXE_FLAGS}" )
else()
    message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_LINKER_EXE_FLAGS}" )
endif()
