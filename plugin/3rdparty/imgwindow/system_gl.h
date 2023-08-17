#if _MSC_VER                    // compiling via MS Visual Studio
#include <windows.h>            // need to make sure this is read first
#endif

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif
