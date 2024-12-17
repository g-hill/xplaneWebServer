
cmake_minimum_required(VERSION 3.10)
project(XPlaneWebPanel)

set(CMAKE_CXX_STANDARD 17)

# Set SDK path - adjust this to your SDK location
set(XPLANE_SDK_PATH "C:/SDK")

include_directories(
    "${XPLANE_SDK_PATH}/CHeaders/XPLM"
    "${XPLANE_SDK_PATH}/CHeaders/Widgets"
    "${XPLANE_SDK_PATH}/CHeaders/Wrappers"
)

# Platform-specific settings
if(WIN32)
    add_definitions(-DIBM=1)
    set(PLATFORM "win")
    set(ARCH "x64")
    link_directories("${XPLANE_SDK_PATH}/Libraries/Win")
endif()

# Create shared library
add_library(WebPanel SHARED
    WebPanel.cpp
)

# Link against XPLM
if(WIN32)
    target_link_libraries(WebPanel "XPLM_64")
endif()

# Set output name and location
set_target_properties(WebPanel PROPERTIES
    PREFIX ""
    SUFFIX ".xpl"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/64"
)
