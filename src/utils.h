#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>

inline float randF(float lo, float hi)
{
    return lo + (float)rand() / RAND_MAX * (hi - lo);
}

// Uniform random direction on the unit sphere (Marsaglia cylindrical method).
inline glm::vec3 randUnitVec()
{
    float u     = randF(-1.f, 1.f);
    float theta = randF(0.f, 2.f * glm::pi<float>());
    float r     = std::sqrt(std::max(0.f, 1.f - u * u));
    return { r * std::cos(theta), u, r * std::sin(theta) };
}
