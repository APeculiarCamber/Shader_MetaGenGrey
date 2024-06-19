//
// Created by idemaj on 6/19/24.
//

#ifndef SHADER_METAGEN_TYPEDESCRIPTORS_H
#define SHADER_METAGEN_TYPEDESCRIPTORS_H
#include <iostream>

// Macro to generate a vector of size N with member variables x, y, z, w
#define GENERATE_VECTOR(N, ...)            \
struct vec##N {                            \
    union {                                \
        struct { __VA_ARGS__ };            \
        float data[N];                     \
    };                                     \
};

#define GENERATE_IVECTOR(N, ...)            \
struct ivec##N {                            \
    union {                                \
        struct { __VA_ARGS__ };            \
        int data[N];                      \
    };                                     \
};

// Macro to generate a matrix of size MxN
#define GENERATE_MATRIX(M, N)              \
struct mat##M##x##N {                      \
    union {                                \
        float data[M][N];                  \
        vec##N  rows[M];                   \
    };                                     \
};

// Generate vectors of size 2 to 4
GENERATE_VECTOR(2, float x; float y;)
GENERATE_VECTOR(3, float x; float y; float z;)
GENERATE_VECTOR(4, float x; float y; float z; float w;)

GENERATE_IVECTOR(2, int x; int y;)
GENERATE_IVECTOR(3, int x; int y; int z;)
GENERATE_IVECTOR(4, int x; int y; int z; int w;)

// Generate matrices of size 2x2 to 4x4
GENERATE_MATRIX(2, 2)
GENERATE_MATRIX(2, 3)
GENERATE_MATRIX(2, 4)
GENERATE_MATRIX(3, 2)
GENERATE_MATRIX(3, 3)
GENERATE_MATRIX(3, 4)
GENERATE_MATRIX(4, 2)
GENERATE_MATRIX(4, 3)
GENERATE_MATRIX(4, 4)


#endif //SHADER_METAGEN_TYPEDESCRIPTORS_H
