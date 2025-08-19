#include "loader.h"
#include <stdio.h>
#include <stdlib.h>

#define HASHTABLE_SIZE 10000

#define HASHTABLE_SIZE 10000

typedef struct Node Node;
typedef struct Node{
    int vertIndex;
    int uvIndex;
    int vo;
    Node* next;
} Node;

typedef struct{
    Node* buckets[HASHTABLE_SIZE];
} Hashtable;

unsigned int data_hash(int a, int b){
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

void insert_unique_face(Obj* obj, Hashtable* table, float* tmpVertBuffer, float* vertArray, float* uvArray, int* vo, int a, int b){
    unsigned int hash = data_hash(a, b);
    Node* tmp = table->buckets[hash];
    while(tmp){
        if(tmp->vertIndex == a && tmp->uvIndex == b){
            *vo = tmp->vo;
            return;
        }
        tmp = tmp->next;
    }

    Node* newNode = malloc(sizeof(Node));
    newNode->vertIndex = a;
    newNode->uvIndex = b;
    newNode->vo = obj->vCount / STRIDE;
    newNode->next = NULL;
    hashtable_add_node(table, newNode, hash);

    int offset = obj->vCount;
    tmpVertBuffer[offset + 0] = vertArray[(a - 1)*VERT_SIZE + 0];
    tmpVertBuffer[offset + 1] = vertArray[(a - 1)*VERT_SIZE + 1];
    tmpVertBuffer[offset + 2] = vertArray[(a - 1)*VERT_SIZE + 2];
    tmpVertBuffer[offset + 3] = uvArray[(b - 1)*UV_SIZE + 0];
    tmpVertBuffer[offset + 4] = uvArray[(b - 1)*UV_SIZE + 1];

    *vo = newNode->vo;
    obj->vCount += STRIDE;
}

_bool obj_load(Obj* model, const char* path, _bool hasUVs){
    float* tmpVertArray = malloc(sizeof(float)*MAX_VERTICES*VERT_SIZE);
    float* tmpUVArray = malloc(sizeof(float)*MAX_VERTICES*UV_SIZE);
    float* tmpVBO = malloc(sizeof(float)*MAX_VERTICES*STRIDE);
    float x, y, z;

    int vIndex = 0, uvIndex = 0;
    int index = 0;
    
    char buff[MAX_LINE];

    FILE* file = fopen(path, "r");
    if(!file){
        printf("error opening file: %s\n", path);
        return 0;
    }

    Hashtable table;
    for(int i = 0; i < HASHTABLE_SIZE; i++){
        table.buckets[i] = NULL;
    }

    model->iCount = 0;
    while(fgets(buff, MAX_LINE, file) != NULL){
        if(vIndex > MAX_VERTICES){
            printf("failed to load object, too many vertices\n");
            return 0;
        }

        if(buff[0] == 'v' && buff[1] == ' '){
            sscanf(buff, "v %f %f %f", &x, &y, &z);
            tmpVertArray[vIndex++] = x;
            tmpVertArray[vIndex++] = y;
            tmpVertArray[vIndex++] = z;
        }
        else if(buff[0] == 'v' && buff[1] == 't'){
            sscanf(buff, "vt %f %f", &x, &y);
            tmpUVArray[uvIndex++] = y; /* swapped x and y because the textures were coming out rotated 90 degrees (was x) */
            tmpUVArray[uvIndex++] = x; /* swapped x and y because the textures were coming out rotated 90 degrees (was y) */
        }
        else if(buff[0] == 'f'){
            int a, b, c, d, e, f;
            sscanf(buff, "f %d/%d %d/%d %d/%d", &a, &b, &c, &d, &e, &f);

            model->iCount++;
        }
    }
    rewind(file);

    model->iCount *= VERT_SIZE;
    model->indices = malloc(sizeof(int)*model->iCount);

    model->vCount = 0;
    while(fgets(buff, MAX_LINE, file) != NULL){ //setting up vertex buffer
        if(buff[0] == 'f'){
            int a, b, c, d, e, f;
            sscanf(buff, "f %d/%d %d/%d %d/%d", &a, &b, &c, &d, &e, &f);
            int vo1 = a - 1, vo2 = c - 1, vo3 = e - 1;

            insert_unique_face(model, &table, tmpVBO, tmpVertArray, tmpUVArray, &vo1, a, b);
            insert_unique_face(model, &table, tmpVBO, tmpVertArray, tmpUVArray, &vo2, c, d);
            insert_unique_face(model, &table, tmpVBO, tmpVertArray, tmpUVArray, &vo3, e, f);
            
            model->indices[index] = vo1;
            model->indices[index + 1] = vo2;
            model->indices[index + 2] = vo3;
            index += 3;
        }
    }

    model->vertices = malloc(sizeof(float)*model->vCount);
    for(int i = 0; i < model->vCount; i++){
        model->vertices[i] = tmpVBO[i];
    }

    free(tmpVertArray);
    free(tmpUVArray);
    free(tmpVBO);
    hashtable_free(&table);

    fclose(file);

    return 1;
}

void obj_delete(Obj* model){
    free(model->vertices);
    free(model->indices);
}