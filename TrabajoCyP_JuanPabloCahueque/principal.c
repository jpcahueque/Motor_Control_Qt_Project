/*
//Trabajo de fin de curso Computadores y Programacion
//Por: Juan Pablo Cahueque Perez
//Fecha de presentacion: Enero de 2022
//Universidad de Oviedo
//
//principal.c
//Objetivo: Realizar el control de un simulador de motor mediante la recepcion de comandos varios
//Ampliaciones realizadas:
//1. Recepcion de comandos para manejo de nuevas tablas RZ
//2. Implementacion de libreria ncurses
//3. Obtencion de posicion mediante integracion de velocidad, y de velocidad mediante derivacion de posicion
//4. Efecto limpia parabrisas en secuancia de activacion de switches 0,1, y luego 2
//Ver Guia de Proyecto (TRABAJO CURSO 2020-21.pdf) para funcionalidad y manejo
*/

//Inclusion de librerias///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "SimulaMotor.h"
#include "funciones_caracteres.h"
#include "funciones_tabla.h"
#include "rutinacontrol.h"
#include "curses.h"
#include <string.h>
#include <time.h>

/*Definicion de constantes*****************************************************************************************************************/
//Constantes para control discreto de posicion/////////////////////////////////////////////////////////////////////////////////////////////
#define b0 0.13f   
#define b1 -0.11f
#define a0 1.0f
#define a1 -0.42f
#define M_POS 1
#define N_POS 1

#define B0_VEL 0.85f
#define B1_VEL -1.36f
#define B2_VEL 0.55f
#define A0_VEL 1.0f
#define A1_VEL -1.91f
#define A2_VEL 1.16f
#define A3_VEL -0.25f
#define M_VEL 2
#define N_VEL 3

//Constante de delay en ejecucion//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define Tm 100

//Constantes para modo de control//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MODO_CONTROL_POS 1 
#define MODO_CONTROL_VEL 2
#define MODO_CONTROL_OPENLOOP 3
#define KP_POS 0.05f
#define KP_VEL 0.005f

//Constantes para modo de referencia///////////////////////////////////////////////////////////////////////////////////////////////////////
#define MODO_REF_POTENCIOM 1 
#define MODO_REF_TECLADO 2

//Variables globales compartidas entre ISR y main//////////////////////////////////////////////////////////////////////////////////////////
char _cmd[60];
int _modo_control = MODO_CONTROL_POS, _modo_ref = MODO_REF_POTENCIOM;
float _valor_ref_teclado = 0;
float _tension_cte_teclado=2.0f;
float _ek[10];
float _uk[10];
float _yk[2];
float _posk[2];
float _velk[2];
float _kp;


int _conteo_interrupciones = 0;

//Variables para ampliaciones//////////////////////////////////////////////////////////////////////////////////////////////////////////////
float* _tabla; //Variables para recepcion de comando RZ =
int _n_els;

int _estado_SW[2];     //Definimos una variable de estado anterior de los pulsadores
int _ultimos_pulsadores[3];
int _limpia_activado = 0;

//Variable para uso de struct//////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct tablasRZ _rz;

//Variables para uso de ncurses
WINDOW *_ventana_cmd,*_ventana_estado,*_subwin;

/******************************************************************************************************************************************/

//Set de consola para pruebas//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SetConsoleSize(int rows_total,int cols_total,int rows_seen,int cols_seen)
{
 HANDLE hConsole=GetStdHandle(STD_OUTPUT_HANDLE);
 COORD const bufferSize={cols_total,rows_total};
 SMALL_RECT const windowRect={0,0,cols_seen-1,rows_seen-1};
 SMALL_RECT const minimal_window = { 0, 0, 1, 1 };
 SetConsoleWindowInfo(hConsole, TRUE, &minimal_window);
 SetConsoleScreenBufferSize(hConsole,bufferSize);
 SetConsoleWindowInfo(hConsole,TRUE,&windowRect);
}

//Funcion principal////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(){

    //Variable para uso de cadenas de caracteres
    char *pt_value;

    float tiempo_sleep = 0; //Variable para detencion de recepcion de comandos

    int flag_comando_valido = 0;

    _estado_SW[0] = _RB_Puerto1;   //Tomamos una muestra inicial de los pulsadores

    //Definicion de memoria dinamica para tablas a y b en control RZ
    _rz.M = 1;
    _rz.N = 1;
    _rz.a = (float*) malloc((_rz.N+1)*sizeof(float));
    _rz.b = (float*) malloc((_rz.M+1)*sizeof(float));

    //Inicializamos los arrays en cero
    InitTabla(_rz.a, _rz.N+1);   
    InitTabla(_rz.b, _rz.M+1);


    //Inicializamos valores de las tablas en general
    _rz.b[0] = b0;
    _rz.b[1] = b1;
    _rz.a[0] = a0;
    _rz.a[1] = a1;

    InitTabla(_posk,10);
    InitTabla(_velk,10);
    InitTabla(_ek, 10);  //Iniciamos los arrays en cero
    InitTabla(_uk, 10);

    _kp = KP_POS; //Variable de inicializacion de constante KP

    //Crear el simulador y lanzarlo////////////////////////////////////////////////////////////////////////////////////////////////////////
    CrearSimulador();   //Ejecutamos el simulador del motor
    LanzarSimulador();
    LCD_init();     //Iniciamos el LCD display

    //SetConsoleSize(60,120,24,40); //Cambiamos el tamaño de la consola para pruebas

    //Inicializacion de ventanas con ncurses//////////////////////////////////////////////////////////////////////////////////////////////
    initscr(); /* Inicializa */
    cbreak(); /* Evita control-break */
    noecho();  /* No hace eco automático de los caracteres pulsados, para que no aparezcan donde esté el cursor */
    refresh(); /* Actualiza la pantalla */
    start_color(); /* Permite colores */
    init_pair(1,COLOR_YELLOW,COLOR_BLUE); /* Inicializa parejas de colores de carácter con fondo (hasta 16 parejas) */
    init_pair(3,COLOR_BLACK,COLOR_WHITE);
    init_pair(2,COLOR_BLACK,COLOR_CYAN);


    //Creacion de la ventana de comandos///////////////////////////////////////////////////////////////////////////////////////////////////
    _ventana_cmd = newwin(10,80,1,1);
    wattron(_ventana_cmd,COLOR_PAIR(1));
    wbkgd(_ventana_cmd,COLOR_PAIR(1) | ' ');
    wclear(_ventana_cmd);
    box(_ventana_cmd,ACS_VLINE,ACS_HLINE);
    wmove(_ventana_cmd,0,10);
    wprintw(_ventana_cmd,"COMANDOS");
    wmove(_ventana_cmd,8,3);
    wprintw(_ventana_cmd,">>");
    wrefresh(_ventana_cmd);

    //Creacion de la ventana de estado/////////////////////////////////////////////////////////////////////////////////////////////////////
    _ventana_estado = newwin(6,80,11,1);
    wattron(_ventana_estado,COLOR_PAIR(2));
    wbkgd(_ventana_estado,COLOR_PAIR(2) | ' ');
    wclear(_ventana_estado);
    box(_ventana_estado,ACS_VLINE,ACS_HLINE);
    wmove(_ventana_estado,0,10);
    wprintw(_ventana_estado,"ESTADO");
    wmove(_ventana_estado,1,1);
    wprintw(_ventana_estado,"Modo: ");
    wattron(_ventana_estado,COLOR_PAIR(3));
    wprintw(_ventana_estado,"CONTROL POS (deg)  ");
    wattron(_ventana_estado,COLOR_PAIR(2));
    wprintw(_ventana_estado," vel=dpos/dt: ");
    wattron(_ventana_estado,COLOR_PAIR(3));
    wprintw(_ventana_estado,"N");
    wattron(_ventana_estado,COLOR_PAIR(2));
    wprintw(_ventana_estado," pos=I vel.dt: ");
    wattron(_ventana_estado,COLOR_PAIR(3));
    wprintw(_ventana_estado,"N");

    wmove(_ventana_estado,2,1);
    wattron(_ventana_estado,COLOR_PAIR(2));
    wprintw(_ventana_estado,"Tipo: ");
    wattron(_ventana_estado,COLOR_PAIR(3));
    wprintw(_ventana_estado,"RZ=[0.130,-0.110]/[1.000,-0.430]");

    wmove(_ventana_estado,3,1);
    wattron(_ventana_estado,COLOR_PAIR(2));
    wprintw(_ventana_estado,"Ref:  ");
    wattron(_ventana_estado,COLOR_PAIR(3));
    wprintw(_ventana_estado,"   0.0deg (POT)");

    wmove(_ventana_estado,4,1);
    wattron(_ventana_estado,COLOR_PAIR(2));
    wprintw(_ventana_estado,"Paso 180: ");
    wattron(_ventana_estado,COLOR_PAIR(3));
    wprintw(_ventana_estado,"NO GESTIONAR");
    wattron(_ventana_estado,COLOR_PAIR(2));
    wprintw(_ventana_estado," Camino corto: ");
    wattron(_ventana_estado,COLOR_PAIR(3));
    wprintw(_ventana_estado,"NO GESTIONAR");

    wrefresh(_ventana_estado);

    int fila = 0; //Definimos una variable para posicionarnos en pantalla

    time_t ahora_sec; //Variables para impresion de timestamp como log de comandos
    struct tm* ahora;

    //Asignamos la rutina de temporizador (ultimo comando antes del bucle while)///////////////////////////////////////////////////////////
    EstablecerISRTemporizador(0,Tm,ISR_TimerControl); 

    //Bucle infinito de ejecucion//////////////////////////////////////////////////////////////////////////////////////////////////////////
    while(1){

        //Requerimos el comando y lo recibimos/////////////////////////////////////////////////////////////////////////////////////////////
        wmove(_ventana_cmd,8,6);
        echo();
        wgetnstr(_ventana_cmd,_cmd,60);
        fila++;
        noecho();
        wmove(_ventana_cmd,8,6);
        wclrtoeol(_ventana_cmd);
        flag_comando_valido = 0;

        //Chequeamos si el comando comienza con POS////////////////////////////////////////////////////////////////////////////////////////
        pt_value = checkCmd(_cmd,"POS");
        if (pt_value != NULL){
            pt_value = SaltarEspacios(pt_value); //Saltamos los espacios en blanco
            if (*pt_value == '='){  //Verificamos que haya un '='
                pt_value ++;
                pt_value = SaltarEspacios(pt_value); //Saltamos los espacios en blanco
                if (strncmp(pt_value,"POT",3) == 0){ //Verificamos el siguiente comando
                    _modo_ref = MODO_REF_POTENCIOM;  //Asignamos el modo de referencia
                }
                else{
                    _modo_ref = MODO_REF_TECLADO;
                    _valor_ref_teclado = atof(pt_value); //Convertimos a float, en caso de que se reciba un numero
                }
                _modo_control = MODO_CONTROL_POS; //Asignamos el modo control correspondiente
                free(_rz.b); //liberamos el espacio
                free(_rz.a);
                _rz.M = M_POS;
                _rz.N = N_POS;
                _rz.a = (float*) malloc((_rz.N+1)*sizeof(float));
                _rz.b = (float*) malloc((_rz.M+1)*sizeof(float));
                _rz.b[0] = b0;  //Definimos las constantes para control de posicion inicialmente
                _rz.b[1] = b1;
                _rz.a[1] = a0;
                _rz.a[1] = a1;
                _kp = KP_POS; //Definimos la constante proporcional
                flag_comando_valido = 1;
            }
        }

        //Chequeamos si el comando comienza con VEL////////////////////////////////////////////////////////////////////////////////////////
        pt_value = checkCmd(_cmd,"VEL");
        if (pt_value != NULL){
            pt_value = SaltarEspacios(pt_value); //Saltamos los espacios en blanco
            if (*pt_value == '='){
                pt_value ++;
                pt_value = SaltarEspacios(pt_value); //Saltamos los espacios en blanco
                if (strncmp(pt_value,"POT",3) == 0){
                    _modo_ref = MODO_REF_POTENCIOM;
                }
                else{
                    _modo_ref = MODO_REF_TECLADO;
                    _valor_ref_teclado = atof(pt_value);
                }
                _modo_control = MODO_CONTROL_VEL;
                InitTabla(_uk,10);
                free(_rz.b);
                free(_rz.a);
                _rz.M = M_VEL;
                _rz.N = N_VEL;
                _rz.a = (float*) malloc((_rz.N+1)*sizeof(float));
                _rz.b = (float*) malloc((_rz.M+1)*sizeof(float));
                _rz.b[0] = B0_VEL;    //Iniciamos las constantes para Rz de velocidad
                _rz.b[1] = B1_VEL;
                _rz.b[2] = B2_VEL;
                _rz.a[0] = A0_VEL;
                _rz.a[1] = A1_VEL;
                _rz.a[2] = A2_VEL;
                _rz.a[3] = A3_VEL;
                _kp = KP_VEL;
                flag_comando_valido = 1;
            }
        }

        //Chequeamos si el comando comienza con TENSION////////////////////////////////////////////////////////////////////////////////////
        pt_value = checkCmd(_cmd,"TENSION");
        if (pt_value != NULL){
            pt_value = SaltarEspacios(pt_value); //Saltamos los espacios en blanco
            if (*pt_value == '='){
                pt_value ++;
                pt_value = SaltarEspacios(pt_value); //Saltamos los espacios en blanco
                _tension_cte_teclado = atof(pt_value);
                flag_comando_valido = 1;
            }
        }

        //Chequeamos si el comando comienza con OPEN LOOP//////////////////////////////////////////////////////////////////////////////////
        pt_value = checkCmd(_cmd,"OPEN LOOP");
        if (pt_value != NULL){
            pt_value = SaltarEspacios(pt_value); //Saltamos los espacios en blanco
            if (*pt_value == '='){
                pt_value ++;
                pt_value = SaltarEspacios(pt_value); //Saltamos los espacios en blanco
                _modo_control = MODO_CONTROL_OPENLOOP;
                _tension_cte_teclado = atof(pt_value);
                flag_comando_valido = 1;
            }
        }

        //Chequeamos si el comando comienza con KP/////////////////////////////////////////////////////////////////////////////////////////
        pt_value = checkCmd(_cmd,"KP");
        if (pt_value != NULL){
            pt_value = SaltarEspacios(pt_value); //Saltamos los espacios en blanco
            if (*pt_value == '='){
                pt_value ++;
                pt_value = SaltarEspacios(pt_value); //Saltamos los espacios en blanco
                _kp = atof(pt_value);
                flag_comando_valido = 1;
            }
        }

        //Chequeamos si el comando comienza con SLEEP
        pt_value = checkCmd(_cmd,"SLEEP");
        if (pt_value != NULL){
            pt_value = SaltarEspacios(pt_value); //Saltamos los espacios en blanco
            if (*pt_value == '='){
                pt_value ++;
                pt_value = SaltarEspacios(pt_value); //Saltamos los espacios en blanco
                tiempo_sleep = atof(pt_value);
                _RB_Puerto0 = _RB_Puerto0 | (1); //encendemos el ultimo bit para indicar que no se reciben comandos
                Sleep(tiempo_sleep);    //Se aplica un Sleep para esperar y no recibir comandos
                _RB_Puerto0 = _RB_Puerto0 & ~(1<<0); //Apagamos el ultimo bit para indicar que se reciben comandos
                flag_comando_valido = 1;
            }

        }

        //Chequeamos si el comando comienza con RZ/////////////////////////////////////////////////////////////////////////////////////////
        pt_value = checkCmd(_cmd,"RZ");
        if(pt_value != NULL){
            pt_value = SaltarEspacios(pt_value);
            if (*pt_value == '='){
                pt_value++;
                pt_value = SaltarEspacios(pt_value);
                pt_value = RellenarTabla(pt_value,&_tabla,&_n_els);
                if (pt_value != NULL){
                    free(_rz.b);
                    _rz.b = _tabla;
                    _rz.M = _n_els - 1;
                }
                pt_value = SaltarEspacios(pt_value);
                pt_value = checkCmd(pt_value,"/");
                if (pt_value != NULL){
                    pt_value = SaltarEspacios(pt_value);
                    pt_value = RellenarTabla(pt_value,&_tabla,&_n_els);
                    if (pt_value != NULL){
                        free(_rz.a);
                        _rz.a = _tabla;
                        _rz.N = _n_els - 1;
                    }
                }
                flag_comando_valido = 1;
            }

        }

        //Obtenemos el timestamp///////////////////////////////////////////////////////////////////////////////////////////////////////////
        ahora_sec=time(NULL);
        ahora=localtime(&ahora_sec);


        //Si hemos impreso ya siete comandos///////////////////////////////////////////////////////////////////////////////////////////////
        if(fila==7){
            for (int i = 1; i<8; i++){
                wmove(_ventana_cmd,i,1);
                wclrtoeol(_ventana_cmd); //Borramos las lineas 
                box(_ventana_cmd,ACS_VLINE,ACS_HLINE);
                wrefresh(_ventana_cmd);
            }
            fila = 1; //Posicionamos la impresion de nuevo arriba
        }

        //Si el comando no es valido
        if(flag_comando_valido == 0){
            wmove(_ventana_cmd,fila,1);
            wprintw(_ventana_cmd,"->%02d/%02d/%02d %02d:%02d:%02d Error. Comando no reconocido.%s",ahora->tm_mday,ahora->tm_mon+1,ahora->tm_year-100,ahora->tm_hour,ahora->tm_min,ahora->tm_sec,_cmd);
        }
        //Si el comando es valido
        else{
            wmove(_ventana_cmd,fila,1);
            wprintw(_ventana_cmd,"->%02d/%02d/%02d %02d:%02d:%02d Correcto:%s",ahora->tm_mday,ahora->tm_mon+1,ahora->tm_year-100,ahora->tm_hour,ahora->tm_min,ahora->tm_sec,_cmd);
        }
    }

}
