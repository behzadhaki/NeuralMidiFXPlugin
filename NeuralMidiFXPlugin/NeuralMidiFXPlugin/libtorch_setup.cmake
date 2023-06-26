# libtorch_setup.txt
# Find libtorch
# find_package(Torch REQUIRED)

# Download libtorch if it's not installed
find_package(Torch)
if(NOT Torch_FOUND)


  set(TORCH_VERSION 2.0.0)
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

    file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
    add_custom_command(TARGET example-app
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TORCH_DLLS}
            $<TARGET_FILE_DIR:example-app>)

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

