#ifndef FUNCIONES_CARACTERES_H
#define FUNCIONES_CARACTERES_H

//Descripcion:
//Funcion que compara dos cadenas de caracteres y devuelve un puntero hacia el resto de la cadena
//Param:
//*comando: puntero hacia cadena de caracteres ingresada por el usuario
//*busca: puntero hacia cadena de caracteres para buscar en comando ingresado
//Return:
//Cadena de caracteres que comienza en la posicion justo despues de haber encontrado la coincidencia
char* checkCmd(const char *comando, const char *busca);

//Descripcion:
//Funcion que salta los espacios y las comas en una cadena de caracteres
//Param:
//*pt: puntero hacia cadena de caracteres
char* SaltarEspaciosYComas(char* pt);

//Descripcion:
//Funcion que salta los espacios y las comas en una cadena de caracteres
//Param:
//*pt: puntero hacia cadena de caracteres
char* SaltarEspacios(char* pt);

//Descripcion:
//Funcion que cuenta los elementos numericos en una cadena de caracteres ingresada de la forma [# # #...]
//Param:
//*cadena: puntero hacia cadena de caracteres
//*nels: puntero de variable en donde se guarda el valor de interes
//Return:
// puntero hacia donde queda la cadena de caracteres evaluada
char* ContarElementosEnTabla(const char* cadena, int* n_els);

//Descripcion:
//Funcion que asigna los elementos numericos en una cadena de caracteres ingresada de la forma [# # #...] a un puntero del tipo float*
//Param:
//*cadena: puntero hacia cadena de caracteres
//*nels: puntero de variable en donde se guarda el valor de interes
//tabla: puntero a tabla en donde guardar los valores de interes
//Return:
// puntero hacia donde queda la cadena de caracteres evaluada
char* RellenarTabla(const char* cadena, float** tabla, int* n_els);


#endif // FUNCIONES_CARACTERES_H
