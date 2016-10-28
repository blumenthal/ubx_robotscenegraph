# - Try to find Jansson
# Once done this will define
#  JANSSON_FOUND - System has Jansson
#  JANSSON_INCLUDE_DIRS - The Jansson include directories
#  JANSSON_LIBRARIES - The libraries needed to use Jansson

find_path(JANSSON_INCLUDE_DIR jansson.h
          /usr/include
          /usr/local/include )

find_library(JANSSON_LIBRARY NAMES jansson
             PATHS /usr/lib /usr/local/lib )

set(JANSSON_LIBRARIES ${JANSSON_LIBRARY} )
set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set JANSSON_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Jansson  DEFAULT_MSG
                                  JANSSON_LIBRARY JANSSON_INCLUDE_DIR)

mark_as_advanced(JANSSON_INCLUDE_DIR JANSSON_LIBRARY )