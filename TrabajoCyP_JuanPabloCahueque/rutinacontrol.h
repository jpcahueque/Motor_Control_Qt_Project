#ifndef RUTINACONTROL_H
#define RUTINACONTROL_H

//Funcion para manejar rutina de servicio de temporización
void ISR_TimerControl();

//Funcion que devuelve el valor del potenciometro en grados
//Return float grados de referencia del potenciometro
float LeerRef_deg();

//Funcion que devuelve un valor correspondiente a una entrada segun la relacion lineal encontrada con los otros puntos
//Param
//I valor de entrada para conocer su valor correspondiente
//x1 valor en x mas pequeño para relacion lineal
//x2 valor en x mas grande para relacion lineal
//y1 valor en y mas pequeño para relacion lineal
//y2 valor en y mas grande para relacion lineal
//Return float correspondiente al valor de entrada segun relacion lineal
float RelacionLineal(float I, float x1, float x2, float y1, float y2);

//Funcion que devuelve la posicion del motor en grados
//Return float de la posicion del motor en grados
float LeerPosMotor_deg();

//Funcion que devuelve la velocidad del motor en RPM
//Return float de la velocidad del motor en RPM
float LeerVelMotor_rpm();

//Funcion que activa el motor segun la tension indicada
//Param u_motor_v float de la tension del motor
void ActivarTensionMotor(float u_motor_v);

//Funcion que devuelve tension a alimentar al motor segun control Todo Nada para posicion
//Param
//errk ultimo error de posicion
//tension Tension a manejar en el Control TodoNada
float ControlTodoNada(float errk, float tension);

//Funcion que devuelve la tension a alimentar al motor según control Proporcional
//Param
//errk ultimo error de posición
//kp constante proporcional en control P
//Return float de la tensión a ingresar al motor según control
float ControlProporcional(float errk, float kp);

//Funcion que devuelve la referencia indicada por el potenciómetro en RPM
//Return float con las rpm indicadas por el potenciómetro
float LeerPot_rpm();

//Funcion que devuelve la tensión a alimentar al motor según control PI de velocidad
//Param
//errk ultimo error
//uk[] puntero a tabla de ultimas tensiones alimentadas al motor
//kp constante proporcional en control PI
//Return float de tensión a ingresar al motor según control
float ControlPI(float errk, float uk[], float kp);

//Funcion que devuelve la tensión a alimentar según control Todo Nada para velocidad
//Param
//errk ultimo error de velocidad
//uk[] puntero a tabla de ultimas tensiones alimentadas al motor
//tension Tension a manejar en el Control Todo Nada
//Return float de tensión a ingresar al motor según control
float ControlTodoNadaVel(float errk, float uk[], float tension);

#endif // RUTINACONTROL_H
