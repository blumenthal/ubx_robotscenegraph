# - Try to find UBX
# Once done this will define
#
#  UBX_FOUND - if UBX was found
#  UBX_INCLUDE_DIR - the UBX include directory (-I)
#  UBX_LINK_DIRECTORIES - the UBX linker directories (-L)
#  UBX_LIBRARIES - UBX libraries
#
# You can set an environment variable "UBX_ROOT" to help CMake to find the UBX library,
# in case it is not installed in one of the standard paths.
#

FIND_PATH(UBX_INCLUDE_DIR NAMES ubx.h
  PATHS
  $ENV{UBX_ROOT}/src
  ENV CPATH
  /usr/include/
  /usr/local/include/
  NO_DEFAULT_PATH
)

FIND_LIBRARY(UBX_LIBRARY NAMES "ubx" 
  PATHS
  $ENV{UBX_ROOT}/src
  $ENV{UBX_ROOT}/lib 
  ENV LD_LIBRARY_PATH
  ENV LIBRARY_PATH
  /usr/lib
  /usr/local/lib
  NO_DEFAULT_PATH
)

IF(UBX_LIBRARY)
  GET_FILENAME_COMPONENT(UBX_LINK_DIRECTORIES ${UBX_LIBRARY} PATH CACHE)
ENDIF(UBX_LIBRARY)


SET(UBX_LIBRARIES_TMP
)


IF(UBX_LIBRARY) 
  SET(UBX_LIBRARIES
    ${UBX_LIBRARY}
    CACHE STRING "Microblx library"
  ) 
ENDIF(UBX_LIBRARY)





include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set UBX_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(UBX  DEFAULT_MSG
                                  UBX_INCLUDE_DIR UBX_LINK_DIRECTORIES UBX_LIBRARIES)

# show the UBX_INCLUDE_DIR and UBX_LIBRARIES variables only in the advanced view
IF(UBX_FOUND)
  MARK_AS_ADVANCED(UBX_INCLUDE_DIR UBX_LINK_DIRECTORIES UBX_LIBRARIES)
ENDIF(UBX_FOUND)
