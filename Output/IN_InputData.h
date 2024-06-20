#include <vulkan/vulkan.h>
#include <array>

struct vec2 { union { struct { float x; float y; }; float data[2]; }; };
struct vec3 { union { struct { float x; float y; float z; }; float data[3]; }; };
struct vec4 { union { struct { float x; float y; float z; float w; }; float data[4]; }; };

struct ivec2 { union { struct { int x; int y; }; int data[2]; }; };
struct ivec3 { union { struct { int x; int y; int z; }; int data[3]; }; };
struct ivec4 { union { struct { int x; int y; int z; int w; }; int data[4]; }; };

struct mat2x2 { union { float data[2][2]; vec2 rows[2]; }; };
struct mat2x3 { union { float data[2][3]; vec3 rows[2]; }; };
struct mat2x4 { union { float data[2][4]; vec4 rows[2]; }; };
struct mat3x2 { union { float data[3][2]; vec2 rows[3]; }; };
struct mat3x3 { union { float data[3][3]; vec3 rows[3]; }; };
struct mat3x4 { union { float data[3][4]; vec4 rows[3]; }; };
struct mat4x2 { union { float data[4][2]; vec2 rows[4]; }; };
struct mat4x3 { union { float data[4][3]; vec3 rows[4]; }; };
struct mat4x4 { union { float data[4][4]; vec4 rows[4]; }; };
