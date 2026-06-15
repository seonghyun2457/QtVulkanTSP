cmake_minimum_required(VERSION 3.19)
project(QtVulkanPathFinder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add shaders directory
include_directories(shaders)
file(GLOB SHADERS "shaders/shader.*")

# Add header directory
include_directories(header)
file(GLOB HEADERS "header/*.h")

# Add src directory
file(GLOB SOURCES "src/*.cpp")
file(GLOB UI_FILES "src/*.ui")

# Add glm
add_subdirectory(externals/glm)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Gui)

qt_standard_project_setup()

qt_add_executable(${PROJECT_NAME}
    MACOSX_BUNDLE
    ${HEADERS}
    ${SOURCES}
    ${UI_FILES}
    ${SHADERS}
)

# Debug builds use the console subsystem so qDebug() output is visible in the console.
# Other configurations (Release, etc.) build as a console-less GUI app (WIN32).
set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE "$<NOT:$<CONFIG:Debug>>"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt::Core
        Qt6::Widgets
        Qt6::Gui
        glm
)

target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/externals/glm")

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ${PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
