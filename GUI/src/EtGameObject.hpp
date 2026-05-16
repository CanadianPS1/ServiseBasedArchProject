#pragma once
#include "EtModel.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
namespace et{
    struct TransformComponent{
        glm::vec3 translation{};
        glm::vec3 scale{1.f, 1.f, 1.f};
        glm::vec3 rotation{};
        glm::mat4 mat4();
        glm::mat3 normalMatrix();
    };
    class EtGameObject{
        public:
        using id_t = unsigned int;
        static EtGameObject createGameObject(){
            static id_t currentId = 0;
            return EtGameObject{currentId++};
        }
        EtGameObject(const EtGameObject &) = delete;
        EtGameObject &operator=(const EtGameObject &) = delete;
        EtGameObject(EtGameObject&&) = default;
        EtGameObject &operator=(EtGameObject&&) noexcept = default;
        id_t getId(){return id;}
        std::shared_ptr<EtModel> model{};
        glm::vec3 color{};
        TransformComponent transform{};
        private:
        EtGameObject(id_t objId) : id{objId}{}
        id_t id;
    };
}