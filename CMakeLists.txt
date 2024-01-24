# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# initialize pico-sdk from GIT
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_FETCH_FROM_GIT on)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

# Set name of project
project(pico_hid C CXX ASM)

# Set C standard
set(CMAKE_C_STANDARD 11)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Create a pico-sdk subdirectory in our project for the libraries to be built into
pico_sdk_init()

# Tell CMake where to find the executable source file
add_executable(pico_hid
    main.c
)

target_include_directories(pico_hid PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_sources(pico_hid PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
)

# Link Libraries
target_link_libraries(pico_hid PUBLIC
    pico_stdlib 
    pico_unique_id 
    tinyusb_device 
    tinyusb_board
)

# Create map/bin/hex/uf2 files in addition to ELF.
pico_add_extra_outputs(pico_hid)

# Enable USB stack for TinyUSB
# pico_enable_stdio_usb(pico_hid 1)
# pico_enable_stdio_uart(pico_hid 0)

# Add any other configurations or options as needed
