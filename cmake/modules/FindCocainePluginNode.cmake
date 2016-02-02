# - Try to find Cocaine Plugin Node library and development includes paths.
# Once done this will define:
#  LIBCOCAINE_PLUGIN_NODE_FOUND         - system has Cocaine Node Service
#  LIBCOCAINE_PLUGIN_NODE_INCLUDE_DIRS  - the Cocaine Node Service include directories
#  LIBCOCAINE_PLUGIN_NODE_LIBRARIES     - the libraries needed to use Cocaine Node Service
#  LIBCOCAINE_PLUGIN_NODE_DEFINITIONS   - compiler switches required for using Cocaine Node Service

find_package(PkgConfig)
pkg_check_modules(PC_LIBCOCAINE_PLUGIN_NODE QUIET libcocaine-plugin-node3)
set(LIBCOCAINE_PLUGIN_NODE_DEFINITIONS ${PC_LIBCOCAINE_PLUGIN_NODE_CFLAGS_OTHER})

find_path(
    LIBCOCAINE_PLUGIN_NODE_INCLUDE_DIR cocaine/service/node.hpp
    HINTS ${PC_LIBCOCAINE_PLUGIN_NODE_INCLUDEDIR} ${PC_LIBCOCAINE_PLUGIN_NODE_INCLUDE_DIRS}
    PATH_SUFFIXES cocaine)

find_library(
    LIBCOCAINE_PLUGIN_NODE_LIBRARY NAMES node.cocaine-plugin.dylib node.cocaine-plugin.so
    HINTS "${CMAKE_PREFIX_PATH}/cocaine"
    PATHS "/usr/lib/cocaine" "/usr/local/lib/cocaine")

set(LIBCOCAINE_PLUGIN_NODE_LIBRARIES ${LIBCOCAINE_PLUGIN_NODE_LIBRARY})
set(LIBCOCAINE_PLUGIN_NODE_INCLUDE_DIRS ${LIBCOCAINE_PLUGIN_NODE_INCLUDE_DIR})

# Handle the QUIETLY and REQUIRED arguments and set LIBCOCAINE_PLUGIN_NODE_FOUND to TRUE if all listed variables
# are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    CocainePluginNode DEFAULT_MSG
    LIBCOCAINE_PLUGIN_NODE_LIBRARY LIBCOCAINE_PLUGIN_NODE_INCLUDE_DIR)

mark_as_advanced(LIBCOCAINE_PLUGIN_NODE_INCLUDE_DIR LIBCOCAINE_PLUGIN_NODE_LIBRARY)
