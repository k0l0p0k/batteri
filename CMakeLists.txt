cmake_minimum_required(VERSION 2.8)
project(batteri C)
set(PROJECT_NAME batteri)
SET(VERSION "0.0.5")
INCLUDE(AddFileDependencies)
INCLUDE(CheckIncludeFiles)
INCLUDE(FindPkgConfig)

# these are all the libraries I need
FIND_PACKAGE(GTK2 REQUIRED gtk)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(GOBJECT REQUIRED gobject-2.0)
pkg_check_modules(UPOWER REQUIRED upower-glib)

set(CMAKE_BUILD_TYPE "RELEASE")
set(CMAKE_CONFIGURATION_TYPE "RELEASE")

set(CMAKE_INSTALL_PREFIX "/usr")
FILE(RELATIVE_PATH LIB_DIR_RELATIVE "/${BIN_DIR}" "/${LIB_DIR}")

INCLUDE_DIRECTORIES(${GTK2_INCLUDE_DIRS})

#compiler and linker  make it SMALL and fast please
set(CMAKE_C_FLAGS "-Os -finline-small-functions -ffunction-sections -fdata-sections -fmerge-all-constants -fomit-frame-pointer -mno-accumulate-outgoing-args -fno-unwind-tables -fno-asynchronous-unwind-tables")
install (FILES ${PROJECT_NAME}.desktop DESTINATION share/applications)
# link everything
set(LINK_FLAGS "${UPOWER_LDFLAGS} ${GLIB2_LDFLAGS} ${GOBJECT_LDFLAGS} -Wl --gc-sections --sort-common --as-needed -s")
# add our executable
add_executable(${PROJECT_NAME} src/${PROJECT_NAME}.c)
# link it
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${GTK2_LIBRARIES} ${UPOWER_LIBRARIES})
# install it to bin
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
