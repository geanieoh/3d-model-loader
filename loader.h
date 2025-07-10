#ifndef LOADER_H
#define LOADER_H

#define MAX_LINE 256
#define MAX_VERTICES 100000
#define MAX_FACES 100000

#define VERT_SIZE 3 // x, y, z
#define UV_SIZE 2   // u, v
#define STRIDE 5    // x, y, z, u, v

typedef unsigned char _bool;
#define _true 1
#define _false 0

typedef struct{
    float* vertices;
    unsigned int* indices;
    int vCount;
    int iCount;
} Obj;

_bool obj_load(Obj* model, const char* path, _bool hasUVs);
void obj_delete(Obj* model);

#endif