#include <iostream>

#include "errorHandler.h"
#include "GLFW/glfw3.h"

void glClearErrors()
{
    while (glGetError() != GL_NO_ERROR);
}

bool glLogCall(const char* func, const char* file, int line)
{
    while (GLenum error = glGetError())
    {
        std::cout << "OpenGL Error (" << error << "): " << func << " " << file << ": " << line << std::endl;
        return false;
    }
    return true;
}