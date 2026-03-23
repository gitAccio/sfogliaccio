cmake_minimum_required(VERSION 3.20)
project(SfogliAccio VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "-g -O0 -Wall -Wextra")

# ── Qt6 ──────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Concurrent PrintSupport)

# ── MuPDF ────────────────────────────────────────────────────────────────────
# Se passati da riga di comando (GitHub Actions), usali direttamente
if(MUPDF_LIBRARIES AND MUPDF_INCLUDE_DIRS)
  set(MUPDF_FOUND TRUE)
  message(STATUS "MuPDF fornito esternamente: ${MUPDF_LIBRARIES}")
else()
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(MUPDF QUIET mupdf)
  endif()

  if(NOT MUPDF_FOUND)
    find_library(MUPDF_LIB     NAMES mupdf       PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
    find_library(MUPDF_3RD_LIB NAMES mupdf-third PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
    find_path   (MUPDF_INC     mupdf/fitz.h      PATHS /usr/include /usr/local/include)

    if(MUPDF_LIB AND MUPDF_INC)
      set(MUPDF_FOUND        TRUE)
      set(MUPDF_LIBRARIES    ${MUPDF_LIB})
      set(MUPDF_INCLUDE_DIRS ${MUPDF_INC})
      if(MUPDF_3RD_LIB)
        list(APPEND MUPDF_LIBRARIES ${MUPDF_3RD_LIB})
      endif()
    else()
      message(FATAL_ERROR "MuPDF not found.\n  Arch: sudo pacman -S mupdf-tools")
    endif()
  endif()
endif()

# ── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
  src/main.cpp
  src/MainWindow.cpp
  src/PdfDocument.cpp
  src/PdfView.cpp
  src/SidebarWidget.cpp
  src/SearchBar.cpp
  src/TitleBar.cpp
  src/ZoomController.cpp
  include/MainWindow.h
  include/PdfDocument.h
  include/PdfView.h
  include/SidebarWidget.h
  include/SearchBar.h
  include/TitleBar.h
  include/ZoomController.h
  include/Theme.h
)

qt_add_resources(APP_RESOURCES resources/resources.qrc)

# Icona Windows
if(WIN32)
  set(APP_ICON_RESOURCE resources/app.rc)
endif()

qt_add_executable(sfogliaccio WIN32 ${SOURCES} ${APP_RESOURCES} ${APP_ICON_RESOURCE})

target_include_directories(sfogliaccio PRIVATE include/ ${MUPDF_INCLUDE_DIRS})

target_link_libraries(sfogliaccio PRIVATE
  Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Concurrent Qt6::PrintSupport
  ${MUPDF_LIBRARIES})

# m e pthread solo su Linux/macOS
if(UNIX)
  target_link_libraries(sfogliaccio PRIVATE m pthread)
endif()

# Dipendenze opzionali MuPDF (Linux)
if(UNIX)
  foreach(_lib jpeg jbig2dec openjp2 harfbuzz freetype gumbo)
    find_library(_found_${_lib} ${_lib})
    if(_found_${_lib})
      target_link_libraries(sfogliaccio PRIVATE ${_found_${_lib}})
    endif()
  endforeach()
endif()

install(TARGETS sfogliaccio RUNTIME DESTINATION bin)
install(FILES resources/sfogliaccio.desktop DESTINATION share/applications)
