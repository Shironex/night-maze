# VideoDecoderWindows.cpp is listed on every system and is empty outside of Windows, so
# the checks that run over all .cpp files find it in the list of compiled files everywhere.
target_sources(video PRIVATE
    VideoClock.cpp
    VideoClock.hpp
    VideoDecoder.hpp
    VideoDecoderWindows.cpp
    VideoPlayer.cpp
    VideoPlayer.hpp
)

# The arithmetic of the clock of a video needs neither a decoder nor OpenGL, so the test
# program compiles it directly. Linking the library itself would bring Media Foundation
# into the test program.
target_sources(night_maze_tests PRIVATE
    VideoClock.cpp
    VideoClock.hpp
)

if(APPLE)
    # AVFoundation, the decoder of macOS, is used from Objective-C++ (the language is switched
    # on at the top of the root list, the frameworks are linked there).
    target_sources(video PRIVATE
        VideoDecoderApple.mm
    )
    # Automatic reference counting: the compiler releases the Objective-C objects the file
    # holds, there is no retain and no release in it. TARGET_DIRECTORY: the property is read
    # in the folder that made the target, the root.
    set_source_files_properties(VideoDecoderApple.mm TARGET_DIRECTORY video PROPERTIES
        COMPILE_OPTIONS "-fobjc-arc"
    )
endif()
