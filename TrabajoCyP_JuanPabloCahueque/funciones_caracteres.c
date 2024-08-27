 #include <stdlib.h>
#include <string.h>

char* checkCmd(const char *comando, const char *busca){
    //Compara el comando ingresado con la cadena en busca
    //Si no encuentra una coincidencia devuelve NULL para poder manejar el error al retorno
    if (strncmp(comando,busca,strlen(busca)) != 0){
        return NULL;
    }
    //Si encuentra coincidencia devolvemos el puntero en la nueva posicion para seguir usando el comando
    char *pt = comando+strlen(busca);
    return pt;

}

char* SaltarEspaciosYComas(char* pt){
    while (*pt == ' ' || *pt == ','){ //Nos saltamos todos los espacios en blanco y las comas
        pt++;
    }
    return pt;
}

char* SaltarEspacios(char* pt){
    while(*pt == ' '){    //Nos saltamos todos los espacios en blanco
        pt++;
    }
    return pt;
}

const char* ContarElementosEnTabla(const char* cadena, int* n_els){
    char *pt, *ptfin;
    int cuenta = 0;
    float valor = 1.0;
    pt = checkCmd(cadena,"["); //Chequeamos si el comando esta bien escrito
    if (pt != NULL){
        pt = SaltarEspacios(pt); //Nos saltamos todos los espacios en blanco
        while (valor != 0.0){ //Hasta que no haya un valor numerico
            valor = strtod(pt,&ptfin); //Verificamos si hay un valor numerico
            if (valor != 0.0){
                cuenta++; //Sumamos a la cuenta de elementos
                pt = SaltarEspaciosYComas(ptfin); //Nos saltamos los espacios en blanco
            }
        } 
        pt  = checkCmd(pt,"]"); //Chequeamos si el comando esta bien escrito
        if (pt == NULL){
            return NULL;
        }
        *n_els = cuenta; //Devolvemos la cuenta en el puntero
    }
    else{
        return NULL;
    }
    return pt;
}

const char* RellenarTabla(const char* cadena, float** tabla, int* n_els){
    int n, i;
    float *t;
    char *pt, *ptfin;
    float valor = 1.0;

    ptfin = ContarElementosEnTabla(cadena,&n); //Contamos los elementos en la tabla

    if (ptfin == NULL){
        return NULL;
    }

    t = (float*) malloc(n*sizeof(float)); //Asignamos memoria dinamica a la tabla nueva

    i = 0;
    pt = checkCmd(cadena,"["); //Chequeamos si el comando esta bien escrito
    if (pt != NULL){
        pt = SaltarEspacios(pt);
        while (valor != 0.0){ //Verificamos que haya un valor numerico
            valor = strtod(pt,&ptfin);
            if (valor != 0.0){
                pt = SaltarEspaciosYComas(ptfin);
                t[i] = valor; //Agregamos el valor a la tabla
                i++;
            }
        }
        pt  = checkCmd(pt,"]"); //Chequeamos si el comando esta bien escrito
        if (pt == NULL){
            free(t);
            return NULL;
        }
    }
    else{
        free(t); //Liberamos la memoria dinamica
        return NULL;
    }
    *tabla = t; //Devolvemos la tabla en el puntero
    *n_els = n;
    return pt;
}
