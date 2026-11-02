cmake_minimum_required(VERSION 3.22)
project(KitMatch VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# JUCE Framework autocontenido en el repositorio
add_subdirectory(JUCE)

juce_add_gui_app(DrumMatch
    PRODUCT_NAME "KitMatch"
    VERSION "1.0.0"
    BUNDLE_ID "com.rodbonzai.kitmatch"
)

target_sources(DrumMatch PRIVATE
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

target_include_directories(DrumMatch PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_definitions(DrumMatch PRIVATE
    JUCE_APPLICATION_NAME_STRING="DrumMatch"
    JUCE_APPLICATION_VERSION_STRING="1.0.0"
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

target_link_libraries(DrumMatch PRIVATE
    sqlite3
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
