
//Caudalimetro Automatizado
//Desarrollado por: Allan Valle y Jose Mejía
//Universidad Nacional Autónoma de Honduras
//En colaboración con el Instituto Hondureño de Ciencias de la Tierra


//Configuración del Real Time Clock (RTC)
#include <DS3231.h>
DS3231  rtc(A4, A5);  //SDA -> A4, SCL -> A5
Time  t;
String dia;
String fecha;
String hora;
String mes;
String anho;
String diaSemana;
String horaCompleta;
String fechaCompleta;
String minutos;


//Definición de librerías para el módulo SD
#include <SD.h>
#include<SPI.h>
File myFile;    //Definición del objeto myFile/
const int pinSD = 10; //Chip select del módulo SD.


//Datos del Tanque
const float alto_tanque = 200; // La distancia a la cual esta colocado el sensor respecto a la base del tanque. [cm]
const float area_base = 7850; // [cm^2]


//Definición de pines y constantes del sensor ultrasónico
const int PinTrig = 4;
const int PinEcho = 3;
const float VelSon = 34600.0; // [cm/s] A 25 grados centigrados.
float distancia; //distancia entre el sensor y el agua


//Definición de variables para el calculo del caudal
float altura; //altura del tanque
double volumen; //volumen medido
double volumenAntMin; //Aqui se almacena el volumen medido cada minuto.
double volumenAntHora; //Aqui se almacena el volumen medido cada hora.
double caudalMin; //Aqui se almacena el caudal medido cada minuto.
double caudalHora; //Aqui se almacena el caudal medido cada hora.
float tiempoAntMin; //Almacena como evento que ya transcurrió un minuto.
float tiempoAntHora; //Almacena como evento que ya transcurrió una hora.


// Luz indicadora como indicador del estado de funcionamiento
int luzIndicadora = 8;


void setup() {
  Serial.begin(9600);

  //Sensor Ultrasónico
  pinMode(PinTrig, OUTPUT);
  pinMode(PinEcho, INPUT);

  tiempoAntMin = millis(); // Evento que se actualizará cada minuto
  tiempoAntHora = millis(); // Evento que se actualizará cada hora


  rtc.begin(); // Se inicializa el RTC.

  // Configuracion inicial RTC. Solo subir una vez.
  /*rtc.setDOW(FRIDAY);     // Set Day-of-Week to SUNDAY
    rtc.setTime(10, 25, 0);     // Set the time to 12:00:00 (24hr format)
    rtc.setDate(19, 10, 2018);   // Set the date to January 1st, 2014
  */

  //Luz Indicadora
  pinMode(luzIndicadora, OUTPUT);

  //Volumen Inicial
  volumenAntMin = calculo_volumen();  //Condición inicial de volumen en el tanque.
  volumenAntHora = volumenAntMin;
}

void loop() {

  caudalCadaMinuto();
}


//Esta funcion verifica si la SD se inicializó.
void sdInitialization() {
  if (!SD.begin(pinSD)) {
    Serial.println("initialization failed!");
    digitalWrite(luzIndicadora, HIGH); //Si no se logra inicializar se encenderá una luz de alerta.
    return;
  }
  Serial.println("initialization done.");
  digitalWrite(luzIndicadora, LOW);  //Si no se enciende la luz -> Se inicializó correctamente.
}


void tiempo() {
  t = rtc.getTime();  // Lee tiempo y fecha actual del RTC.
  fecha = t.date;     //Retorna dia del mes ej. 19
  mes = t.mon;        //Retorna mes ej. Octubre -> 10
  minutos = t.min;    //Retorna los minutos.
  hora = t.hour;      //Retorna la hora ej. 5pm -> 17
  fechaCompleta = rtc.getDateStr(FORMAT_SHORT, FORMAT_LITTLEENDIAN); //Retorna la fecha completa -> 19.10.18
  anho = fechaCompleta.substring(6);    //Retorna solo el año (ultimos dos bytes)de la cadena anterior, ej. 2018 -> 18
  horaCompleta = rtc.getTimeStr();      //Retorna la hora completa hh:mm:ss.
}


void sdCard(float caudal, String nombreArchivo) {

  String dato = fechaCompleta + "," + horaCompleta + "," + String(caudal);  // Se construye una cadena la información como una fila dentro del archivo final.
  myFile = SD.open(nombreArchivo + ".txt" , FILE_WRITE);  //Se abre el archivo.

  if (myFile) {
    Serial.print("Writing to " + nombreArchivo + ".txt..."); //Si se logra abrir el archivo se muestra este mensaje.
    myFile.println(dato);  //Se escribe la cadena de información dentro del archivo.
    myFile.close();        //Se cierra el archivo
    Serial.println("done.");
  }

  else {
    Serial.println("error opening " + nombreArchivo + ".txt"); //Se muestra un mensaje de error si no se logra abrir el archivo.
  }

  //Se verifica que los datos se escribieron correctamente.
  myFile = SD.open(nombreArchivo + ".txt");
  if (myFile) {
    Serial.println(nombreArchivo + ".txt opened correctly:");

    /*while (myFile.available()) {   //Se leen los datos que contiene el archivo.
      Serial.write(myFile.read());
          }*/

    myFile.close(); // Se cierra el archivo.
  }

  else {
    Serial.println("error opening " + nombreArchivo + ".txt"); //Si no se puede abrir el archivo se muestra un mensaje de error.
  }
  return;
}


void iniciarTrigger()  //Se transmite un pulso de 10 microsegundos al pin indicado.
{
  digitalWrite(PinTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(PinTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(PinTrig, LOW);
}

double calculo_distancia ()
{
  iniciarTrigger();
  unsigned long tiempo = pulseIn(PinEcho, HIGH); // Tiempo que le toma al pulso retornar al sensor ultrasónico. [us]
  distancia = tiempo * 0.000001 * VelSon / 2.0;  //Se calcula la distancia. [cm]
  //Para Debugging.
  /*Serial.print(distancia);
    Serial.println("m");*/

  return distancia;
}



float calculo_volumen () {
  calculo_distancia();
  while (distancia <= alto_tanque) {  //Mientras la altura del agua este por debajo del sensor.
    altura = alto_tanque - distancia;   //Se calcula la altura del agua.

    /*Serial.print("Altura de agua: "); //Para debugging.
      Serial.println(altura);*/

    volumen = area_base * altura / 1000000;

    /*Serial.print("Volumen: ");  // Para debugging.
      Serial.println(volumen);*/

    return volumen;
  }
}

float caudalCadaMinuto() {  //Se calcula el caudal promedio de cada minuto.

  if (millis() - tiempoAntMin > 60000) { // Una vez transcurrido un minuto.
    sdInitialization();             //Se inicializa la SD
    tiempoAntMin = millis();        //Se guarda el tiempo en el cual ocurrió este evento.
    calculo_volumen();              //Se calcula el volumen en ese momento.
    caudalMin = (volumen - volumenAntMin) / 60000; //Se calcula el caudal promedio en un minuto.
    volumenAntMin = volumen;        //Se guarda el volumen calculado
    tiempo();                       //Se extrae el tiempo en cual se tomaron las mediciones.
    String minFile = fecha + mes + anho + hora;  //Nombre del archivo en el cual se guardará la información. El archivo se nombra como fecha mes año hora.
    sdCard(caudalMin, minFile);                  //Se escriba la información en la SD.

    caudalCadaHora();  //Se verifica si ya transcurrió una hora.
  }
}

void caudalCadaHora() {

  if (millis() - tiempoAntHora > 3600000) {  //Una vez transcurrida una hora.
    tiempoAntHora = millis();
    calculo_volumen();
    caudalHora = volumen - volumenAntHora / 3600000; //Se calcula el caudal promedio en una hora.
    volumenAntHora = volumen;
    tiempo();
    String hourFile = "cp" + fecha + mes + anho;  //Nombre del archivo que contiene los caudales promedio de cada hora en ese dia.
    sdCard(caudalHora, hourFile);
  }
}

