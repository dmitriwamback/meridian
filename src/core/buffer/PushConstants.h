//
// Created by Dmitri Wamback on 2026-05-30.
//

#ifndef MERIDIAN_PUSHCONSTANTS_H
#define MERIDIAN_PUSHCONSTANTS_H

#include <glm/glm/glm.hpp>

struct PushConstants {
    alignas(16) glm::mat4 model;  // 64 bytes, 16-byte aligned
};

#endif //MERIDIAN_PUSHCONSTANTS_H