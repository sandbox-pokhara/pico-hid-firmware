# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project
project(project1 C CXX ASM)

# Set C standard
set(CMAKE_C_STANDARD 11)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Create a pico-sdk subdirectory in our project for the libraries to be built into
pico_sdk_init()

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME}
    main.c
    # test.c
    )

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_sources(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/main.c
    ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
    # ${CMAKE_CURRENT_LIST_DIR}/test.c
    )

# Link Libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
    pico_stdlib 
    pico_unique_id 
    tinyusb_device 
    tinyusb_board
)

# Create map/bin/hex/uf2 files in addition to ELF.
pico_add_extra_outputs(${PROJECT_NAME})

# Enable USB stack for TinyUSB
# pico_enable_stdio_usb(${PROJECT_NAME} 1)
# pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Add any other configurations or options as needed
