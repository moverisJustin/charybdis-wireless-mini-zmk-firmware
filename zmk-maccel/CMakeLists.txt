if(CONFIG_ZMK_INPUT_PROCESSOR_MACCEL)
  zephyr_library()
  zephyr_library_sources(src/input_processor_maccel.c)
  # Need to access ZMK's headers for drivers/input_processor.h
  # CMAKE_SOURCE_DIR is zmk/app when building via west
  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
endif()
