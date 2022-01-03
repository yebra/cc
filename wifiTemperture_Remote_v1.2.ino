
/**  *
  WIFI MANAGER

  1. Versión
  Autor: Alberto Yebra
  Abril 2020
  Descripción:
  Sistema de calefacción encargado de:
  1- PANTALLA OLED con informacion de Temperat  ura
  2- Sensor de TEMPERATURA

  Todo sobre el ESP8266


    Conexión con Alexa
  Conexión a sensor DHT temperatura

  Versión 1.0

  1.1  Change Wifimanage library becasue solve problem when wifi switch off and system was not recovering properly after serveral hours without Router AP Wifi signal
  1.2 Primera versión estable instalada Diciembre 2021
      - Compatible con Alexa
      - Compatible con Blink
      



  nota compilar con la opción
  auxmoESP 3.X.X: When using Arduino Core for ESP8266 v2.4.X, double check you are building the project with LwIP Variant set to "v1.4 Higher Bandwidth".
* */


#define __DEBUG__ 1


#include "Arduino.h"
#include <Ticker.h> ///Librería para el parapadeo del led
#include "DHT.h"//Librería sensor de Temperatura DHT
#include <EEPROM.h> //Librería Acceso a la EEPROM
//#include <ESPAsyncTCP.h>
//#include <ESPAsyncWebServer.h>
//#include <AsyncElegantOTA.h>
#include <DNSServer.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <Wire.h>
//#include <WiFiManager.h> 
//PANTALLA OLED SSD1306
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

// Replace with your network credentials (STATION)
const char* ssid = "MOVISTAR_A2A0";
const char* password = "BE54316C434112149380";
//const char* ssid = "MOVISTAR_9A76_INV";
//const char* password = "Root2016@";

const char* PARAM_INPUT_1 = "Status";
const char* PARAM_INPUT_2 = "humedity";

//AsyncWebServer server2(8080);
DNSServer dns;


//Humedity PINES DHT22
#define DHTPIN 13 //(GPI1 13 - (D7) 
#define DHTTYPE DHT22


//OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 //LED_BUILTIN
// Initialize the OLED display using Wire library
Adafruit_SSD1306  display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  //D2=SDK  D1=SCK  As per labeling on NodeMCU

String webPage, notice;
//Variables que nos va a enviar el WifiManager para poder conocer cómo está configurado
int quemador = 3; //Valor 3 cuando no tenemos info
int calefaccion = 3;
int termostato = 3;

//Define la Struct que contiene la información que vamos a representar en la pantalla OLED
struct StructOLED {
  char cadenaTiempo[16];
  float temperatura;
  float consigna;
  boolean quemador;  //Indica si en estos momentos tenemos el quemador activado
  int Secuencia_Quemador; //Para realizar la animacion
  boolean termostato_pared; //Indica si el termostato de la pared está a ÓN
  boolean calefaccion; //Indica si tenemos la calefacción habilitada
  boolean mostrar_comunicacion; //Para indicar que hay un mensaje nuevo
  char comunicaciones[25]; //Informacion sobre el estado de las comunicaciones
};

StructOLED DatosOLED;


// Temporizador
unsigned long marcaTiempoDate = 0;
unsigned long tiempoRefreshDate = 1000;

// Variables almacena tiempo millis
int dias;
int horas;
int minutos;
int segundos;

// Cadena para almacenar texto formateado
char cadenaTiempo[16];

// Variable para recibir cuando se recibe peticiones http y mostrar en display que tiene comunicación
boolean estado_comunicacion;

DHT dht(DHTPIN, DHTTYPE);
////WiFiClient espClient;


//Variables información temporal
long lastMsg = 0;
long lastMsg2 = 0;

char msg[50];
int value = 0;


//IP address by default when you have connected to network
IPAddress local_IP(192, 168, 1, 3);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);


Ticker ticker;

// Pin LED azul
byte pinLed = 8;

//to take information about temperture and humedity
float h ;
float t ;

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16


static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};

static const unsigned char PROGMEM image_portada[8192] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x3f, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xcf, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xf3, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x3f, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc7, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x1f, 0xff, 0xff, 0x9f, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc7, 0xff, 0xff, 0xcf, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf1, 0xff, 0xff, 0xe3, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, 0xfb, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x83, 0xff, 0xf3, 0xfe, 0x3f, 0xff, 0xf9, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x38, 0x01, 0xf8, 0xff, 0x9f, 0xff, 0xfc, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xfc, 0x7c, 0x1f, 0x8f, 0xff, 0xfe, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0x3f, 0x0f, 0xc7, 0xff, 0xfe, 0x7f, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xdf, 0x8f, 0xe7, 0xff, 0xff, 0x3f, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xbf, 0xfe, 0xef, 0xcf, 0xf3, 0xff, 0xff, 0xbf, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xbf, 0xfe, 0xe7, 0xc7, 0xf9, 0xff, 0xff, 0x9f, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0xbf, 0xfe, 0xf7, 0xe7, 0xf8, 0xff, 0xff, 0xdf, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0xbf, 0xfe, 0xf7, 0xe7, 0xf8, 0xff, 0xff, 0xcf, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xfb, 0xff, 0xff, 0xf7, 0xe7, 0xfc, 0xff, 0xff, 0xef, 0xff, 0xff,
  0xff, 0xff, 0xfe, 0x3f, 0xff, 0xfb, 0xff, 0xff, 0xe7, 0xf7, 0xfe, 0xff, 0xff, 0xef, 0xff, 0xff,
  0xff, 0xff, 0xc0, 0xff, 0xff, 0xfb, 0xf7, 0xfb, 0xef, 0xf3, 0xfe, 0xff, 0xff, 0xef, 0xff, 0xff,
  0xff, 0xff, 0x1f, 0xff, 0xff, 0xfd, 0xf1, 0xf3, 0xdf, 0xfb, 0xff, 0x7f, 0xff, 0xef, 0xff, 0xff,
  0xff, 0xfe, 0x7f, 0xc1, 0xff, 0xfe, 0xfc, 0x07, 0x9f, 0xfb, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xfe, 0xff, 0x1f, 0xff, 0xff, 0x1f, 0xfe, 0x7f, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xfc, 0xfe, 0xff, 0x9f, 0xff, 0xcf, 0xfc, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xfd, 0xfd, 0xf9, 0x9f, 0xff, 0xf3, 0xf9, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xfd, 0xf3, 0x0f, 0xff, 0xfc, 0x07, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xef, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xf8, 0x03, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfb, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfb, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfb, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfb, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xf9, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfd, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfd, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfd, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xf9, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfb, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfb, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfb, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfb, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xfb, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xf3, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xf7, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xc7, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xdf, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static const unsigned char PROGMEM image_data_CalefaccionOFF[1024] = {
  0x01, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x0f, 0xf0, 0x1f, 0xc3, 0xf8,
  0x01, 0x00, 0x02, 0x0f, 0xf0, 0x1f, 0xc3, 0xf8, 0x01, 0x00, 0x02, 0x30, 0x0c, 0x60, 0x0c, 0x00,
  0x01, 0x00, 0x02, 0x30, 0x0c, 0x60, 0x0c, 0x00, 0x01, 0x00, 0x02, 0x30, 0x0c, 0x60, 0x0c, 0x00,
  0x01, 0x00, 0x02, 0x30, 0x0c, 0x60, 0x0c, 0x00, 0x01, 0xff, 0xfe, 0x30, 0x0c, 0x7f, 0xcf, 0xf8,
  0x01, 0xff, 0xfe, 0x30, 0x0c, 0x7f, 0xcf, 0xf8, 0x01, 0x00, 0x02, 0x30, 0x0c, 0x60, 0x0c, 0x00,
  0x01, 0x7f, 0xfa, 0x30, 0x0c, 0x60, 0x0c, 0x00, 0x01, 0x7f, 0xfa, 0x30, 0x0c, 0x60, 0x0c, 0x00,
  0x01, 0x7f, 0xfa, 0x30, 0x0c, 0x60, 0x0c, 0x00, 0x01, 0x7f, 0xfa, 0x0f, 0xf0, 0x60, 0x0c, 0x00,
  0x01, 0x00, 0x02, 0x0f, 0xf0, 0x60, 0x0c, 0x00, 0x01, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const unsigned char PROGMEM image_data_CalefaccionON[1024] = {
  0x01, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x03, 0xfc, 0x0c, 0x00, 0xc0,
  0x01, 0x7f, 0xfa, 0x03, 0xfc, 0x0c, 0x00, 0xc0, 0x01, 0x7f, 0xfa, 0x0c, 0x03, 0x0f, 0x00, 0xc0,
  0x01, 0x7f, 0xfa, 0x0c, 0x03, 0x0f, 0x00, 0xc0, 0x01, 0x7f, 0xfa, 0x0c, 0x03, 0x0c, 0xc0, 0xc0,
  0x01, 0x00, 0x02, 0x0c, 0x03, 0x0c, 0xc0, 0xc0, 0x01, 0xff, 0xfe, 0x0c, 0x03, 0x0c, 0x30, 0xc0,
  0x01, 0xff, 0xfe, 0x0c, 0x03, 0x0c, 0x30, 0xc0, 0x01, 0x00, 0x02, 0x0c, 0x03, 0x0c, 0x0c, 0xc0,
  0x01, 0x00, 0x02, 0x0c, 0x03, 0x0c, 0x0c, 0xc0, 0x01, 0x00, 0x02, 0x0c, 0x03, 0x0c, 0x03, 0xc0,
  0x01, 0x00, 0x02, 0x0c, 0x03, 0x0c, 0x03, 0xc0, 0x01, 0x00, 0x02, 0x03, 0xfc, 0x0c, 0x00, 0xc0,
  0x01, 0x00, 0x02, 0x03, 0xfc, 0x0c, 0x00, 0xc0, 0x01, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char PROGMEM image_data_Quemador1[1024] = {
  0x00, 0x00, 0xfc, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87, 0x3f, 0x73, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x81, 0x61, 0x41, 0xbe, 0x00, 0x00, 0x00, 0x00, 0x61, 0xc1, 0x40, 0xa3, 0x00, 0x00,
  0x00, 0x06, 0x60, 0x01, 0xc0, 0xe0, 0xc0, 0x00, 0x00, 0x05, 0xc0, 0x00, 0x00, 0x00, 0x40, 0x00,
  0x00, 0x06, 0x01, 0x80, 0x0c, 0x80, 0x40, 0x00, 0x00, 0x03, 0xc3, 0xc4, 0x3d, 0xf0, 0x40, 0x00,
  0x00, 0x00, 0xc1, 0xcc, 0x3f, 0xf1, 0xc0, 0x00, 0x00, 0x00, 0xc1, 0xfb, 0xff, 0xc3, 0x00, 0x00,
  0x00, 0x00, 0xc0, 0x01, 0xef, 0x02, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0xc3, 0x1e, 0x00, 0x00,
  0x00, 0x00, 0x30, 0x30, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0xe0, 0xc0, 0x00, 0x00,
  0x00, 0x00, 0x1f, 0xed, 0xa1, 0x80, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc7, 0x1f, 0x00, 0x00, 0x00
};
static const unsigned char PROGMEM image_data_Quemador2[1024] = {
  0x00, 0x00, 0xf0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x03, 0xf0, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x90, 0x73, 0x70, 0x07, 0x80, 0x00, 0x00, 0x06, 0x70, 0xd1, 0x78, 0x07, 0x80, 0x00,
  0x00, 0x06, 0x78, 0x93, 0xcc, 0x05, 0x80, 0x00, 0x00, 0x05, 0xc8, 0xb6, 0x0c, 0x1c, 0x80, 0x00,
  0x00, 0x06, 0x0c, 0xb4, 0x04, 0xb1, 0x80, 0x00, 0x00, 0x03, 0xcf, 0xdc, 0x35, 0xb1, 0x80, 0x00,
  0x00, 0x00, 0xce, 0x8c, 0x3f, 0xfb, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x70, 0x7c, 0x27, 0x00, 0x00,
  0x00, 0x00, 0xc0, 0x00, 0x29, 0xcf, 0x00, 0x00, 0x00, 0x01, 0x87, 0xc0, 0x07, 0xfe, 0x00, 0x00,
  0x00, 0x03, 0x0c, 0x30, 0x0d, 0xe0, 0x00, 0x00, 0x00, 0x02, 0x0c, 0x38, 0xf8, 0x00, 0x00, 0x00,
  0x00, 0x03, 0xef, 0xed, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xc7, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char PROGMEM   image_data_TermostatoPared [] = {
  0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04,
  0x10, 0xf8, 0x04, 0x70, 0x00, 0x3f, 0xf8, 0x04, 0x10, 0x04, 0x04, 0x50, 0x00, 0x40, 0x04, 0x04,
  0x10, 0x04, 0x04, 0x70, 0x00, 0x43, 0x84, 0x04, 0x10, 0x04, 0x04, 0x00, 0x00, 0x40, 0x04, 0x04,
  0x10, 0x04, 0x04, 0x00, 0x00, 0x7f, 0xfc, 0x04, 0x10, 0x78, 0x00, 0x00, 0x00, 0x40, 0x04, 0x04,
  0x10, 0x80, 0x04, 0x00, 0x00, 0x40, 0x04, 0x04, 0x10, 0x80, 0x04, 0x00, 0x00, 0x41, 0x04, 0x04,
  0x10, 0x80, 0x04, 0x00, 0x00, 0x43, 0x84, 0x04, 0x10, 0x80, 0x04, 0x00, 0x00, 0x41, 0x04, 0x04,
  0x10, 0x7c, 0x04, 0x00, 0x00, 0x40, 0x04, 0x04, 0x10, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xf8, 0x04,
  0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc
};



unsigned long previousMillis = 0;
unsigned long interval = 30000;


void handleTempertature(){
  h = dht.readHumidity();
  t = dht.readTemperature();
     Serial.println("Printing OLED Display");
     //OLED_print();
  //display.clearDisplay();
  //display.setTextSize(2);
  //display.setCursor(0, 16);
  //display.println("TEST");
  //display.display(); 
  //server.send(200,"text/plain", String(t, 3).c_str());
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  // Display static text
  display.println("Hello, world!");
  display.display();

  
   server.send(200,"text/plain","TEST");
  //OLED_print();

}



void setup()
{

  Serial.begin(115200);
#ifdef __DEBUG__
  Serial.println("Initializing OLED Display");
  Serial.begin(115200);
  Serial.println("");
 
#endif
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  DatosOLED.mostrar_comunicacion = 0; //Inicialmente estamos a 0 en las comunicaciones no mensaje de ESTADO;
  DatosOLED.Secuencia_Quemador = 0; //Para visualización iniciamos la variable de animacion;
 
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.invertDisplay(true);
  display.fillScreen(0);         //Limpiamos la pantalla
  display.drawBitmap(0, 0, image_portada, 128, 64, SSD1306_WHITE);
  display.display();
  delay(3000); // Pause for 2 seconds

  //display.drawBitmap(0, 0, image_portada, 128, 64, SSD1306_WHITE);
  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  
 //TEXTO CON FUENTE PREDETERMINADA
  display.invertDisplay(false);
  display.fillScreen(0);         //Limpiamos la pantalla

  display.setFont();             //Fuente por defecto -si no la hemos cambiado no es necesario seleccionarla

  display.setTextSize(1);
  display.setTextColor(1, 0);
  display.setCursor(0, 0);
  display.println("Temperatura");
  display.setTextSize(2);
  display.print("IKER-ANDER");

  display.setTextSize(2);
  display.setTextColor(1, 0);    //Color invertido
  display.setCursor(0, 32);
  display.print("Iniciando...");
  display.display();             //Refrescamos la pantalla para visualizarlo
  Serial.println("Iniciando OLED");
  delay(4000);

  // Modo del pin
  //pinMode(pinLed, OUTPUT);

  // Empezamos el temporizador que hará parpadear el LED
  //ticker.attach(0.2, parpadeoLed);

  // Connect to Wi - Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  //WiFi.mode(WIFI_STA);

  WiFi.config(local_IP, gateway, subnet);
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }

  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.print("ESP IP Address: http://");
  Serial.println(WiFi.localIP());
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());

#ifdef __DEBUG__
  Serial.println("Ya estás conectado");
#endif

  // Eliminamos el temporizador
  ticker.detach();

  // Apagamos el LED
  //digitalWrite(pinLed, HIGH);

  dht.begin();


  // Setting the ESP as an access point
#ifdef __DEBUG__
  Serial.print("Setting STA ");
#endif
  // Remove the password parameter, if you want the AP (Access Point) to be open


  ///server.on("/temperature", handleTempertature);

  ///server.begin();
  //AsyncElegantOTA.begin(&server2);    // Start ElegantOTA
  
}



void loop() {
  ///server.handleClient();
  
  //AsyncElegantOTA.loop();
  //ESP.wdtFeed(); // feeds the dog
  //delay(0);
  //OLED_print();
  //yield();
  /*
    Value  Constant  Meaning
    0 WL_IDLE_STATUS  temporary status assigned when WiFi.begin() is called
    1 WL_NO_SSID_AVAIL   when no SSID are available
    2 WL_SCAN_COMPLETED scan networks is completed
    3 WL_CONNECTED  when connected to a WiFi network
    4 WL_CONNECT_FAILED when the connection fails for all the attempts
    5 WL_CONNECTION_LOST  when the connection is lost
    6 WL_DISCONNECTED when disconnected from a network
  */


  // Protección overflow
  if (millis() < marcaTiempoDate) {
    marcaTiempoDate = millis();
  }


  // Comprobar is hay que actualizar temperatura
  if (millis() - marcaTiempoDate >= tiempoRefreshDate)
  {
    // Actualizar variables de tiempo
    millisToTiempo(millis());
    // Componer cadena con la información del tiempo formateada
    //sprintf(cadenaTiempo, "%02d:%02d:%02d:%02d", dias, horas, minutos, segundos);

    // Marca de tiempo
    marcaTiempoDate = millis();
  }

  long now = millis();

  unsigned long currentMillis = millis();
  
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    Serial.println(WiFi.localIP());
    //Alternatively, you can restart your board
    //ESP.restart();
    Serial.println(WiFi.RSSI());
    previousMillis = currentMillis;
  }



  // if WiFi is down, try reconnecting
  if (now - lastMsg2 >= 400000) {
    Serial.print("Check if the connection is ok Status");
    Serial.print(WiFi.status() );
    Serial.print("The IP adress: ");
    Serial.println(WiFi.localIP());

    if (WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect();
      Serial.println(" Disconnecting!");
      delay(100);
      WiFi.begin(ssid, password);
      if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println(" WiFi Failed!");
        return;
      }
      WiFi.reconnect();
      Serial.println(" Reconecting!");
    }

    lastMsg2 = now;

  }

  if (now - lastMsg > 4000){
    lastMsg = now;
    ++value;
    //OLED_recorrer();
    //snprintf (msg, 75, "%d", t);
    h = dht.readHumidity();
    t = dht.readTemperature();
    t = 15.2;
 
    if (isnan(h) || isnan(t)) {
      Serial.println("Error en la lectura del sensor!\n");
      return;
    }
    Serial.println("Temperture ");
    Serial.println(t);
    sprintf(DatosOLED.cadenaTiempo, cadenaTiempo);
    DatosOLED.temperatura = t;
    DatosOLED.consigna=12.3;
    //display.display();
    //OLED_print();
    //  display.clearDisplay();
    //  display.setTextSize(2);
    //  display.setCursor(0, 16);
    //  display.println(DatosOLED.temperatura, 1);
    //  display.display(); 
  }
  //delay(2000);
  // DatosOLED.consigna = round(Consigna);//
  //Tomamos estado del quemador para OLED

  //OLED_print2();
  
  //delay(2000);
  OLED_print();
}

void OLED_print2(void){
  display.clearDisplay();
  
  // display temperature
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Temperature: ");
  display.setTextSize(2);
  display.setCursor(0,10);
  display.print(t);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  display.setTextSize(2);
  display.print("C");
  
  // display humidity
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("Humidity: ");
  display.setTextSize(2);
  display.setCursor(0, 45);
  display.print(h);
  display.print(" %"); 
  
  display.display(); 
}

/*
  Función que convierte millis() a segundos, minutos, horas y días
  Almacena la información en variables globales
*/
void millisToTiempo(unsigned long valMillis) {
  // Se obtienen los segundos
  valMillis = valMillis / 1000;

  segundos = valMillis % 60; // se divide entre segundos por minuto y te quedas con el resto
  minutos = valMillis / 60; // Se convierte a minutos
  minutos = minutos % 60; // se divide entre minutos por hora y te quedas con el resto
  horas = (valMillis / 60) / 60; // Se convierte en horas
  horas = horas % 24; // se divide entre horas al día y te quedas con el resto
  dias = ((valMillis / 60) / 60) / 24; // Se convierte en días
#ifdef __DEBUG__

#endif
}


//OLED function
void OLED_print(void) {

  //Serial.print("\n OLED:Secuencia_QUEMADOR:");
  //Serial.print(DatosOLED.Secuencia_Quemador);
  display.clearDisplay();
  //Serial.println("Imprimientodo en OLED");
  //display.fillScreen(0);         //Limpiamos la pantalla

  // Dibujar texto tiempo
  DatosOLED.Secuencia_Quemador = DatosOLED.Secuencia_Quemador + 1;
  if (DatosOLED.Secuencia_Quemador > 3) {
    DatosOLED.Secuencia_Quemador = 0;
  }
  //SE muestra el menaje una vez recibida una comunicación, después ya el funcionamiento de todo
  //Bloque 1
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(DatosOLED.cadenaTiempo);

  //Bloque 2
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.println(DatosOLED.temperatura, 1);
  //display.cp437(true);
  //display.write(167);
  //display.println("C");

  // Bloque 3
  display.setTextSize(2);
  display.setCursor(70, 0);
  display.println(DatosOLED.consigna, 1);
  //display.cp437(true);
  //display.write(167);
  //display.println("C");

  // Bloque 4
  //Serial.print("\n OLED:Quemador:");
  //Serial.print(DatosOLED.quemador);

  if (DatosOLED.quemador == 1) {
    if (DatosOLED.Secuencia_Quemador == 1) {
      display.drawBitmap(64, 16, image_data_Quemador1, 64, 16, SSD1306_WHITE);
    } else {
      DatosOLED.Secuencia_Quemador = 0;
      display.drawBitmap(64, 16, image_data_Quemador2, 64, 16, SSD1306_WHITE);
    }
  }

  // Bloque 5
  //Serial.println("DatosOLED.termostato_pared");
  //Serial.println(DatosOLED.termostato_pared);

  //Serial.print("\n OLED:Termostato:");
  //Serial.print(DatosEE.E_Termostato);

  if (DatosOLED.termostato_pared == 1) {
    //display.fillCircle(16,48 , 8, SSD1306_WHITE);
    display.drawBitmap(0, 40, image_data_TermostatoPared, 64, 16, SSD1306_WHITE);
  }//else display.drawCircle(16, 48, 8, SSD1306_WHITE);

  // Bloque 6

  //Serial.print("\n OLED:Calefaccion:");
  //Serial.print(DatosOLED.calefaccion);

  if (DatosOLED.calefaccion == 1) {
    display.drawBitmap(64, 40, image_data_CalefaccionON, 64, 16, SSD1306_WHITE);
  } else display.drawBitmap(64, 40, image_data_CalefaccionOFF, 64, 16, SSD1306_WHITE);
  //display.drawCircle(96, 64, 8, SSD1306_WHITE);
  // Bloque 7
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.println(DatosOLED.comunicaciones);
  //Limpiamos el buffer una vez mostrada una vez mostrada en el display
  sprintf(DatosOLED.comunicaciones, "");

  display.display();
}
void OLED_parpadeo() {
  //Función para mostrar un parpadeo en esquina derecha cuando se recibe un dato
  estado_comunicacion = !estado_comunicacion;
  if (estado_comunicacion = 1) {
    display.drawCircle(123, 5, 5, SSD1306_WHITE);
    display.fillCircle(123, 5, 5, SSD1306_WHITE);
  } else {
    display.drawCircle(123, 5, 5, SSD1306_WHITE);
  }
  display.display();

}
void OLED_recorrer() {
  // Limpiar buffer pantalla
  display.clearDisplay();
  // Dibujar línea horizontal
  display.drawLine(0, 18, display.width(), 18, SSD1306_WHITE);
 
  // Dibujar texto tiempo
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(cadenaTiempo);
 
  // Enviar a pantalla
  display.display();
}
void parpadeoLed() {
  // Cambiar de estado el LED
  byte estado = digitalRead(pinLed);
  digitalWrite(pinLed, !estado);
}
