#include "loader.h"
#include <stdio.h>
#include <stdlib.h>

#define HASHTABLE_SIZE 10000

typedef struct Node Node;
typedef struct Node{
    int vertIndex;
    int uvIndex;
    Node* next;
} Node;

typedef struct{
    Node* buckets[HASHTABLE_SIZE];
} Hashtable;

unsigned int hashtable_hash(int a, int b){
    unsigned int result = (a*393241) ^ (b*786433);
    return result%HASHTABLE_SIZE;
}

void hashtable_add_node(Hashtable* table, Node* node, unsigned int hash){
    if(table->buckets[hash] == NULL){
        table->buckets[hash] = node;
        return;
    }

    Node* tmp = table->buckets[hash];
    while(tmp->next){
        tmp = tmp->next;
    }

    tmp->next = node;
}

void hashtable_free(Hashtable* table) {
    for (int i = 0; i < HASHTABLE_SIZE; ++i) {
        Node* node = table->buckets[i];
        while (node) {
            Node* next = node->next;
            free(node);
            node = next;
        }
    }
}

void insert_unique_face(Obj* obj, Hashtable* table, float* tmpVertBuffer, float* vertArray, float* uvArray, int* vo, int* largest, int a, int b){
    _bool dupeVertFound = _false, vertSeen = _false;

    unsigned int hash = hashtable_hash(a, b);
    Node* tmp = table->buckets[hash];
    if(tmp != NULL){
        while(tmp){
            if(tmp->vertIndex == a){
                vertSeen = _true;
                if(tmp->uvIndex == b){
                    dupeVertFound = _true;
                    break;
                }
            }
            tmp = tmp->next;
        }
    }

    if(!dupeVertFound){
        Node* newNode = malloc(sizeof(Node));
        newNode->vertIndex = a;
        newNode->uvIndex = b;
        newNode->next = NULL;
        hashtable_add_node(table, newNode, hash);
        if(!vertSeen){
            tmpVertBuffer[(a - 1)*STRIDE] = vertArray[(a - 1)*VERT_SIZE];
            tmpVertBuffer[(a - 1)*STRIDE + 1] = vertArray[(a - 1)*VERT_SIZE + 1];
            tmpVertBuffer[(a - 1)*STRIDE + 2] = vertArray[(a - 1)*VERT_SIZE + 2];
            tmpVertBuffer[(a - 1)*STRIDE + 3] = uvArray[(b - 1)*UV_SIZE];
            tmpVertBuffer[(a - 1)*STRIDE + 4] = uvArray[(b - 1)*UV_SIZE + 1];
            printf("%d: %d\t%d\t\n", b, (b - 1)*UV_SIZE + 1, MAX_VERTICES*UV_SIZE);
        }
        else{
            *vo = *largest/STRIDE;
            tmpVertBuffer[*largest] = vertArray[(a - 1)*VERT_SIZE];
            tmpVertBuffer[*largest + 1] = vertArray[(a - 1)*VERT_SIZE + 1];
            tmpVertBuffer[*largest + 2] = vertArray[(a - 1)*VERT_SIZE + 2];
            tmpVertBuffer[*largest + 3] = uvArray[(b - 1)*UV_SIZE];
            tmpVertBuffer[*largest + 4] = uvArray[(b - 1)*UV_SIZE + 1];
            *largest += STRIDE;
            //printf("%d\t%d\t\n", (b - 1)*UV_SIZE + 1, MAX_VERTICES*UV_SIZE);
        }
        obj->vCount += STRIDE;
    }
}

_bool obj_load(Obj* model, const char* path, _bool hasUVs){
    float* tmpVertArray = malloc(sizeof(float)*MAX_VERTICES*VERT_SIZE);
    float* tmpUVArray = malloc(sizeof(float)*MAX_VERTICES*UV_SIZE);
    float* tmpVBO = malloc(sizeof(float)*MAX_VERTICES*STRIDE);
    float x, y, z;

    int vIndex = 0, uvIndex = 0, largest = 0;
    int index = 0;
    
    char buff[MAX_LINE];

    FILE* file = fopen(path, "r");
    if (!file) {
        printf("error opening file: %s\n", path);
        return _false;
    }

    Hashtable table;
    if(hasUVs){
        for(int i = 0; i < HASHTABLE_SIZE; i++){
            table.buckets[i] = NULL;
        }
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
                int a, b, c, d, e, f;
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

    model->vCount = 0;
    if(hasUVs) largest *= STRIDE;
    while(fgets(buff, MAX_LINE, file) != NULL){ //setting up vertex buffer
        if(buff[0] == 'f'){
            int a, b, c, d, e, f;
            if(hasUVs) sscanf(buff, "f %d/%d %d/%d %d/%d", &a, &b, &c, &d, &e, &f);
            else sscanf(buff, "f %d %d %d", &a, &c, &e);
            int vo1 = a - 1, vo2 = c - 1, vo3 = e - 1;

            if(hasUVs){
                insert_unique_face(model, &table, tmpVBO, tmpVertArray, tmpUVArray, &vo1, &largest, a, b);
                insert_unique_face(model, &table, tmpVBO, tmpVertArray, tmpUVArray, &vo2, &largest, c, d);
                insert_unique_face(model, &table, tmpVBO, tmpVertArray, tmpUVArray, &vo3, &largest, e, f);
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

    free(tmpVertArray);
    free(tmpUVArray);
    free(tmpVBO);
    if(hasUVs) hashtable_free(&table);

    fclose(file);

    return _true;
}

void obj_delete(Obj* model){
    free(model->vertices);
    free(model->indices);
}