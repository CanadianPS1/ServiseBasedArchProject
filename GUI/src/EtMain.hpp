#pragma once
#include "EtWindow.hpp"
#include "Et_pipeline.hpp"
#include "EtDevice.hpp"
namespace et{
    class EtMain{
        public:
        static constexpr int WIDTH = 1400;
        static constexpr int HEIGHT = 800;
        void run();
        private:
        EtWindow etWindow{WIDTH, HEIGHT, "ET: Extra Terestial"};
        EtDevice etDevice{etWindow};
        EtPipeline etPipeline{etDevice, "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv", EtPipeline::defultPipelineConfigInfo(WIDTH,HEIGHT)};
    };
}