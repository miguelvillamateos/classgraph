#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define NOMATCH 1

int match(const char *pattern, const char *str);
void listDirectoryRecursively(const char *basePath, const char *pattern, FILE *outputFile, const char *excludePath);
void findAndSavePhpClass(const char *filePath, FILE *outputFile);
void generateGraph(const char *listFileName, const char *graphFileName);

typedef struct {
    char *name;
    char *parent;
    char *directoryPath;
} ClaseInfo;

char* sanitizeForDot(const char* input) {
    if (input == NULL) return NULL;
    char* sanitized = strdup(input);
    if (sanitized == NULL) return NULL;
    for (int i = 0; sanitized[i] != '\0'; i++) {
        if (!isalnum(sanitized[i])) {
            sanitized[i] = '_';
        }
    }
    return sanitized;
}

char* getDirectory(const char* filePath) {
    if (filePath == NULL) return NULL;
    char* pathCopy = strdup(filePath);
    if (pathCopy == NULL) return NULL;

    char* lastSeparator = strrchr(pathCopy, '/');
    

    if (lastSeparator != NULL) {
        *lastSeparator = '\0';
    } else {
        free(pathCopy);
        return strdup(".");
    }
    return pathCopy;
}

void parseDefinition(const char* definition, char** className, char** parentName) {
    char buffer[1024];
    strncpy(buffer, definition, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char* token;
    char* rest = buffer;
    int esClase = 0;

    token = strtok_r(rest, " \t\n\r", &rest);
    while(token != NULL) {
        if (strcmp(token, "class") == 0 || strcmp(token, "interface") == 0 || strcmp(token, "trait") == 0) {
            esClase = 1;
            break;
        }
        token = strtok_r(rest, " \t\n\r", &rest);
    }

    if (!esClase) return;

    token = strtok_r(rest, " \t\n\r", &rest);
    if (token != NULL) {
        *className = strdup(token);
    } else {
        return;
    }

    token = strtok_r(rest, " \t\n\r", &rest);
    if (token != NULL && strcmp(token, "extends") == 0) {
        token = strtok_r(rest, " \t\n\r", &rest);
        if (token != NULL) {
            *parentName = strdup(token);
        }
    }
}


void findAndSavePhpClass(const char *filePath, FILE *outputFile) {
    FILE *inputFile = fopen(filePath, "r");
    if (inputFile == NULL) {
        fprintf(stderr, "No se pudo abrir el fichero para lectura: %s\n", filePath);
        return;
    }

    char linea[1024];
    char linea_limpia[1024];    
    int in_multiline_comment = 0;

    while (fgets(linea, sizeof(linea), inputFile) != NULL) {
        char *p = linea;
        char *q = linea_limpia; // clean_line

        while (*p) {
            if (in_multiline_comment) {
                if (*p == '*' && *(p + 1) == '/') {
                    in_multiline_comment = 0;
                    p += 2;
                } else {
                    p++;
                }
            } else {
                if (*p == '/' && *(p + 1) == '*') {
                    in_multiline_comment = 1;
                    p += 2;
                } else {
                    *q++ = *p++;
                }
            }
        }
        *q = '\0';

        char *comment_slash = strstr(linea_limpia, "//");
        if (comment_slash) *comment_slash = '\0';
        char *comment_hash = strstr(linea_limpia, "#");
        if (comment_hash) *comment_hash = '\0';

        const char *keywords[] = {"class ", "interface ", "trait ", "abstract class ", "final class "};
        const char *found_keyword = NULL;

        for (size_t i = 0; i < sizeof(keywords)/sizeof(keywords[0]); ++i) {
            char *p_key = strstr(linea_limpia, keywords[i]);
            if (p_key != NULL) {
                if ((p_key == linea_limpia || isspace(*(p_key-1)) || *(p_key-1) == '{' || *(p_key-1) == '(') && strstr(p_key, "::class") == NULL) {
                    found_keyword = keywords[i];
                    break;
                }
            }
        }

        if (found_keyword != NULL) {
            fprintf(outputFile, "%s\n", filePath);
            char *start = linea_limpia;
            while(isspace(*start)) start++;
            char *end = start + strlen(start) - 1;
            while(end > start && (isspace(*end) || *end == '\n' || *end == '\r')) *end-- = '\0';

            while(end > start && (*end == '{' || *end == '(' || isspace(*end))) {
                *end-- = '\0';
            }

            fprintf(outputFile, "  -> %s\n", start);
        }
    }

    fclose(inputFile);
}


void listDirectoryRecursively(const char *basePath, const char *pattern, FILE *outputFile, const char *excludePath)
{
    DIR *dir = opendir(basePath);
    if (dir == NULL)
    {
        fprintf(stderr, "Could not open directory: %s\n", basePath);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char pathCompleto[1024];
        snprintf(pathCompleto, sizeof(pathCompleto), "%s/%s", basePath, entry->d_name);

        if (excludePath != NULL) {
            char excludeCopy[1024];
            strncpy(excludeCopy, excludePath, sizeof(excludeCopy) - 1);
            excludeCopy[sizeof(excludeCopy) - 1] = '\0';

            int excluded = 0;
            char *token = strtok(excludeCopy, ";");
            while (token != NULL) {
                if (match(token, pathCompleto) == 0) {
                    excluded = 1;
                    break;
                }
                token = strtok(NULL, ";");
            }
            if (excluded) continue;
        }

        struct stat statbuf;
        if (stat(pathCompleto, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                listDirectoryRecursively(pathCompleto, pattern, outputFile, excludePath);
            } else if (S_ISREG(statbuf.st_mode)) {
                if (match(pattern, entry->d_name) == 0) {
                    findAndSavePhpClass(pathCompleto, outputFile);
                }
            }
        }
    }

    closedir(dir);
}

void generateGraph(const char *listFileName, const char *graphFileName) {
    FILE *listFile = fopen(listFileName, "r");
    if (listFile == NULL) {
        perror("Error al abrir el fichero de lista para generar el gráfico");
        return;
    }

    printf("Generando gráfico de dependencias en '%s'...\n", graphFileName);

    ClaseInfo *classes = NULL;
    int numClasses = 0;
    int capacity = 0;

    char linea[2048];
    char currentFilePath[1024] = {0};

    while (fgets(linea, sizeof(linea), listFile)) {
        linea[strcspn(linea, "\r\n")] = 0;

        char *separator = strstr(linea, " -> ");
        if (separator != NULL) {
            *separator = '\0';
            char *definition = separator + 4;

            char *className = NULL;
            char *parentName = NULL;
            parseDefinition(definition, &className, &parentName);

            if (className != NULL) {
                if (numClasses >= capacity) {
                    capacity = (capacity == 0) ? 256 : capacity * 2;
                    ClaseInfo *temp_clases = realloc(classes, capacity * sizeof(ClaseInfo));
                    if (temp_clases == NULL) {
                        perror("Error: Fallo al realocar memoria para las clases");
                        free(className);
                        free(parentName);
                        for (int i = 0; i < numClasses; i++) {
                            free(classes[i].name);
                            free(classes[i].parent);
                            free(classes[i].directoryPath);
                        }
                        free(classes);
                        fclose(listFile);
                        return;
                    }
                    classes = temp_clases;
                }
                classes[numClasses].name = className;
                classes[numClasses].parent = parentName;
                classes[numClasses].directoryPath = getDirectory(currentFilePath);
                numClasses++;
            } else {
                free(parentName);
            }
        } else {
            strncpy(currentFilePath, linea, sizeof(currentFilePath) - 1);
            currentFilePath[sizeof(currentFilePath) - 1] = '\0';
        }
    }
    fclose(listFile);

    FILE *graphFile = fopen(graphFileName, "w");
    if (graphFile == NULL) {
        perror("Error al crear el fichero para el gráfico");
        for (int i = 0; i < numClasses; i++) {
            free(classes[i].name);
            free(classes[i].parent);
            free(classes[i].directoryPath);
        }
        free(classes);
        return;
    }

    fprintf(graphFile, "digraph G {\n");
    fprintf(graphFile, "    rankdir=BT;\n");
    fprintf(graphFile, "    node [shape=box, style=filled, fillcolor=white];\n");
    fprintf(graphFile, "    edge [arrowhead=empty];\n\n");

    for (int i = 0; i < numClasses; i++) {
        if (classes[i].directoryPath == NULL) {
            continue;
        }

         
        char* idCluster = sanitizeForDot(classes[i].directoryPath);
        if (idCluster == NULL) continue;
       
        
        fprintf(graphFile, "    subgraph \"cluster_%s\" {\n", idCluster);
        fprintf(graphFile, "        label = \"%s\";\n", classes[i].directoryPath);
        fprintf(graphFile, "        style=\"filled,dotted\";\n");
        fprintf(graphFile, "        fillcolor=white;\n");

        for (int j = 0; j < numClasses; j++) {
            if (classes[j].directoryPath != NULL && strcmp(classes[i].directoryPath, classes[j].directoryPath) == 0) {
                fprintf(graphFile, "        \"%s\";\n", classes[j].name);
            }
        }
        fprintf(graphFile, "    }\n\n");

        char* dirProcesado = classes[i].directoryPath;
        for (int j = 0; j < numClasses; j++) {
            if (classes[j].directoryPath != NULL && strcmp(dirProcesado, classes[j].directoryPath) == 0) {
                free(classes[j].directoryPath);
                classes[j].directoryPath = NULL;
            }
        }
        free(idCluster);
    }

    for (int i = 0; i < numClasses; i++) {
        if (classes[i].parent != NULL) {
            fprintf(graphFile, "    \"%s\" -> \"%s\";\n", classes[i].name, classes[i].parent);
        }
        free(classes[i].name);
        free(classes[i].parent);
    }
    free(classes);
    fprintf(graphFile, "}\n");
    fclose(graphFile);

    printf("Gráfico generado correctamente.\n");
}

int match(const char *pattern, const char *str) {
    while (*pattern) {
        if (*pattern == '*') {
            return (match(pattern + 1, str) == 0 || (*str && match(pattern, str + 1) == 0)) ? 0 : NOMATCH;
        } else if (*pattern == '?' || *pattern == *str) {
            if (!*str) return NOMATCH;
            pattern++;
            str++;
        } else {
            return NOMATCH;
        }
    }
    return *str == '\0' ? 0 : NOMATCH;
}


int main(int argc, char *argv[])
{
    #ifdef _WIN32
    // Configura la consola de Windows para que muestre correctamente los caracteres UTF-8
    SetConsoleOutputCP(CP_UTF8);
    #endif

    const char *baseDirectory = ".";
    const char *pattern = "*.php"; // Patrón de fichero fijo
    const char *outputFileName = "list.txt";
    char *graphFileName = NULL;
    const char *excludePath = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-i") == 0) {
            if (i + 1 < argc) {
                baseDirectory = argv[++i];
            } else {
                fprintf(stderr, "Error: El flag '-i' requiere un valor (directorio base).\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                outputFileName = argv[++i];
            } else {
                fprintf(stderr, "Error: El flag '-o' requiere un valor (fichero de salida).\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-x") == 0) {
            if (i + 1 < argc) {
                excludePath = argv[++i];
            } else {
                fprintf(stderr, "Error: El flag '-x' requiere un valor (ruta a excluir).\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-g") == 0) {
            if (i + 1 < argc) {
                const char *rawGraphName = argv[++i];
                const char *extension = ".dot";
                size_t nameLen = strlen(rawGraphName);
                size_t extLen = strlen(extension);

                if (nameLen < extLen || strcmp(rawGraphName + nameLen - extLen, extension) != 0) {
                    graphFileName = malloc(nameLen + extLen + 1);
                    if (graphFileName) {
                        strcpy(graphFileName, rawGraphName);
                        strcat(graphFileName, extension);
                    }
                } else {
                    graphFileName = strdup(rawGraphName);
                }
            } else {
                fprintf(stderr, "Error: El flag '-g' requiere un valor (nombre del fichero .dot para el gráfico).\n");
                return 1;
            }
        } else {
            fprintf(stderr, "Uso: %s [-i directorio_base] [-o fichero_salida] [-g fichero_grafo.dot] [-x \"patron1;patron2;...\"]\n", argv[0]);
            fprintf(stderr, "Ejemplo: %s -i ./src -o clases.txt -x \"./src/vendor;./src/tests\"\n", argv[0]);
            return 1;
        }
    }

    FILE *outputFile = fopen(outputFileName, "w");
    if (outputFile == NULL)
    {
        perror("Error al abrir el fichero de salida");
        return 1;
    }

    printf("Buscando definiciones de clases en ficheros PHP dentro de '%s'...\n", pattern, baseDirectory);

    listDirectoryRecursively(baseDirectory, pattern, outputFile, excludePath);

    fclose(outputFile);
    printf("Listado generado correctamente en '%s'.\n", outputFileName);

    if (graphFileName != NULL) {
        generateGraph(outputFileName, graphFileName);
    }

    free(graphFileName);
    return 0;
}
