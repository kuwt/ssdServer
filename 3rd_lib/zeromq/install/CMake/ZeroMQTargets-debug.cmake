#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libzmq" for configuration "Debug"
set_property(TARGET libzmq APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libzmq PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/libzmq-v140-mt-gd-4_3_1.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/libzmq-v140-mt-gd-4_3_1.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS libzmq )
list(APPEND _IMPORT_CHECK_FILES_FOR_libzmq "${_IMPORT_PREFIX}/lib/libzmq-v140-mt-gd-4_3_1.lib" "${_IMPORT_PREFIX}/bin/libzmq-v140-mt-gd-4_3_1.dll" )

# Import target "libzmq-static" for configuration "Debug"
set_property(TARGET libzmq-static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(libzmq-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX;RC"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libzmq-v140-mt-sgd-4_3_1.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS libzmq-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_libzmq-static "${_IMPORT_PREFIX}/lib/libzmq-v140-mt-sgd-4_3_1.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
