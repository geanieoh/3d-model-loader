#include "loader.h"
#include <stdio.h>
#include <stdlib.h>

void insert_unique_face(Obj* obj, int* uniqueEntries, float* tmpVertBuffer, float* vertArray, float* uvArray, int* vo, int* largest, int a, int b){
    _bool dupeVertFound = _false, nextAvailableIndexFound = _false, vertSeen = _false;
    int nextAvailableIndex;
    for(int i = 0; i < obj->iCount*2; i+=2){
        if(uniqueEntries[i] == 0 && uniqueEntries[i + 1] == 0 && !nextAvailableIndexFound){
            nextAvailableIndex = i;
            nextAvailableIndexFound = _true;
        }
        if(uniqueEntries[i] == a){
            vertSeen = 1;
            if(uniqueEntries[i + 1] == b){
                dupeVertFound = _true;
                break;
            }
        }
    }

    if(!dupeVertFound){
        uniqueEntries[nextAvailableIndex] = a; uniqueEntries[nextAvailableIndex + 1] = b;
        if(!vertSeen){
            tmpVertBuffer[(a - 1)*STRIDE] = vertArray[(a - 1)*VERT_SIZE];
            tmpVertBuffer[(a - 1)*STRIDE + 1] = vertArray[(a - 1)*VERT_SIZE + 1];
            tmpVertBuffer[(a - 1)*STRIDE + 2] = vertArray[(a - 1)*VERT_SIZE + 2];
            tmpVertBuffer[(a - 1)*STRIDE + 3] = uvArray[(b - 1)*UV_SIZE];
            tmpVertBuffer[(a - 1)*STRIDE + 4] = uvArray[(b - 1)*UV_SIZE + 1];
        }
        else{
            *vo = *largest/STRIDE;
            tmpVertBuffer[*largest] = vertArray[(a - 1)*VERT_SIZE];
            tmpVertBuffer[*largest + 1] = vertArray[(a - 1)*VERT_SIZE + 1];
            tmpVertBuffer[*largest + 2] = vertArray[(a - 1)*VERT_SIZE + 2];
            tmpVertBuffer[*largest + 3] = uvArray[(b - 1)*UV_SIZE];
            tmpVertBuffer[*largest + 4] = uvArray[(b - 1)*UV_SIZE + 1];
            *largest += STRIDE;
        }
        obj->vCount += STRIDE;
    }
}

_bool obj_load(Obj* model, const char* path, _bool hasUVs){
    float tmpVertArray[MAX_VERTICES];
    float tmpUVArray[MAX_VERTICES];
    float* tmpVBO = malloc(sizeof(float)*MAX_VERTICES);
    float x, y, z;

    int vIndex = 0, uvIndex = 0, largest = 0;
    int a, b, c, d, e, f, index = 0;
    
    char buff[MAX_LINE];

    FILE* file = fopen(path, "r");
    if (!file) {
        printf("error opening file: %s\n", path);
        return _false;
    }

    model->iCount = 0;
    while(fgets(buff, MAX_LINE, file) != NULL){
        if(vIndex > MAX_VERTICES){
            printf("failed to load object, too many vertices\n");
            return _false;
        }

        if(buff[0] == 'v' && buff[1] == ' '){
            sscanf(buff, "v %f %f %f", &x, &y, &z);
            tmpVertArray[vIndex++] = x;
            tmpVertArray[vIndex++] = y;
            tmpVertArray[vIndex++] = z;
        }
        else if(buff[0] == 'v' && buff[1] == 't'){
            sscanf(buff, "vt %f %f", &x, &y);
            tmpUVArray[uvIndex++] = x;
            tmpUVArray[uvIndex++] = y;
        }
        else if(buff[0] == 'f'){
            if(hasUVs){
                sscanf(buff, "f %d/%d %d/%d %d/%d", &a, &b, &c, &d, &e, &f);
                if(a > largest) largest = a;
                if(c > largest) largest = c;
                if(e > largest) largest = e;
            }

            model->iCount++;
        }
    }
    rewind(file);

    model->iCount *= VERT_SIZE;
    model->indices = malloc(sizeof(int)*model->iCount);
    int* uniqueEntries;
    if(hasUVs) uniqueEntries = calloc(model->iCount*UV_SIZE, sizeof(int));

    model->vCount = 0;
    if(hasUVs) largest *= STRIDE;
    while(fgets(buff, MAX_LINE, file) != NULL){ //setting up vertex buffer
        if(buff[0] == 'f'){
            if(hasUVs) sscanf(buff, "f %d/%d %d/%d %d/%d", &a, &b, &c, &d, &e, &f);
            else sscanf(buff, "f %d %d %d", &a, &c, &e);
            int vo1 = a - 1, vo2 = c - 1, vo3 = e - 1;

            if(hasUVs){
                insert_unique_face(model, uniqueEntries, tmpVBO, tmpVertArray, tmpUVArray, &vo1, &largest, a, b);
                insert_unique_face(model, uniqueEntries, tmpVBO, tmpVertArray, tmpUVArray, &vo2, &largest, c, d);
                insert_unique_face(model, uniqueEntries, tmpVBO, tmpVertArray, tmpUVArray, &vo3, &largest, e, f);
            }
            else{
                model->vCount += VERT_SIZE;
            }
            
            model->indices[index] = vo1;
            model->indices[index + 1] = vo2;
            model->indices[index + 2] = vo3;
            index += 3;
        }
    }

    model->vertices = malloc(sizeof(float)*model->vCount);
    if(hasUVs){
        for(int i = 0; i < model->vCount; i++){
            model->vertices[i] = tmpVBO[i];
        }
    }
    else{
        for(int i = 0; i < model->vCount; i++){
            model->vertices[i] = tmpVertArray[i];
        }
    }
    
    free(tmpVBO);
    if(hasUVs) free(uniqueEntries);

    fclose(file);

    return _true;
}

void obj_delete(Obj* model){
    free(model->vertices);
    free(model->indices);
}