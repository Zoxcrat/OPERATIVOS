#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdlib.h>
#include <stdio.h>
#include <../../utils/src/utils/conexiones_cliente.h>
#include <../../utils/src/utils/conexiones_servidor.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/error.h>
#include <readline/readline.h>
#include <string.h>
#include <commons/temporal.h>
#include <commons/temporal.h>
#include <pthread.h>
#include <commons/bitarray.h>

t_log* logger;
t_log* logger_obligatorio;
t_config* config;

//t_contexto_ejecucion* contexto_de_ejecucion;
int filesystem_socket;

// Variables del config (Las pongo aca asi no estamos revoleando el cfg para todos lados)

char* IP_ESCUCHA;
char* PUERTO_ESCUCHA;
char* MOUNT_DIR;
int BLOCK_SIZE;
int BLOCK_COUNT;
int RETARDO_ACCESO_BLOQUE;
int RETARDO_RESPUESTA;
t_log_level LOG_LEVEL;

pthread_mutex_t fs_lock;  
FILE *blocks_file;             // Archivo bloques.dat donde se almacenan los datos
typedef struct {
    int size;                  // Tamaño del archivo en bytes
    int index_block;           // Número de bloque que contiene los punteros a los bloques de datos
} Metadata;

#define BITMAP_SIZE ((BLOCK_COUNT + 7) / 8)


// Buffer para el bitarray  ???
char bitmap_buffer[BITMAP_SIZE];

t_bitarray *bitarray;

// INIT
void leer_config();
void* procesar_conexion(void* arg);
void* procesar_peticion(void* arg);
void terminar_programa();

void inicializar_filesystem() {
    // Abrir o crear el archivo bitmap.dat
    char bitmap_path[256];
    snprintf(bitmap_path, sizeof(bitmap_path), "%s/bitmap.dat", MOUNT_DIR);
    
    FILE* bitmap_file = fopen(bitmap_path, "r+");
    if (bitmap_file == NULL) {
        // Si el archivo no existe, crear uno nuevo
        bitmap_file = fopen(bitmap_path, "w+");
        if (bitmap_file == NULL) {
            log_error(logger, "No se pudo crear bitmap.dat");
            exit(1);
        }
        // Inicializar el bitmap con 0s
        memset(bitmap_buffer, 0, BITMAP_SIZE);
        fwrite(bitmap_buffer, BITMAP_SIZE, 1, bitmap_file);
    } else {
        // Leer el contenido del bitmap existente
        fread(bitmap_buffer, BITMAP_SIZE, 1, bitmap_file);
    }
    fclose(bitmap_file);

    // Inicializar el bitarray
    init_bitarray();

    // Abrir o crear el archivo bloques.dat
    char blocks_path[256];
    snprintf(blocks_path, sizeof(blocks_path), "%s/bloques.dat", MOUNT_DIR);
    blocks_file = fopen(blocks_path, "r+");
    if (blocks_file == NULL) {
        blocks_file = fopen(blocks_path, "w+");
        if (blocks_file == NULL) {
            log_error(logger, "No se pudo crear bloques.dat");
            exit(1);
        }
    }
}

void init_bitarray() {
    bitarray = bitarray_create_with_mode(bitmap_buffer, BITMAP_SIZE, LSB_FIRST);
}

void destroy_bitarray() {
    bitarray_destroy(bitarray);
}

int buscar_bloque_libre() {
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (!bitarray_test_bit(bitarray, i)) {  // Si el bit está en 0 (libre)
            bitarray_set_bit(bitarray, i);     // Marcar el bloque como ocupado
            return i;                          
        }
    }
    return -1; 
}
// Verifica si hay suficientes bloques libres en el bitarray
int hay_bloques_libres(int num_blocks) {
    int free_blocks = 0;
    for (int i = 0; i < BLOCK_COUNT; i++) {
        if (!bitarray_test_bit(bitarray, i)) {  
            free_blocks++;
        }
        if (free_blocks >= num_blocks) {
            return 1;  // Hay espacio suficiente
        }
    }
    return 0;  // No hay espacio suficiente
}
void liberar_bloque(int block_num) {
    bitarray_clean_bit(bitarray, block_num);  

}
// Escribe datos en un bloque del archivo bloques.dat
void escribir_bloque(int block_num, const void *data) {
    fseek(blocks_file, block_num * BLOCK_SIZE, SEEK_SET);
    fwrite(data, BLOCK_SIZE, 1, blocks_file);
    fflush(blocks_file);  // Asegura que los datos se escriban
}

void crear_archivo_metadata(const char* nombre_archivo, int tamaño_archivo, int bloque_indice) {
    char path_metadata[512];  
    snprintf(path_metadata, sizeof(path_metadata), "%s/files/%s", MOUNT_DIR, nombre_archivo);

    FILE* metadata_file = fopen(path_metadata, "w");
    if (metadata_file == NULL) {
        log_error(logger, "No se pudo crear el archivo de metadata: %s", path_metadata);
        return;
    }

    fprintf(metadata_file, "SIZE=%d\n", tamaño_archivo);
    fprintf(metadata_file, "INDEX_BLOCK=%d\n", bloque_indice);

    fclose(metadata_file);
    log_info(logger, "Archivo de metadata creado: %s", path_metadata);
}

#endif