include(GNUInstallDirs)

set(CMAKE_VERBOSE_MAKEFILE ON)

if(APPLE)
    set(APPLE_ICON_NAME "qimgv.icns")
    set(APPLE_ICON_PATH "distrib/${APPLE_ICON_NAME}")
endif()

# TRANSLATIONS
find_package(Qt6 COMPONENTS LinguistTools)
# Example of creating a new translation file.
# 1. cd into qimgv directory and run:
#   lupdate -recursive . -ts res/translations/qimgv_en_US.ts    [replace "en_US" with your locale]
# 2. Add your .ts filename into TS_FILES list below
# 3. Translate it with Qt Linguist
# 更新 .ts / 编译 .qm（qt_add_lupdate 生成 qimgv_lupdate target，可手动调用；qt_add_lrelease 把 .qm 加入目标源）
set(TS_FILES
    "zh_CN.ts"
)
list(TRANSFORM TS_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/res/translations/)
set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION ${PROJECT_BINARY_DIR}/qimgv/translations)

# ADD EXECUTABLE
add_executable(qimgv
    appversion.cpp
    core.cpp
    main.cpp
    settings.cpp
    themestore.cpp
    shortcutbuilder.cpp
    proxystyle.cpp
    macosapplication.cpp
    resources.qrc
    qimgv.rc
    ${APPLE_ICON_PATH}
)

qt_add_lupdate(qimgv TS_FILES ${TS_FILES})
qt_add_lrelease(qimgv TS_FILES ${TS_FILES})

# ADD SOURCES
add_subdirectory(components)
add_subdirectory(sourcecontainers)
add_subdirectory(gui)
add_subdirectory(utils)

# C++ STANDARD
target_compile_features(qimgv PRIVATE cxx_std_23)
set_target_properties(qimgv PROPERTIES CXX_EXTENSIONS OFF)

# LINK STUFF
target_link_libraries(qimgv PRIVATE Qt6::Core Qt6::Widgets Qt6::Svg Qt6::PrintSupport Qt6::OpenGLWidgets Qt6::CanvasPainter)

# OPTION DEFINITIONS, LINKING
if(KDE_SUPPORT)
    target_link_libraries(qimgv PRIVATE KF6::WindowSystem)
    target_compile_definitions(qimgv PRIVATE USE_KDE_BLUR)
endif()
if(VIDEO_SUPPORT)
    target_compile_definitions(qimgv PRIVATE USE_MPV)
    if(WIN32)
        ADD_DEFINITIONS(-D_QIMGV_PLUGIN_DIR="plugins")
        ADD_DEFINITIONS(-D_QIMGV_PLAYER_PLUGIN="player_mpv.dll")
    else()
        ADD_DEFINITIONS(-D_QIMGV_PLUGIN_DIR="${QIMGV_PLUGIN_DIR}")
        ADD_DEFINITIONS(-D_QIMGV_PLAYER_PLUGIN="player_mpv.so")
    endif()
endif()

# generate proper GUI program on specified platform
if(WIN32) # Check if we are on Windows
    # Windows 11 DWM 圆角/边框处理（win11window.cpp）
    target_link_libraries(qimgv PRIVATE dwmapi)
    if(MSVC) # Check if we are using the Visual Studio compiler
        set_target_properties(qimgv PROPERTIES WIN32_EXECUTABLE ON)
        target_link_options(qimgv PRIVATE "/ENTRY:mainCRTStartup")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_link_options(qimgv PRIVATE "-mwindows")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(CMAKE_CXX_FLAGS "$ENV{CMAKE_CXX_FLAGS} -Wno-shift-negative-value")
        set(CMAKE_EXE_LINKER_FLAGS "$ENV{CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
        target_link_options(qimgv PRIVATE "-mwindows")
    else()
        message(SEND_ERROR "You are using an unsupported Windows compiler! (Not MSVC or GCC)")
    endif(MSVC)
elseif(APPLE)
    set_target_properties(qimgv PROPERTIES
        MACOSX_BUNDLE YES
        MACOSX_BUNDLE_BUNDLE_NAME "qimgv"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.2"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.2"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/distrib/MyBundleInfo.plist.in
        MACOSX_BUNDLE_ICON_FILE ${APPLE_ICON_NAME}
    )
    set_source_files_properties(${APPLE_ICON_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
elseif(NOT UNIX)
    message(SEND_ERROR "You are on an unsupported platform! (Not Win32, Mac OS X or Unix)")
endif()

## INSTALLATION
install(TARGETS qimgv DESTINATION bin)

# .desktop
install(FILES distrib/qimgv.desktop
    DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/applications)

# icons
foreach(icon IN ITEMS 16x16 22x22 24x24 32x32 36x36 48x48 64x64 128x128 256x256)
    install(FILES distrib/hicolor/${icon}/apps/qimgv.png DESTINATION
        ${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/${icon}/apps)
endforeach()
install(FILES distrib/hicolor/scalable/apps/qimgv.svg
    DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/icons/hicolor/scalable/apps)

# AppData manifest
install(FILES distrib/qimgv.appdata.xml DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/metainfo)

# Install internationalization language files
if(WIN32)
    install(DIRECTORY "${PROJECT_BINARY_DIR}/qimgv/translations" DESTINATION "${CMAKE_INSTALL_BINDIR}")
else()
    ADD_DEFINITIONS(-DTRANSLATIONS_PATH="${CMAKE_INSTALL_FULL_DATAROOTDIR}/qimgv/translations")
    install(DIRECTORY "${PROJECT_BINARY_DIR}/qimgv/translations" DESTINATION "${CMAKE_INSTALL_FULL_DATAROOTDIR}/qimgv")
endif()

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()