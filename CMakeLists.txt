cmake_minimum_required(VERSION 3.16)

project(FPlayer VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

link_directories(${PROJECT_SOURCE_DIR}/lib)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Multimedia Network Concurrent)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appFPlayer
    main.cpp
    LoginManager.cpp
    SDKManager.cpp
    CameraTreeModel.cpp
    CameraTreeService.cpp
    PlaybackManager.cpp
    PsParser.cpp
    RTSPClientWrapper.cpp
    RTSPDecoder.cpp
    SVACDecoder.cpp
    SvacImageProvider.cpp
    LoginManager.h
    SDKManager.h
    CameraTreeModel.h
    CameraTreeService.h
    PlaybackManager.h
    PsParser.h
    RTSPClientWrapper.h
    RTSPDecoder.h
    StreamDecoder.h
    SVACDecoder.h
    SvacImageProvider.h
    ZxSvacDecLib.h
    tri_core_api.h
    mpeg-proto.h
    base_defs.h
    vitech_invs.h
    logger.h
)

qt_add_qml_module(appFPlayer
    URI FPlayer
    VERSION 1.0
    QML_FILES
        Login.qml
        HomePage.qml
        PlayerPage.qml
    RESOURCES
        images/logo.png
        SOURCES SvacImageProvider.cpp

)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appFPlayer PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appFPlayer
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appFPlayer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/live555/liveMedia/include
    ${CMAKE_CURRENT_SOURCE_DIR}/live555/groupsock/include
    ${CMAKE_CURRENT_SOURCE_DIR}/live555/UsageEnvironment/include
    ${CMAKE_CURRENT_SOURCE_DIR}/live555/BasicUsageEnvironment/include
)

target_link_libraries(appFPlayer
    PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia Qt6::Network Qt6::Concurrent
)

target_link_libraries(appFPlayer PRIVATE
    ZxSvacDecLib
    p35114_sdk
    liveMedia
    groupsock
    UsageEnvironment
    BasicUsageEnvironment
    ws2_32
)


if (WIN32)
    target_compile_definitions(appFPlayer PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

include(GNUInstallDirs)
install(TARGETS appFPlayer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
