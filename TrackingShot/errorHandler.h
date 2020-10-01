#pragma once

// openGL Error handling:
// use with GLCALL(glfunction());
#define ASSERT(func) if (!(func)) __debugbreak();
#define GLCALL(func) glClearErrors(); \
                     func;\
                     ASSERT(glLogCall(#func, __FILE__, __LINE__))

void glClearErrors();

bool glLogCall(const char* func, const char* file, int line);