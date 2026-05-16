#pragma once
#include "EtWindow.hpp"
#include "EtDevice.hpp"
#include "EtRenderer.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>
#include "EtGameObject.hpp"
namespace et{
    struct GlobalUbo{
        glm::mat4 projectionView{1.f};
        glm::vec3 lightDirection = glm::normalize(glm::vec3{1.f,-3.f,-1.f});
    };
    class EtMain{
        public:
            static constexpr int WIDTH = 1400;
            static constexpr int HEIGHT = 788;
            EtMain();
            ~EtMain();
            EtMain(const EtMain &) = delete;
            EtMain &operator=(const EtMain &) = delete;
            void run();
        private:
            void loadGameObjects();
            EtWindow etWindow{WIDTH, HEIGHT, "ET: Extra Terestial"};
            EtDevice etDevice{etWindow};
            EtRenderer etRenderer{etWindow, etDevice};
            std::vector<EtGameObject> gameObjects;
    };
}