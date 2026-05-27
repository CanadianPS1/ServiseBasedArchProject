#pragma once
#include "LoggerInner.hpp"
#include "WebSockets.hpp"
#include "../include/httplib/httplib.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include "../include/nlohmann/json.hpp"
namespace et{
    void LoggerInner::Typer(GLFWwindow* window){
        if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) name.push_back("q");
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) name.push_back("w");
        if(glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) name.push_back("e");
        if(glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) name.push_back("r");
        if(glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) name.push_back("t");
        if(glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) name.push_back("y");
        if(glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) name.push_back("u");
        if(glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) name.push_back("i");
        if(glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) name.push_back("o");
        if(glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) name.push_back("p");
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) name.push_back("a");
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) name.push_back("s");
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) name.push_back("d");
        if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) name.push_back("f");
        if(glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) name.push_back("g");
        if(glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) name.push_back("h");
        if(glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) name.push_back("j");
        if(glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) name.push_back("k");
        if(glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) name.push_back("l");
        if(glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) name.push_back("z");
        if(glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) name.push_back("x");
        if(glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) name.push_back("c");
        if(glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) name.push_back("v");
        if(glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) name.push_back("b");
        if(glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) name.push_back("n");
        if(glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) name.push_back("m");
        if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS){
            std::string namestd = "";
            for(int i = 0; i < name.size(); i++) namestd += name[i];
            std::cout<<namestd<<std::endl;
            httplib::Client client(AUTH_HOST, AUTH_PORT);
            client.set_connection_timeout(CONNECTION_TIMEOUT_SEC);
            client.set_read_timeout(READ_TIMEOUT_SEC);
            auto res = client.Get(("/get_game_state/" + namestd).c_str());
            if(res->status == 404) WebSockets::Connect("localhost", "8001", "/game?"+ namestd + "&mode=new");
            else WebSockets::Connect("localhost", "8001", "/game?"+ namestd + "&mode=continue");
            try{auto json = nlohmann::json::parse(res->body);}
            catch(const nlohmann::json::parse_error& e){std::cerr<<("Failed to parse save for '{}': '{}'!", namestd, e.what());}
            loggedIn = true;
        }
    }
}