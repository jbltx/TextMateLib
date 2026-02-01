#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "oniguruma::onig" for configuration "Release"
set_property(TARGET oniguruma::onig APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(oniguruma::onig PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libonig.a"
  )

list(APPEND _cmake_import_check_targets oniguruma::onig )
list(APPEND _cmake_import_check_files_for_oniguruma::onig "${_IMPORT_PREFIX}/lib/libonig.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
