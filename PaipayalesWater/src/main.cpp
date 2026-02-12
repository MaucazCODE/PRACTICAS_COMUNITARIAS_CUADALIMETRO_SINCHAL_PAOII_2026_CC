#include <SPI.h>
#include <SoftwareSerial.h>
#include <ModbusMaster.h>
     
#define SLAVE_ID 1
#define REGISTRO_FLUJO 0x00  // Flow Rate m^3/h
#define LONGITUD_FLUJO 2
#define REGISTRO_CONSUMO 0x7c //Flow for this day m^3
#define LONGITUD_CONSUMO 2
#define MAX485_DE 22
#define MAX485_RE 21


// Instanciar el objeto con ambas librerías. 
ModbusMaster sensor;
void leer_flujo();
void leer_consumo();
void imprimir_resultados();
void enviar_a_esp32();

void preTransmission()
{
  delay(10);  
  digitalWrite(MAX485_RE, HIGH); // Modo transmisión.
  digitalWrite(MAX485_DE, HIGH);
}

void postTransmission()
{
  delay(10);
  digitalWrite(MAX485_RE,LOW); //Modo recepción
  digitalWrite(MAX485_DE,LOW);
}

void setup()
{
  pinMode(MAX485_RE, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  // Init in receive mode
  digitalWrite(MAX485_RE,LOW);
  digitalWrite(MAX485_DE,LOW);
  //Serial.begin(9600);
  //Serial.println("PRUEBA CON EL SIMULADOR SLAVE MODBUS");
  Serial1.begin(9600,SERIAL_8N1); //Cambio 8o1 por 8N1
  sensor.begin(1, Serial1); //SLAVE_ID cambio por 1  este serial es para el flujometro
  sensor.preTransmission(preTransmission);
  sensor.postTransmission(postTransmission);

  while (!Serial);
  Serial.begin(9600);
  delay(100);
  Serial.println("Inicializando programa");
  Serial.println("Inicializando comunicacion con ESP32");
  Serial2.begin(9600);

}

int16_t packetnum = 0;  // packet counter, we increment per xmission

union
{
  uint32_t i;
  float f;
} flujo;

union
{
  uint32_t j;
  float t;
} consu;

void loop() 
{
  //Serial.println("LECTURA DE REGISTROS");
  leer_flujo();
  leer_consumo();

  imprimir_resultados();
  Serial.println("Sending to ESP32-heltec V3");
  // Send a message to heltec v3 (programa despues)
  enviar_a_esp32();
  delay(2000);

}

void leer_flujo() {
  uint8_t result;
  result = sensor.readHoldingRegisters(REGISTRO_FLUJO,LONGITUD_FLUJO);
  delay(1500);
  if (result == sensor.ku8MBSuccess) {
    flujo.i = (((unsigned long)sensor.getResponseBuffer(0x01)<<16) | (sensor.getResponseBuffer(0x00)));
  } else  {
    Serial.println("Error al leer flujo: ");
    flujo.f = -1.0;
  }
}
  
void leer_consumo(){
  uint8_t result;
  result = sensor.readHoldingRegisters(REGISTRO_CONSUMO,LONGITUD_CONSUMO);
  delay(1500);
  if (result == sensor.ku8MBSuccess) {
    consu.j = (((unsigned long)sensor.getResponseBuffer(0x01)<<16) | (sensor.getResponseBuffer(0x00)));
    //cons.k=(((unsigned long)sensor.getResponseBuffer(0x03)<<16) | (sensor.getResponseBuffer(0x02)));
  } else  {
    Serial.println("Error al leer consumo: ");
    consu.t = -1;
  }
} 

void imprimir_resultados(){
  // Imprimimos por pantalla de ambos datos
  Serial.println("Flujo es:  "+String(flujo.f,5)+ " m3/s");
  Serial.println("Consumo es:  "+String(consu.t,5)+ " m3");  
}


//para comunicar
void enviar_a_esp32() {
  String mensaje=String(flujo.f,3)+";"+String(consu.t,3)+"E";
  //enviarlo como un solo paquete de string
  Serial.println(mensaje); 
  Serial2.println(mensaje);
}
