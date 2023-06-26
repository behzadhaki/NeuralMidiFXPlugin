# libtorch_setup.txt
# Find libtorch
# find_package(Torch REQUIRED)

# Download libtorch if it's not installed
find_package(Torch)
if(NOT Torch_FOUND)


  set(TORCH_VERSION 1.13.1)
  # Check if we're running on Windows
  if(MSVC)
    # https://download.pytorch.org/libtorch/cpu/libtorch-win-shared-with-deps-2.0.1%2Bcpu.zip
    message(STATUS "Downloading libtorch for Windows...")
    set(TORCH_FILE libtorch-win-shared-with-deps-${TORCH_VERSION}%2Bcpu.zip)
    file(DOWNLOAD
      https://download.pytorch.org/libtorch/cpu/${TORCH_FILE}
      ${CMAKE_BINARY_DIR}/${TORCH_FILE}
      SHOW_PROGRESS
    )

    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ${TORCH_FILE}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
  else()

    message(STATUS "Downloading libtorch for MacOS...")
    set(TORCH_FILE libtorch-cxx11-abi-shared-with-deps-${TORCH_VERSION}%2Bcpu.zip)
    file(DOWNLOAD
            https://download.pytorch.org/libtorch/cpu/libtorch-macos-${TORCH_VERSION}.zip
            ${CMAKE_BINARY_DIR}/${TORCH_FILE}
            SHOW_PROGRESS
    )

    execute_process(COMMAND unzip ${TORCH_FILE}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
  endif()

  set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/libtorch)
endif()

