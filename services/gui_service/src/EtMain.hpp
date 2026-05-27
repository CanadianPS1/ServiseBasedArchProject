#pragma once
#include "EtWindow.hpp"
#include "EtDevice.hpp"
#include "EtRenderer.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>
#include "EtGameObject.hpp"
#include "EtDescriptors.hpp"
#include "WebSockets.hpp"
namespace et{
    struct GlobalUbo{
        glm::mat4 projectionView{1.f};
        alignas(16) glm::vec3 lightDirection = glm::normalize(glm::vec3{1.f,-3.f,-1.f});
        alignas(16) glm::vec3 etLocation{};
    };
    struct UiVertex{
        glm::vec2 pos;
        glm::vec2 uv;
    };
    enum class GameState{
        Login,
        InGame
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
            std::unique_ptr<EtDescriptorPool> globalPool{};
            std::vector<EtGameObject> gameObjects;
    };
}