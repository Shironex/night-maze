# GLAD: generated OpenGL 4.1 Core function loader (see README.md for how to regenerate).
add_library(glad STATIC src/gl.c)

# SYSTEM: the generated headers must not produce warnings in our own code.
target_include_directories(glad SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
