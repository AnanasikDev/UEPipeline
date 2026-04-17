#pragma once

class GLFWwindow;

class App
{
public:
    int Init();
    void Exit();
    void Run();

private:
    void Tick();
    void SetupImGui();
    void PreRender();
    void Render();
    void PostRender();

    GLFWwindow* window;
};