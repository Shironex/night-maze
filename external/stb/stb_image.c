/* The one translation unit that holds the implementation of stb_image.

/* stb_image.h is a single header. Included normally it only declares its functions.
   With this macro defined first, the same header also emits their definitions. That must
   happen in exactly one source file of the program: this one. Every other file includes
   the header without the macro. */
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
