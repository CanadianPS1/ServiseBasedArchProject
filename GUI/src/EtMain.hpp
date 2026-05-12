#pragma once
#include "EtWindow.hpp"
namespace et{
    class EtMain{
        public:
        static constexpr int WIDTH = 800;
        static constexpr int HEIGHT = 600;
        void run();
        private:
        EtWindow etWindow{WIDTH, HEIGHT, "Hello Vulkan"};
    };
}