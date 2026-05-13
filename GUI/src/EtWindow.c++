#include "EtWindow.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
namespace et{
    EtWindow::EtWindow(int w, int h, std::string name) : width{w}, height{h}, windowName{name}{
        initWindow();
    }
    EtWindow::~EtWindow(){
        glfwDestroyWindow(window);
        glfwTerminate();
    }
    void EtWindow::initWindow(){
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
        glfwShowWindow(window);
        glfwFocusWindow(window);
    }
    void EtWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface){
        if(glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) throw std::runtime_error("failed to create window surface");
    }
}