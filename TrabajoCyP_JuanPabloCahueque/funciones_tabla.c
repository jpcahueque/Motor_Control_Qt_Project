#include <stdlib.h>
#include <funciones_tabla.h>

void InitTabla(float t[],int n){
    //Se asigna el valor 0 a cada elemento en la tabla
    for (int i = 0; i<n; i++){
        t[i]=0;
    }
}

void DesplazarTabla(float t[],int n){
    //Se desplaza el elemento a su index mas significativo
    for(int i = n-1; i > 0; i--){
        t[i] = t[i-1];
    }
}

float ProductoEscalar(float v1[], float v2[], int numDatos){
    float sum = 0;
    //Se realiza el producto escalar de los dos vectores ingresados
    for(int i = 0; i < numDatos; i++){
        sum = sum + (v1[i]*v2[i]);
    }
    return sum;
}

void DesplazarTablaInt(int t[],int n){
    //Se desplaza el elemento a su index mas significativo
    for(int i = n-1; i > 0; i--){
        t[i] = t[i-1];
    }
}
