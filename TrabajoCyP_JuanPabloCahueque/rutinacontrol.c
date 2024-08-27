/*
//Trabajo de fin de curso Computadores y Programacion
//Por: Juan Pablo Cahueque Perez
//Fecha de presentacion: Enero de 2022
//Universidad de Oviedo
//
//rutinacontrol.c
//Objetivo: Manejar la funcion de interrupcion para el control del motor
*/

//Inclusion de librerias
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "SimulaMotor.h"
#include "funciones_caracteres.h"
//#include "funciones_sensoresMotor.h"
#include "funciones_tabla.h"
#include "curses.h"
#include <string.h>

/*Definicion de constantes*****************************************************************************************************************/
#define umax_ad_v 5.0f //Maximo de voltios de entrada en conversor AD
#define max_ad 1023.0f //Maximo valor de salida en conversor AD
#define usens_min_v -10.0f //Maximo valor de salida en usens2
#define usens_max_v 10.0f //Maximo valor de salida en usens2
#define pos_ref_max_deg 180.0f //Maximo valor de salida en pos_ref
#define pos_ref_min_deg -180.0f //Minimo valor de salida en pos_ref
#define vel_ref_max_rpm 100.0f //Maximo valor de salida en vel_ref
#define vel_ref_min_rpm -100.0f //Minimo valor de salida en vel_ref
#define u_mot_max_v 10.0f //Maximo valor de salida en umot
#define u_mot_min_v -10.0f //Minimo valor de salida en umot

//Constantes para control discreto de posicion/////////////////////////////////////////////////////////////////////////////////////////////
#define b0 0.13f   //Constantes para control discreto de posicion
#define b1 -0.11f
#define a1 -0.42f
#define M_POS 1
#define N_POS 1

#define B0_VEL 0.85f
#define B1_VEL -1.36f
#define B2_VEL 0.55f
#define A1_VEL -1.91f
#define A2_VEL 1.16f
#define A3_VEL -0.25f
#define M_VEL 2
#define N_VEL 3

//Constante de delay en ejecucion//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define Tm 100  //Constante de delay en ejecucion

//Constantes para modo de control//////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MODO_CONTROL_POS 1 //Constantes para modo de control
#define MODO_CONTROL_VEL 2
#define MODO_CONTROL_OPENLOOP 3
#define KP_POS 0.05f
#define KP_VEL 0.005f

//Constantes para modo de referencia///////////////////////////////////////////////////////////////////////////////////////////////////////
#define MODO_REF_POTENCIOM 1 //Constantes para modo de referencia
#define MODO_REF_TECLADO 2

//Variables globales compartidas entre ISR y main//////////////////////////////////////////////////////////////////////////////////////////
extern char _cmd[40];
extern int _modo_control, _modo_ref;
extern float _valor_ref_teclado;
extern float _tension_cte_teclado;
extern float _ek[10];
extern float _uk[10];
extern float _yk[2];
extern float _posk[2];
extern float _velk[2];
extern float _kp;

extern int _conteo_interrupciones;

//Variables para ampliaciones//////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern float* _tabla;
extern int _n_els;

//variables para ncurses
extern WINDOW *_ventana_cmd,*_ventana_estado,*_subwin;

extern int _estado_SW[2];     //Definimos una variable de estado anterior de los pulsadores
extern int _ultimos_pulsadores[3];
extern int _limpia_activado;

//Variable para uso de struct//////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern struct tablasRZ _rz;


float RelacionLineal(float I, float x1, float x2, float y1, float y2){
    float m = 0;
    //Se calcula la pendiente según los datos ingresados
    m = ((y2-y1)/(x2-x1));
    //Se usa la relación lineal para hallar el valor correspondiente al valor de entrada
    return m*(I-x1) + y1;
}

float LeerRef_deg(){
    int ad2;
    float uad2, usens2, pos_ref;
    //Se lee la entrada analogica corespondiente al sensor de potenciometro
    ad2 = LeerEntradaAnalogica(2);
    //Se realiza la conversión según los valores de conversión AD
    uad2 = RelacionLineal(ad2,0,max_ad,0,umax_ad_v);
    //Según el valor de conversión AD, se encuentra el voltaje correspondiente en el potenciometro
    usens2 = RelacionLineal(uad2,0,umax_ad_v,0,usens_max_v);
    //Según el voltaje en potenciómetro, se encuentra la posición del mismo
    pos_ref = RelacionLineal(usens2,0,usens_max_v,pos_ref_min_deg,pos_ref_max_deg);
    return pos_ref;
}

float LeerPosMotor_deg(){
    int ad1;
    float uad1, usens1, pos_mot;
    //Se lee la entrada analogica corespondiente al sensor de posición del motor
    ad1 = LeerEntradaAnalogica(1);
    //Se realizan la conversión según los valores de conversión AD
    uad1 = RelacionLineal(ad1,0,max_ad,0,umax_ad_v);
    //Según el valor de conversión AD, se encuentra el voltaje correspondiente en el sensor del motor
    usens1 = RelacionLineal(uad1,0,umax_ad_v,0,usens_max_v);
    //Según el voltaje en sensor, se encuentra la posición del motor
    pos_mot = RelacionLineal(usens1,0,usens_max_v,pos_ref_min_deg,pos_ref_max_deg);
    return pos_mot;
}

float LeerVelMotor_rpm(){
    int ad0;
    float uad0, usens0, vel_mot;
    //Se lee la entrada analogica corespondiente al sensor de velocidad del motor
    ad0 = LeerEntradaAnalogica(0);
    //Se realizan la conversión según los valores de conversión AD
    uad0 = RelacionLineal(ad0,0,max_ad,0,umax_ad_v);
    //Según el valor de conversión AD, se encuentra el voltaje correspondiente en el sensor de velocidad del motor
    usens0 = RelacionLineal(uad0,0,umax_ad_v,usens_min_v,usens_max_v);
    //Según el voltaje en sensor, se encuentra la velocidad del motor en RPM
    vel_mot = RelacionLineal(usens0,usens_min_v,usens_max_v,vel_ref_min_rpm,vel_ref_max_rpm);
    return vel_mot;
}

void ActivarTensionMotor(float u_motor_v){
    float duty = 0;
    //Se halla el valor corresponciente en DUTY CYCLE para el valor de tensión ingresado
    duty = RelacionLineal(u_motor_v,u_mot_min_v,u_mot_max_v,0,100);
    EscribirSalidaPwm((int)(duty*10),1000);
}

float ControlTodoNada(float errk, float tension){
    float u_motor_v = 0;
    //Si el error es mayor a 0, aplicar una tensión positiva constante
    if (errk>0){
        u_motor_v = tension;
    }
    //Si el error es mayor a 0, aplicar una tensión negativa constante
    else if (errk<0){
        u_motor_v = -tension;
    }
    //Si el error es 0, no aplicar tensión
    else if (errk==0){
        u_motor_v = 0;
    }

    return u_motor_v;
}

float ControlProporcional(float errk, float kp){
    float u_motor_v = 0;
    //Aplicamos la expresión para control P según el error ingresado
    u_motor_v = kp * errk;
    return u_motor_v;
}

float LeerPot_rpm(){
    //Leemos la posición del potenciómetro en grados
    float pos_ref = LeerRef_deg();
    //Se encuentra su equivalente en RPM
    float rpm = RelacionLineal(pos_ref, pos_ref_min_deg,pos_ref_max_deg,-60,60);
    return rpm;
}
float ControlPI(float errk, float uk[], float kp){
    float u_motor_v = 0;
    //Aplicamos la expresión para control PI según el error ingresado
    u_motor_v = uk[1] + kp * errk;
    return u_motor_v;
}

float ControlTodoNadaVel(float errk, float uk[], float tension){
    float u_motor_v = 0;
    //Si el error es mayor a 0, sumar una tensión positiva constante
    if (errk>0){
        u_motor_v = uk[1] + tension;
    }
    //Si el error es menor a 0, sumar una tensión negativa constante
    else if (errk<0){
        u_motor_v = uk[1] - tension;
    }
    //Si el error es 0, no aplicar tensión
    else if (errk==0){
        u_motor_v = 0;
    }
    return u_motor_v;
}
//Funcion de manejo de Control de motor////////////////////////////////////////////////////////////////////////////////////////////////////
void ISR_TimerControl(){
    _conteo_interrupciones++;

    //Variables de Control tomadas del simulador
    float refk=0;
    float vel_motor_rpm;

    //Ampliacion para modo parabrisas//////////////////////////////////////////////////////////////////////////////////////////////////////
    _estado_SW[0] = _RB_Puerto1; //Asignamos el nuevo estado de los pulsadores
    if (_estado_SW[1] != _estado_SW[0]){ //Si detectamos un cambio
        DesplazarTablaInt(_ultimos_pulsadores,3); //Desplazamos la tabla de ultimo pulsador y asignamos valores de interes
        if((_estado_SW[0]&(1<<0))&&((_estado_SW[1] &(1<<0))==0)){
            _ultimos_pulsadores[0]=0;
        }
        else if((_estado_SW[0]&(1<<1))&&((_estado_SW[1] &(1<<1))==0)){
            _ultimos_pulsadores[0]=1;
        }
        else if((_estado_SW[0]&(1<<2))&&((_estado_SW[1] &(1<<2))==0)){
            _ultimos_pulsadores[0]=2;
        }
        else{
            _ultimos_pulsadores[0]=-1;
        }
        if (_ultimos_pulsadores[0]==2 && _ultimos_pulsadores[1]==1 && _ultimos_pulsadores[2]==0){ //Si se cumple la condicion de secuencia
            _limpia_activado = 1; //Se valida para entrar en modo parabrisas
            _modo_ref = MODO_REF_TECLADO;
            _valor_ref_teclado = 70;
        }
    }

    if(_limpia_activado){ //Activamos el movimiento hasta que se pulse un switch
        if(_ultimos_pulsadores[0]==-1){
            _limpia_activado = 0;
        }

        if(_posk[0]>69.5){
            _modo_ref = MODO_REF_TECLADO;
            _valor_ref_teclado = -70;
        }
        if(_posk[0]<-69.5){
            _modo_ref = MODO_REF_TECLADO;
            _valor_ref_teclado = 70;
        }
    }

    //Verificamos que pulsador de parada este en 0 o en 1//////////////////////////////////////////////////////////////////////////////////
    if (_RB_Puerto1 & (1<<0)&&_limpia_activado==0){ //Si el bit 0 es 1 y no estamos en modo parabrisas
        _uk[0] = 0;
        wmove(_ventana_estado,1,7);
        wattron(_ventana_estado,COLOR_PAIR(3));
        wprintw(_ventana_estado,"Motor Parado       ");
        wmove(_ventana_estado,2,7);
        wattron(_ventana_estado,COLOR_PAIR(2));
        wclrtoeol(_ventana_estado);
    }
    else{ //Si el bit 0 es 0
        //Desplazamiento de tablas/////////////////////////////////////////////////////////////////////////////////////////////////////////
        DesplazarTabla(_ek, 10);    //Movemos los arrays en una posicion para insertar el siguiente valor de simulacion
        DesplazarTabla(_uk, 10);
        DesplazarTabla(_posk, 2);
        DesplazarTabla(_velk, 2);
        DesplazarTabla(_yk,2);

        //Obtencion de referencias y posicion actual///////////////////////////////////////////////////////////////////////////////////////
        vel_motor_rpm = LeerVelMotor_rpm(); //Leemos el sensor de velocidad del motor
        _velk[0] = vel_motor_rpm;
        _posk[0] = LeerPosMotor_deg();

        //Ampliacion para derivar velocidad////////////////////////////////////////////////////////////////////////////////////////////////
        if ((_RB_Puerto1 & (1<<7))){ //Si esta pulsado el switch 7 obtiene la derivada de la posicion como la velocidad
            wmove(_ventana_estado,1,40);
            wattron(_ventana_estado,COLOR_PAIR(3));
            wprintw(_ventana_estado,"Y");
            if (_posk[0] - _posk[1] < -180){ //Se valida si el motor ha pasado la referencia entre -180 y 180 grados
                _posk[1] = _posk[1] - 360;
            }
            else if (_posk[0] - _posk[1] > 180){
                _posk[1] = _posk[1] + 360;
            }
            _velk[0] = ((_posk[0] - _posk[1])/Tm) *(1000*60/360); //Derivamos la posicion y convertimos a rpm
        }
        else{ //Si no esta pulsado el boton 7, se lee la velocidad del sensor
            wmove(_ventana_estado,1,40);
            wattron(_ventana_estado,COLOR_PAIR(3));
            wprintw(_ventana_estado,"N");
        }
        //Ampliacion para integrar posicion////////////////////////////////////////////////////////////////////////////////////////////////
        if ((_RB_Puerto1 & (1<<6))){//Si esta pulsado el switch 6 obtiene la posicion como la integral de la velocidad
            wmove(_ventana_estado,1,56);
            wattron(_ventana_estado,COLOR_PAIR(3));
            wprintw(_ventana_estado,"Y");
            _posk[0] = _posk[1] + ((_velk[0] + _velk[1])/2*Tm) / (1000/(360/60));
        }
        else{
            wmove(_ventana_estado,1,56);
            wattron(_ventana_estado,COLOR_PAIR(3));
            wprintw(_ventana_estado,"N");
        }

        //Si estamos realizando control de posicion, leer los valores para posicion//////////////////////////////////////////////////////
        if(_modo_control == MODO_CONTROL_POS){
            refk = LeerRef_deg();   //Por default leemos el POT para posicion para control
            _yk[0] = _posk[0]; //Por default leemos el sensor de posicion del motor
            wmove(_ventana_estado,1,7);
            wattron(_ventana_estado,COLOR_PAIR(3));
            wprintw(_ventana_estado,"CONTROL POS (deg)  ");
        }
        //Si estamos realizando control de velocidad, leer los valores para velocidad//////////////////////////////////////////////////////
        else if (_modo_control == MODO_CONTROL_VEL){ //Si estamos realizando el control de velocidad
            wmove(_ventana_estado,1,7);
            wattron(_ventana_estado,COLOR_PAIR(3));
            wprintw(_ventana_estado,"CONTROL VEL (rpm)  ");
            wmove(_ventana_estado,4,1);
            wattron(_ventana_estado,COLOR_PAIR(2));
            wclrtoeol(_ventana_estado);
            refk = LeerPot_rpm();   //Leemos el POT con referencias para velocidad
            //Leemos el sensor de velocidad en el estado actual
            _yk[0] = _velk[0];
        }


        //Verificamos cual es la referencia requerida//////////////////////////////////////////////////////////////////////////////////////
        if (_modo_ref == MODO_REF_TECLADO){
            refk = _valor_ref_teclado;
            if (_modo_control == MODO_CONTROL_VEL){
                wmove(_ventana_estado,3,7);
                wattron(_ventana_estado,COLOR_PAIR(3));
                wprintw(_ventana_estado,"   %2.1f rpm (TECLADO) ",refk);
            }
            else if (_modo_control == MODO_CONTROL_POS){
                wmove(_ventana_estado,3,7);
                wattron(_ventana_estado,COLOR_PAIR(3));
                wprintw(_ventana_estado,"   %2.1f deg (TECLADO) ",refk);
            }

        }
        else if (_modo_ref == MODO_REF_POTENCIOM){
            if (_modo_control == MODO_CONTROL_VEL){
                wmove(_ventana_estado,3,7);
                wattron(_ventana_estado,COLOR_PAIR(3));
                wprintw(_ventana_estado,"   %2.1f rpm (POT)       ",refk);
            }
            else if (_modo_control == MODO_CONTROL_POS){
                wmove(_ventana_estado,3,7);
                wattron(_ventana_estado,COLOR_PAIR(3));
                wprintw(_ventana_estado,"  %2.1f deg (POT)       ",refk);
            }
        }

        //Realizamos la correcion para no entrar en loop en los limites del potenciometro//////////////////////////////////////////////////
        if ((_RB_Puerto1 & (1<<1)) && _modo_control == MODO_CONTROL_POS){ //Si el bit 1 es 1 y modo control de posicion
            wmove(_ventana_estado,4,1);
            wattron(_ventana_estado,COLOR_PAIR(2));
            wprintw(_ventana_estado,"Paso 180: ");
            wattron(_ventana_estado,COLOR_PAIR(3));
            wprintw(_ventana_estado,"   GESTIONAR");
            if (_yk[0] - _yk[1] < -180){
                _yk[0] = _yk[0] + 360;
            }
            else if (_yk[0] - _yk[1] > 180){
                _yk[0] = _yk[0] - 360;
            }
        }
        else if (_modo_control == MODO_CONTROL_POS){
            wmove(_ventana_estado,4,1);
            wattron(_ventana_estado,COLOR_PAIR(2));
            wprintw(_ventana_estado,"Paso 180: ");
            wattron(_ventana_estado,COLOR_PAIR(3));
            wprintw(_ventana_estado,"NO GESTIONAR");
        }

        //Calculamos el error actual///////////////////////////////////////////////////////////////////////////////////////////////////////
        _ek[0] = refk-_yk[0];

        //Realizamos la correcion para seguir la trayectoria mas corta en correcion de error de posicion///////////////////////////////////
        if ((_RB_Puerto1 & (1<<2)) && _modo_control == MODO_CONTROL_POS){ //Si el bit 2 es 1 y modo control de posicion
            wattron(_ventana_estado,COLOR_PAIR(2));
            wprintw(_ventana_estado," Camino corto: ");
            wattron(_ventana_estado,COLOR_PAIR(3));
            wprintw(_ventana_estado,"   GESTIONAR");
            if (_ek[0] > 180){
                _ek[0] = _ek[0] - 360;
            }
            else if (_ek[0] < - 180){
                _ek[0] = _ek[0] + 360;
            }
        }
        else if (_modo_control == MODO_CONTROL_POS){
            wattron(_ventana_estado,COLOR_PAIR(2));
            wprintw(_ventana_estado," Camino corto: ");
            wattron(_ventana_estado,COLOR_PAIR(3));
            wprintw(_ventana_estado,"NO GESTIONAR");
        }



        //Realizamos el calculo para el modo de control correspondiente////////////////////////////////////////////////////////////////////
        wmove(_ventana_estado,2,1);
        wattron(_ventana_estado,COLOR_PAIR(2));
        wclrtoeol(_ventana_estado);
         //Revisamos si estamos en modo OPENLOOP
        if (_modo_control == MODO_CONTROL_OPENLOOP){
            _uk[0] = _tension_cte_teclado;
            wmove(_ventana_estado,1,7);
            wattron(_ventana_estado,COLOR_PAIR(3));
            wprintw(_ventana_estado,"OPEN LOOP          ");
            wmove(_ventana_estado,2,1);
            wattron(_ventana_estado,COLOR_PAIR(2));
            wprintw(_ventana_estado,"Tipo: ");
            wattron(_ventana_estado,COLOR_PAIR(3));
            wprintw(_ventana_estado,"Tension = %2.2f",_tension_cte_teclado);
        }
        else{ //Si no...
            //Realizamos control TODO o NADA segun combinacion de SW
            if ((_RB_Puerto1 & (1<<4)) && ((_RB_Puerto1 & (1<<5))==0) ){ //Si el bit 4 es 1 y bit 5 es 0
                wprintw(_ventana_estado,"Tipo: ");
                wattron(_ventana_estado,COLOR_PAIR(3));
                wprintw(_ventana_estado,"T/N tension = %.1fV",_tension_cte_teclado);
                if (_modo_control == MODO_CONTROL_POS){
                    _uk[0] = ControlTodoNada(_ek[0],_tension_cte_teclado);
                }
                else if (_modo_control == MODO_CONTROL_VEL){
                    _uk[0] = ControlTodoNadaVel(_ek[0],_uk, _tension_cte_teclado);
                }
            }
            //Realizamos control Proporcional segun combinacion de SW
            if ((_RB_Puerto1 & (1<<4)) && (_RB_Puerto1 & (1<<5)) ){ //Si el bit 4 es 1 y bit 5 es 0
                wprintw(_ventana_estado,"Tipo: ");
                wattron(_ventana_estado,COLOR_PAIR(3));

                if (_modo_control == MODO_CONTROL_POS){
                    wprintw(_ventana_estado,"P, KP = %.4f V/deg",_kp);
                    _uk[0] = ControlProporcional(_ek[0], _kp);
                }
                else if (_modo_control == MODO_CONTROL_VEL){
                    wprintw(_ventana_estado,"PI, KP = %.4f V/deg",_kp);
                    _uk[0] = ControlPI(_ek[0],_uk, _kp);
                }

            }
            //Realizamos control RZ segun combinacion de SW
            if ((_RB_Puerto1 & (1<<4))==0){ //Si el bit es 0
                wprintw(_ventana_estado,"Tipo: ");
                wattron(_ventana_estado,COLOR_PAIR(3));
                wprintw(_ventana_estado,"RZ =[");
                for (int i = 0; i <= _rz.M; i++){
                    wprintw(_ventana_estado,"%.2f ",_rz.b[i]);
                }
                wprintw(_ventana_estado,"]/");
                wprintw(_ventana_estado,"[");
                for (int i = 0; i <= _rz.N; i++){
                    wprintw(_ventana_estado,"%.2f ",_rz.a[i]);
                }
                wprintw(_ventana_estado,"]");
                //Calculamos productos escalares para control discreto Rz (util para depurar) tanto posicion como velocidad
                float prod1 = ProductoEscalar(_rz.b, _ek, _rz.M+1);
                float prod2 = ProductoEscalar(_rz.a+1,_uk+1,_rz.N);
                 _uk[0] = prod1 - prod2;//Usar regulador Rz
            }
        }

        //Actualizar estado de LEDs

        if (_velk[0]>0.1){ //Si esta girando a la derecha
            _RB_Puerto0 = _RB_Puerto0|(1<<6); //ActivarLED6
            _RB_Puerto0 = _RB_Puerto0&~(1<<7); //Desactivar LED 7
        }
        else if(_velk[0]<-0.1){ //Si gira a izquierdas
            _RB_Puerto0 = _RB_Puerto0|(1<<7); //Activar LED 7
            _RB_Puerto0 = _RB_Puerto0&~(1<<6); //Desactivar LED 6
        }
        else{ //Si esta parado apagar LEDS
            _RB_Puerto0 = _RB_Puerto0&~(1<<6); //Desactivar LED 6
            _RB_Puerto0 = _RB_Puerto0&~(1<<7); //Desactivar LED 6
        }

        //Parpadear el bit 5 si esta en modo control de posicion
        if (_conteo_interrupciones >= 5){
            _conteo_interrupciones = 0;
            if (_modo_control == MODO_CONTROL_POS){
                _RB_Puerto0 = _RB_Puerto0 ^ (1<<5);
                _RB_Puerto0 = _RB_Puerto0 & ~(1<<4);
            }
            //Parpadear el bit 4 si esta en modo control de velocidad
            else if (_modo_control == MODO_CONTROL_VEL){
                _RB_Puerto0 = _RB_Puerto0 ^ (1<<4);
                _RB_Puerto0 = _RB_Puerto0 & ~(1<<5);
            }
        }
    }

    //Aplicamos la tension en el motor
    ActivarTensionMotor(_uk[0]);
    LCD_gotoxy(1,1);
    switch (_modo_control){
    case MODO_CONTROL_POS:
        LCD_printf("CONTROL POS (deg)\n");
        break;
    case MODO_CONTROL_VEL:
        LCD_printf("CONTROL VEL (rpm)\n");
        break;
    case MODO_CONTROL_OPENLOOP:
        LCD_printf("CONTROL OPENLOOP\n");
        break;
    }
    LCD_gotoxy(1,2);
    LCD_printf("R:%6.2f Y:%  6.2f\n",refk,_yk[0]);
    //Obtenemos el estado anterior de los pulsadores
    _estado_SW[1] = _estado_SW[0];

    wattron(_ventana_estado,COLOR_PAIR(2));
    box(_ventana_estado,ACS_VLINE,ACS_HLINE);
    wrefresh(_ventana_estado); //Actualizamos una vez todos los cambios realizados
}
