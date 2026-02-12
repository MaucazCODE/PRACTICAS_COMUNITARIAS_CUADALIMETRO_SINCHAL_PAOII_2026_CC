#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <HardwareSerial.h>

double variableFLUJO = 0.0;
double variableCONSUMO = 0.0;

String textoMega = "Esperando...";
char txpacket[50];

int IDsender = 3;   // Debe coincidir con el receptor: T03
bool lora_idle = true;

static RadioEvents_t RadioEvents;
static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED,
                           GEOMETRY_128_64, RST_OLED);

void VextON(void)
{
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void setup() {

  // UART con Arduino Mega
  Serial1.begin(9600, SERIAL_8N1, 44, 43);  // RX=44, TX=43

  Serial.begin(115200);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  // Configuración LoRa
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(915000000);

  Radio.SetTxConfig(MODEM_LORA,
                    5,      // potencia
                    0,
                    0,      // BW 125 kHz
                    7,      // SF7
                    1,      // CR 4/5
                    8,
                    false,
                    true,
                    0,
                    0,
                    false,
                    3000);

  // Encender pantalla
  VextON();
  delay(100);
  display.init();
  display.setFont(ArialMT_Plain_10);

  Serial.println("Transmisor LoRa listo");
}

void displayVARIABLES(double num1, double num2, String texto) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  display.drawString(0, 0, "Flujo:");
  display.drawString(0, 10, String(num1, 3) + " m3/h");

  display.drawString(0, 20, "Consumo:");
  display.drawString(0, 30, String(num2, 3));

  display.drawString(60, 20, "Emisor");
  display.drawString(60, 30, "Flowmeter");

  display.drawString(0, 45, texto);

  display.display();
}

void loop() {

  // ============================
  // LECTURA DESDE ARDUINO MEGA
  // ============================
  if (Serial1.available() > 0) {
    textoMega = Serial1.readStringUntil('E');
    textoMega.trim();

    // Parseo simple: flujo;consumoE
    int sep = textoMega.indexOf(';');
    if (sep > 0) {
      variableFLUJO = textoMega.substring(0, sep).toFloat();
      variableCONSUMO = textoMega.substring(sep + 1).toFloat();
    }

  } else {
    textoMega = "Sin datos del Mega";
  }

  // Mostrar en pantalla
  displayVARIABLES(variableFLUJO, variableCONSUMO, textoMega);

  // ============================
  // TRANSMISIÓN LORA
  // ============================
  if (lora_idle) {

    sprintf(txpacket, "T%02d|%0.3fC%0.3f",
            IDsender, variableFLUJO, variableCONSUMO);

    Serial.printf("\nEnviando paquete: \"%s\"\n", txpacket);

    Radio.Send((uint8_t *)txpacket, strlen(txpacket));
    lora_idle = false;
  }

  Radio.IrqProcess();
}

// ======================================================
// CALLBACKS LORA
// ======================================================
void OnTxDone(void)
{
  Serial.println("TX completado");
  lora_idle = true;
}

void OnTxTimeout(void)
{
  Serial.println("TX Timeout");
  Radio.Sleep();
  lora_idle = true;
}