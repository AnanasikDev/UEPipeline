#include "gl.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "app.h"

int main();

#ifdef WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return main();
}
#endif

int main()
{
    App* app = new App();
    {
        int out = app->Init();
        if (out) return out;
    }

    app->Run();
    app->Exit();
    delete app;
    return 0;
}