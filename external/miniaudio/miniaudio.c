/* The one translation unit that holds the implementation of miniaudio. */

/* miniaudio.h is a single header. Included normally it only declares its functions.
   With this macro defined first, the same header also emits their definitions. That must
   happen in exactly one source file of the program: this one. The audio library includes
   the header without the macro (src/audio/AudioEngine.cpp).

   The macros that leave parts of miniaudio out (MA_NO_ENCODING and the others) are not
   written here. They come from CMake (cmake/Dependencies.cmake), so that this file and
   the file that includes the header always see the same set. */
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
