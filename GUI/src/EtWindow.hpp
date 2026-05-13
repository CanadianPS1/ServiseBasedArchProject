#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>
namespace et{
    class EtWindow{
        public:
        EtWindow(int w, int h, std::string name);
        ~EtWindow();
        bool shouldClose(){return glfwWindowShouldClose(window);}
        void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);
        private:
        void initWindow();
        const int width;
        const int height;
        std::string windowName;
        GLFWwindow *window;
    };
}