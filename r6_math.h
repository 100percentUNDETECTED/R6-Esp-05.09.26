#pragma once
#include <cmath>
#include <cstdint>

struct vector2_t {
    float x = 0, y = 0;
    bool is_zero() const { return x == 0 && y == 0; }
};

struct vector3_t {
    float x = 0, y = 0, z = 0;
    bool is_valid() const { return fabsf(x) < 50000 && fabsf(y) < 50000 && fabsf(z) < 50000; }
    float distance(const vector3_t& o) const {
        float dx = x - o.x, dy = y - o.y, dz = z - o.z;
        return sqrtf(dx * dx + dy * dy + dz * dz);
    }
};

struct entity_t {
    vector3_t position;
    float distance;
    bool is_local;
};
