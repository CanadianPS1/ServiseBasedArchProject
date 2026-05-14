#pragma once
#include "EtWindow.hpp"
#include "EtDevice.hpp"
#include "EtRenderer.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>
#include "EtGameObject.hpp"
namespace et{
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