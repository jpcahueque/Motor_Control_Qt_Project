#ifndef FUNCIONES_TABLA_H
#define FUNCIONES_TABLA_H

struct tablasRZ {float* a; float* b; int M, N;};

//Funcion que asigna los valores de una tabla a 0
//Param
//t[] puntero a tabla
//n cantidad de elementos en tabla a asignar como 0
void InitTabla(float t[],int n);

//Funcion que desplaza los elementos de una tabla hacia la posicion mas significante
//Param
//t[] puntero a tabla
//n cantidad de elementos en tabla a desplazar
void DesplazarTabla(float t[],int n);

//Funcion que realiza el producto escalar de dos tablas
//Param
//a[] puntero a tabla 1
//b[] puntero a tabla 2
//numDatos cantidad de elementos en cada tabla a usar en producto escalar
float ProductoEscalar(float a[], float b[], int numDatos);

//Funcion que desplaza los elementos de una tabla hacia la posicion mas significante, elementos de tipo int
//Param
//t[] puntero a tabla
//n cantidad de elementos en tabla a desplazar
void DesplazarTablaInt(int t[],int n);

#endif // FUNCIONES_TABLA_H
