#include <../include/mem_instrucciones.h>
#include <stdio.h>

char *leer_archivo_pseudocodigo(char *archivo_a_leer)
{
    char *ruta_completa = malloc(sizeof(char) * 50);
    sprintf(ruta_completa, "%s/%s", PATH_INSTRUCCIONES, archivo_a_leer);

    FILE *archivo = fopen(ruta_completa, "r");
    return ruta_completa;
}
