#include "EtWindow.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
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
        std::cout<<"created the window"<<std::endl;
    }
}