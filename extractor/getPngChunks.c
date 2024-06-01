#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// CONSTS
#define BYTES_READING_WINDOW 4
#define SIGNATURE_BYTES 8
#define RESULTS_FILENAME_SIZE 12
#define RESULTS_FILE_PATH "./testImages/results/"

// ERRORS
#define NO_FILE_PROVIDED_ERROR 1
#define COULD_NOT_OPEN_FILE_ERROR 2
#define WRONG_FILE_TYPE_ERROR 3
#define COULD_NOT_ALLOC_MEM 4

typedef struct {
    unsigned char* chunkName;
    unsigned char* chunkData;
    unsigned long int chunkStartingByte;
    unsigned long int chunkSize;
} PngChunk;

// TODO: add custom chunks handler
// TODO: handle fputs possible errors and possible chunk reading errors
// TODO: create results dir using mkdir + handle possible errors

PngChunk* readChunk(FILE* image);
long int unCharToLI(unsigned char* num);
char* generateRandomFilename();
void writeChunkToJson(PngChunk* chunk, FILE* file);
void printChunkInfo(PngChunk* chunk);

int main(int argc, const char *argv[]) {
    if (argv[1] == NULL) {
        fprintf(stderr, "ERR: no file is provided");
        exit(NO_FILE_PROVIDED_ERROR);
    }

    const char* imagePath = argv[1];
    FILE* image = fopen(imagePath, "rb");
    if (image == NULL) {
        fprintf(stderr, "ERR: could not open file %s", imagePath);
        exit(COULD_NOT_OPEN_FILE_ERROR);
    }
    
    unsigned char signature[SIGNATURE_BYTES + 1];
    fread(signature, SIGNATURE_BYTES, 1, image);
    if (strcmp(signature, "\211PNG\r\n\032\n") != 0) {
        fprintf(stderr, "ERR: File %s is not a PNG file. Wrong signature", imagePath);
        exit(WRONG_FILE_TYPE_ERROR);
    }

    srand(time(NULL));
    char* resultsFileName = generateRandomFilename();
    char* resultsFilePath = malloc(sizeof(char) * (strlen(RESULTS_FILE_PATH) + RESULTS_FILENAME_SIZE + 1));
    if (resultsFilePath == NULL) {
        fprintf(stderr, "ERR: Failed to allocate memory to results file path");
        exit(COULD_NOT_ALLOC_MEM);
    }

    strcpy(resultsFilePath, RESULTS_FILE_PATH);
    strcat(resultsFilePath, resultsFileName);
    
    FILE* result = fopen(resultsFilePath, "wb");
    if (result == NULL) {
        fprintf(stderr, "ERR: could not open file %s", result);
        exit(COULD_NOT_OPEN_FILE_ERROR);
    }

    fputs("{", result);
    fputs("\"chunks\": [", result);

    int k = 0;
    while(!feof(image)) {
        PngChunk* chunk = readChunk(image);

        writeChunkToJson(chunk, result);
        if (strcmp(chunk->chunkName, "IEND") == 0) break; 
        else fputs(",\n", result);

        k += 1;
    }

    fputs("]", result);
    fputs("}", result);

    fclose(result);
    fclose(image);

    return 0;
}

PngChunk* readChunk(FILE* image) {
    PngChunk* chunk = malloc(sizeof(PngChunk));
    if (chunk == NULL) {
        fprintf(stderr, "ERR: Failed to allocate memory to chunk");
        exit(COULD_NOT_ALLOC_MEM);
    }

    unsigned char* window = malloc(sizeof(unsigned char) * BYTES_READING_WINDOW + 1);
    if (window == NULL) {
        fprintf(stderr, "ERR: Failed to allocate memory to chunk window");
        exit(COULD_NOT_ALLOC_MEM);
    }

    fread(window, BYTES_READING_WINDOW, 1, image);
    chunk->chunkStartingByte = ftell(image) - BYTES_READING_WINDOW;
    chunk->chunkSize = unCharToLI(window);
    fread(window, BYTES_READING_WINDOW, 1, image);
    window[BYTES_READING_WINDOW] = '\0';
    chunk->chunkName = window;
    
    if (strcmp(window, "tEXt") == 0 || strcmp(window, "zTXt") == 0 || strcmp(window, "iTXt") == 0) {
        unsigned char* data = malloc(chunk->chunkSize + 1);
        if (data == NULL) {
            fprintf(stderr, "ERR: Failed to allocate memory to chunk data");
            exit(COULD_NOT_ALLOC_MEM);
        }

        for (int i = 0; i < chunk->chunkSize; i++) {
            unsigned char c = fgetc(image);

            if (c == '\n' || c == '\0') {
                data[i] = ' ';
            } else if (c == '\"') {
                data[i] = '\'';
            } else {
                data[i] = c;
            }
        }
        data[chunk->chunkSize] = '\0';
        chunk->chunkData = data;
        fseek(image, 4, SEEK_CUR);
    } else {
        fseek(image, chunk->chunkSize + 4, SEEK_CUR);
        chunk->chunkData = NULL;
    }

    return chunk;
}

long int unCharToLI(unsigned char* num) {
    long int res;
    for(int i = 0; i < BYTES_READING_WINDOW; i++) {
        res = (res << 8) | num[i];
    }

    return res;
}

char* generateRandomFilename() {
    const char* alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    // Filename size + 5 for .json + 1 for null-term
    char* fileName = malloc((RESULTS_FILENAME_SIZE + 6) * sizeof(char));

    for (int i = 0; i < RESULTS_FILENAME_SIZE; i++) {
        *(fileName + i) = *(alphabet + (rand() % 62));
    }

    strcat(fileName, ".json");

    *(fileName + strlen(fileName)) = '\0';

    return fileName;
}

void writeChunkToJson(PngChunk* chunk, FILE* file) {
    fputs("{", file);
    fputs("\"start-byte\" : ", file);
    fprintf(file, "%li", chunk->chunkStartingByte);
    fputs(", ", file);

    fputs("\"size\" : ", file);
    fprintf(file, "%li", chunk->chunkSize);
    fputs(", ", file);

    fputs("\"name\": ", file);
    fputs("\"", file);
    fputs(chunk->chunkName, file);
    fputs("\"", file);
    fputs(", ", file);

    fputs("\"data\": ", file);
    if (chunk->chunkData == NULL) {
        fputs("\"null\"", file);
    } else {
        fputs("\"", file);
        fputs(chunk->chunkData, file);
        fputs("\"", file);
    }
    fputs("}", file);

    return;
}

void printChunkInfo(PngChunk* chunk) {
    printf("NAME: %s, SIZE: %li, START_PTR: %li\n", chunk->chunkName, chunk->chunkSize, chunk->chunkStartingByte);
    if (chunk->chunkData != NULL) {
        printf("DATA: ");
        for (int i = 0; i < chunk->chunkSize; i++) {
            printf("%c", chunk->chunkData[i]);
        }
        printf("\n");
    }

    return;
}
