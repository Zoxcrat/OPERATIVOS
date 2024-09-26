#include <../include/mem_instrucciones.h>
#include <stdio.h>

t_list *leer_archivo_pseudocodigo(char *archivo_a_leer)
{
    char *ruta_completa = malloc(sizeof(char) * 50);
    sprintf(ruta_completa, "%s/%s", PATH_INSTRUCCIONES, archivo_a_leer);

    FILE *archivo = fopen(ruta_completa, "r");

    if (NULL == archivo)
    {
        log_error(logger, "No se pudo abrir el archivo \n");
        exit(0);
    }
    t_list *lista_instrucciones = list_create();
    char lineaActual[100];
    while (fgets(lineaActual, sizeof(lineaActual), archivo) != NULL)
    {
        // printf("Instruccion: %s", lineaActual);
        list_add(lista_instrucciones, lineaActual);
    }

    fclose(archivo);
    return lista_instrucciones;
}
