#pragma once
#include "model.h"

class Asteroid {
public:
    Mesh mesh;

    // seed     : changes the noise offset so every asteroid looks different
    // rings    : latitude subdivisions of the base sphere
    // sectors  : longitude subdivisions of the base sphere
    void generate(int seed = 0, int rings = 32, int sectors = 32);
    void free() { mesh.free(); }
};
