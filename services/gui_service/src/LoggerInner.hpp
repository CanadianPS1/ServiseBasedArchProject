#pragma once
#include "EtGameObject.hpp"
#include <vector>
#define AUTH_HOST "localhost"
#define AUTH_PORT 8000
#define CONNECTION_TIMEOUT_SEC 5
#define READ_TIMEOUT_SEC 5
namespace et{
    class LoggerInner{
        public:
            void Typer(GLFWwindow* window);
            std::vector<std::string> name;
            bool loggedIn = false;
    };
}