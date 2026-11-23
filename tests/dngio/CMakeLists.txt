find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(DNGIO_EXIFTOOL NAMES exiftool REQUIRED)

add_test(NAME dngio.cli
  COMMAND ${CMAKE_COMMAND} -E env "DARKTABLE_CLI=$<TARGET_FILE:darktable-cli>"
    ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/test_cli.py" -v)
set_tests_properties(dngio.cli PROPERTIES LABELS "dngio;integration" TIMEOUT 60)

foreach(case fixture_privacy import_fixtures lossless_roundtrip camera_raw_conversion preview_and_threading compression_and_versions crop_and_xmp reject_invalid_exports import_to_catalog cloned_xmp_brightness)
  add_test(NAME dngio.${case}
    COMMAND ${CMAKE_COMMAND} -E env
      "DARKTABLE_CLI=$<TARGET_FILE:darktable-cli>"
      "EXIFTOOL=${DNGIO_EXIFTOOL}"
      ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/test_dng.py" "DngTests.test_${case}" -v)
  set_tests_properties(dngio.${case} PROPERTIES LABELS "dngio;integration" TIMEOUT 1200 PROCESSORS 2)
endforeach()

add_test(NAME dngio.apple_import
  COMMAND ${CMAKE_COMMAND} -E env "DARKTABLE_CLI=$<TARGET_FILE:darktable-cli>"
    ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/test_apple.py" -v)
set_tests_properties(dngio.apple_import PROPERTIES LABELS "dngio;integration" TIMEOUT 360 PROCESSORS 2)
