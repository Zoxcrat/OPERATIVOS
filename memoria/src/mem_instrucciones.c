#include <../include/mem_instrucciones.h>
#include <stdio.h>

void leer_archivo_pseudocodigo(char *archivo_a_leer, t_list *lista_instrucciones)
{
    char *ruta_completa = malloc(sizeof(char) * 50);
    sprintf(ruta_completa, "%s/%s", PATH_INSTRUCCIONES, archivo_a_leer);

    FILE *archivo = fopen(ruta_completa, "r");

    if (NULL == archivo)
    {
        log_error(logger, "No se pudo abrir el archivo \n");
        exit(0);
    }
    char lineaActual[100];
    while (fgets(lineaActual, sizeof(lineaActual), archivo) != NULL)
    {
        char *nuevaInst = strdup(lineaActual);
        nuevaInst = strtok(nuevaInst, "\n");
        printf("Instruccion: %s\n", nuevaInst);
        list_add(lista_instrucciones, nuevaInst);
    }

    fclose(archivo);
    return;
}
