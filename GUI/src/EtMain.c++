#include "EtMain.hpp"
#include <GLFW/glfw3.h>
namespace et{
    void EtMain::run(){
        while(!etWindow.shouldClose()){
            glfwPollEvents();
        }
    }
}