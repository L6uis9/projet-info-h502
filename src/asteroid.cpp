#include "asteroid.h"

#include <glm/gtc/constants.hpp>
#include <cmath>
#include <vector>
#include <cstdlib>

// Perlin gradient noise over R³  — introduced in "Texturing and Modeling", Ch.12, (p.344-347) needed for ridged multifractal in Ch.16

// Macros

#define B   256
#define BM  (B - 1)

// Dot product between gradient vector and distance vector
#define AT(q, rx, ry, rz)  ((rx)*(q)[0] + (ry)*(q)[1] + (rz)*(q)[2])

// Hermite S-curve function for interpolation
#define S_CURVE(t)  ((t)*(t)*(3.f - 2.f*(t)))

#define LERP(t, a, b)  ((a) + (t)*((b)-(a)))

// setup(i, b0, b1, r0, r1) macro
#define SETUP(vec, i, b0, b1, r0, r1)          \
    do {                                       \
        float _t = (vec)[(i)] + 10000.f;       \
        (b0) = ((int)_t) & BM;                 \
        (b1) = ((b0)+1) & BM;                  \
        (r0) = _t - (int)_t;                   \
        (r1) = (r0) - 1.f;                     \
    } while(0)

static int   perm[B + B + 2];
static float grad[B + B + 2][3];
static bool  noiseReady = false;

// init() of the book

static void noiseInit()
{
    noiseReady = true;
    srandom(1);

    for (int i = 0; i < B; i++) {
        float v[3], s;
        do {
            for (int j = 0; j < 3; j++)
                v[j] = (float)((random() % (B + B)) - B) / (float)B;
            s = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
        } while (s > 1.f);
        s = std::sqrt(s);
        for (int j = 0; j < 3; j++)
            grad[i][j] = v[j] / s;
    }

    /* Create a pseudorandom permutation of [1 .. B] */
    for (int i = 0; i < B; i++)
        perm[i] = i;
    for (int i = B; i > 0; i -= 2) {
        int k   = perm[i];
        int j   = (int)(random() % B);
        perm[i] = perm[j];
        perm[j] = k;
    }

    /* Extend g and p arrays to allow for faster indexing, */
    for (int i = 0; i < B + 2; i++) {
        perm[B + i] = perm[i];
        for (int j = 0; j < 3; j++)
            grad[B + i][j] = grad[i][j];
    }
}

// noise3() of the book

static float noise3(float vec[3])
{
    if (!noiseReady) noiseInit();

    int   bx0, bx1, by0, by1, bz0, bz1;
    float rx0, rx1, ry0, ry1, rz0, rz1;

    SETUP(vec, 0, bx0, bx1, rx0, rx1);
    SETUP(vec, 1, by0, by1, ry0, ry1);
    SETUP(vec, 2, bz0, bz1, rz0, rz1);

    int i   = perm[bx0];
    int j   = perm[bx1];
    int b00 = perm[i + by0];
    int b10 = perm[j + by0];
    int b01 = perm[i + by1];
    int b11 = perm[j + by1];

    float sx = S_CURVE(rx0);
    float sy = S_CURVE(ry0);
    float sz = S_CURVE(rz0);

    float *q, u, v, a, b, c, d;

    q = grad[b00 + bz0];  u = AT(q, rx0, ry0, rz0);
    q = grad[b10 + bz0];  v = AT(q, rx1, ry0, rz0);
    a = LERP(sx, u, v);

    q = grad[b01 + bz0];  u = AT(q, rx0, ry1, rz0);
    q = grad[b11 + bz0];  v = AT(q, rx1, ry1, rz0);
    b = LERP(sx, u, v);
    c = LERP(sy, a, b); /* interpolate in y at low x */

    q = grad[b00 + bz1];  u = AT(q, rx0, ry0, rz1);
    q = grad[b10 + bz1];  v = AT(q, rx1, ry0, rz1);
    a = LERP(sx, u, v);

    q = grad[b01 + bz1];  u = AT(q, rx0, ry1, rz1);
    q = grad[b11 + bz1];  v = AT(q, rx1, ry1, rz1);
    b = LERP(sx, u, v);
    d = LERP(sy, a, b); /* interpolate in y at high x */

    return 1.5f * LERP(sz, c, d); /* interpolate in z */
}

// Surcharge for glm::vec3
static float noise3(glm::vec3 p)
{
    float v[3] = { p.x, p.y, p.z };
    return noise3(v);
}


// Ridged Multifractal - Chapter 16 of the book, PROCEDURAL FRACTAL TERRAINS, p. 504-505
// Recommended parameters : H=1.0, lacunarity=2.0, offset=1.0, gain=2.0

static float ridgedMultifractal(glm::vec3 p,
                                float H, float lacunarity, int octaves,
                                float offset, float gain)
{
    float exponents[16];
    float freq = 1.f;
    for (int i = 0; i <= octaves; i++) {
        /* compute weight for each frequency */
        exponents[i] = std::pow(freq, -H);
        freq *= lacunarity;
    }

    /* get first octave */
    float signal = noise3(p);

    /* get absolute value of signal (this creates the ridges) */
    if (signal < 0.f) signal = -signal;

    /* invert and translate (note that “offset” should be ~= 1.0) */
    signal  = offset - signal;

    /* square the signal, to increase “sharpness” of ridges */
    signal *= signal;

    /* assign initial values */
    float result = signal;
    float weight = 1.f;

    for (int i = 1; i < octaves; i++) {

        /* increase the frequency */
        p.x *= lacunarity;
        p.y *= lacunarity;
        p.z *= lacunarity;

        /* weight successive contributions by previous signal */
        weight = signal * gain;
        if (weight > 1.f) weight = 1.f;
        if (weight < 0.f) weight = 0.f;
        signal = noise3(p);
        if (signal < 0.f) signal = -signal;
        signal  = offset - signal;
        signal *= signal;

        /* weight the contribution */
        signal *= weight;
        result += signal * exponents[i];
    }
    return result;
}


// Generate asteroid
void Asteroid::generate(int seed, int rings, int sectors)
{

    const float seedOffset = (float)seed * 13.7f;
    const float noiseScale = 1.5f;
    const float amplitude  = 0.28f;

    // ridged MF parameters
    const float H          = 1.0f;
    const float lacunarity = 2.0f;
    const int   octaves    = 6;
    const float offset     = 1.0f;
    const float gain       = 2.0f;

    const float hMid = 1.0f;

    if (!noiseReady) noiseInit();

    int numVerts = (rings + 1) * (sectors + 1);
    std::vector<glm::vec3> positions(numVerts);
    std::vector<glm::vec3> normals(numVerts, glm::vec3(0.f));
    std::vector<glm::vec2> texCoords(numVerts);

    // Sphere vertices with noise-based displacement
    for (int r = 0; r <= rings; r++) {
        float phi = glm::pi<float>() * r / rings;
        for (int s = 0; s <= sectors; s++) {
            float theta = 2.f * glm::pi<float>() * s / sectors;

            glm::vec3 dir = {
                std::sin(phi) * std::cos(theta),
                std::cos(phi),
                std::sin(phi) * std::sin(theta)
            };

            glm::vec3 noisePos = dir * noiseScale + glm::vec3(seedOffset);
            float h      = ridgedMultifractal(noisePos, H, lacunarity, octaves, offset, gain);
            float radius = 1.0f + (h - hMid) * amplitude;

            positions [r * (sectors + 1) + s] = dir * radius;
            texCoords [r * (sectors + 1) + s] = { (float)s / sectors, (float)r / rings };
        }
    }

    // indices and normals
    std::vector<uint32_t> indices;
    indices.reserve(rings * sectors * 6);

    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < sectors; s++) {
            uint32_t i0 = (uint32_t)( r      * (sectors + 1) + s);
            uint32_t i1 = (uint32_t)( r      * (sectors + 1) + s + 1);
            uint32_t i2 = (uint32_t)((r + 1) * (sectors + 1) + s);
            uint32_t i3 = (uint32_t)((r + 1) * (sectors + 1) + s + 1);

            /*
            i0 ── i1
            │   /  │
            │  /   │
            i2 ── i3
            */

            // 1st Triangle : i0, i1, i2
            {
                glm::vec3 n = glm::cross(positions[i1] - positions[i0],
                                         positions[i2] - positions[i0]);
                normals[i0] += n; normals[i1] += n; normals[i2] += n;
                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);
            }
            // 2nd Triangle : i1, i3, i2
            {
                glm::vec3 n = glm::cross(positions[i3] - positions[i1],
                                         positions[i2] - positions[i1]);
                normals[i1] += n; normals[i3] += n; normals[i2] += n;
                indices.push_back(i1);
                indices.push_back(i3);
                indices.push_back(i2);
            }
        }
    }

    // assemble mesh
    mesh.vertices.resize(numVerts);
    for (int i = 0; i < numVerts; i++) {
        mesh.vertices[i].position = positions[i];
        mesh.vertices[i].normal   = glm::normalize(normals[i]);
        mesh.vertices[i].texCoord = texCoords[i];
    }
    mesh.indices = std::move(indices);
    mesh.material.Kd = { 0.52f, 0.48f, 0.44f };
    mesh.upload();
}