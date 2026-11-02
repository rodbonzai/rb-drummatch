cmake_minimum_required(VERSION 3.22)
project(KitMatch VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# macOS Universal Binary (Apple Silicon arm64 + Intel x86_64)
if (APPLE)
    if (NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
    endif()
    if (NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS deployment target" FORCE)
    endif()
endif()

# JUCE Framework autocontenido en el repositorio
add_subdirectory(JUCE)

juce_add_gui_app(KitMatch
    PRODUCT_NAME "KitMatch"
    VERSION "1.0.0"
    BUNDLE_ID "com.rodbonzai.kitmatch"
)

target_sources(KitMatch PRIVATE
    Source/Main.cpp
    Source/Domain/InstrumentCategory.h
    Source/Domain/Sample.h
    Source/Domain/LibraryFolder.h
    Source/Domain/PadDefinition.h
    Source/Domain/AcousticFeatures.h
    Source/Domain/MatchCandidate.h
    Source/Analysis/FeatureExtractor.h
    Source/Analysis/FeatureExtractor.cpp
    Source/Analysis/CompatibilityEngine.h
    Source/Analysis/CompatibilityEngine.cpp
    Source/Analysis/MatchRanking.h
    Source/Analysis/MatchRanking.cpp
    Source/Persistence/Database.h
    Source/Persistence/Database.cpp
    Source/Library/SampleCatalog.h
    Source/Library/SampleCatalog.cpp
    Source/Library/LibraryScanner.h
    Source/Library/LibraryScanner.cpp
    Source/Audio/AudioEngine.h
    Source/Audio/AudioEngine.cpp
    Source/UI/PadComponent.h
    Source/UI/PadComponent.cpp
    Source/UI/PadGridComponent.h
    Source/UI/PadGridComponent.cpp
    Source/UI/PadDetailComponent.h
    Source/UI/PadDetailComponent.cpp
    Source/UI/IconButton.h
    Source/UI/IconButton.cpp
    Source/UI/WaveformComponent.h
    Source/UI/WaveformComponent.cpp
    Source/UI/SidebarComponent.h
    Source/UI/SidebarComponent.cpp
    Source/UI/SampleListComponent.h
    Source/UI/SampleListComponent.cpp
    Source/UI/TopBarComponent.h
    Source/UI/TopBarComponent.cpp
    Source/UI/MainComponent.h
    Source/UI/MainComponent.cpp
)

target_include_directories(KitMatch PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_definitions(KitMatch PRIVATE
    JUCE_APPLICATION_NAME_STRING="KitMatch"
    JUCE_APPLICATION_VERSION_STRING="1.0.0-beta.1"
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

# Cross-platform SQLite3 support:
# 1. System/vcpkg SQLite3 if available
# 2. Native macOS system library
# 3. Automatic SQLite amalgamation fetch fallback for Windows/other platforms
find_package(SQLite3 QUIET)

if (TARGET SQLite3::SQLite3)
    set(SQLITE_TARGET SQLite3::SQLite3)
elseif (TARGET SQLite::SQLite3)
    set(SQLITE_TARGET SQLite::SQLite3)
elseif (APPLE)
    set(SQLITE_TARGET sqlite3)
else()
    include(FetchContent)
    FetchContent_Declare(
        sqlite3_amalgamation
        URL https://www.sqlite.org/2024/sqlite-amalgamation-3450200.zip
    )
    FetchContent_MakeAvailable(sqlite3_amalgamation)
    add_library(sqlite3_vendored STATIC ${sqlite3_amalgamation_SOURCE_DIR}/sqlite3.c)
    target_include_directories(sqlite3_vendored PUBLIC ${sqlite3_amalgamation_SOURCE_DIR})
    set(SQLITE_TARGET sqlite3_vendored)
endif()

target_link_libraries(KitMatch PRIVATE
    ${SQLITE_TARGET}
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_dsp
)
