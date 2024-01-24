## Build

1. Make sure that build-essential is installed.

   ```
   sudo apt-get install build-essential
   ```

1. Install CMake (at least version 3.13), and GCC cross compiler

   ```
   sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
   ```

1. Setup a CMake build directory.

   ```
   mkdir build
   cd build
   cmake ..
   ```

1. Make your target from the build directory you created.

   ```
   make pico_hid
   ```
