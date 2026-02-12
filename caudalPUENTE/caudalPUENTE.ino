#include <Wire.h>
#include "HT_SSD1306Wire.h"
#include "LoRaWan_APP.h"
#include "Arduino.h"

double variableFLUJO = 0.0;
double variableCONSUMO = 0.0;

char idDESEADO[4];   // espacio para "T03" + '\0'
char rxpacket[50];

char txpacket[50]; //para recibir

static RadioEvents_t RadioEvents;
bool lora_idle = true;

int16_t rssi, rxSize;

static SSD1306Wire display(0x3c, 500000, SDA_OLED, SCL_OLED,
                           GEOMETRY_128_64, RST_OLED);

void VextON(void)
{
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void setup() {
  Serial.begin(9600);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  // Configuración LoRa
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;

  Radio.Init(&RadioEvents);
  Radio.SetChannel(915000000);

  Radio.SetRxConfig(MODEM_LORA,
                    0,      // BW 125 kHz
                    7,      // SF7
                    1,      // CR 4/5
                    0,
                    8,
                    0,
                    false,
                    0,
                    true,
                    0,
                    0,
                    false,
                    true);

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

  // ID que debe coincidir con el transmisor
  strcpy(idDESEADO, "T03");

  Serial.println("Receptor LoRa listo");
  //arrancamos directamente en recepcion
  Radio.Rx(0);
  lora_idle = false;

}

void displayVARIABLES(double num1, double num2) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  display.drawString(0, 0, "Flujo:");
  display.drawString(0, 10, String(num1, 3) + " l/s");

  display.drawString(0, 20, "Consumo:");
  display.drawString(0, 30, String(num2, 3));

  display.drawString(60, 20, "Receptor");
  display.drawString(60, 30, "Antena");

  display.display();
}

void loop() {

  delay(3000);

  // Mostrar valores actuales
  displayVARIABLES(variableFLUJO, variableCONSUMO);

  // Procesar interrupciones LoRa
  Radio.IrqProcess();

  // IMPORTANTE: limpiar buffer para evitar lecturas fantasma
  //rxpacket[0] = '\0';
}

// ======================================================
// CALLBACK DE RECEPCIÓN LORA
// ======================================================
void OnRxDone(uint8_t *payload, uint16_t size, int16_t packetRssi, int8_t snr)
{
  rssi = packetRssi;
  rxSize = size;

  memcpy(rxpacket, payload, size);
  rxpacket[size] = '\0';

  Radio.Sleep();
  lora_idle = true;

  Serial.printf("\nPaquete recibido: \"%s\" RSSI=%d\n", rxpacket, rssi);

  // -----------------------------
  // PARSING DEL PAQUETE
  // -----------------------------
  if (rxpacket[0] != 'T') return;

  char *sep1 = strchr(rxpacket, '|');
  if (!sep1) return;

  char id[10];
  int largoID = sep1 - rxpacket;
  strncpy(id, rxpacket, largoID);
  id[largoID] = '\0';

  if (strcmp(id, idDESEADO) != 0) {
    Serial.println("ID inválido");
    return;
  }

  char valores[30];
  strcpy(valores, sep1 + 1);

  char *sep2 = strchr(valores, 'C');
  if (!sep2) return;

  *sep2 = '\0';

  variableFLUJO = atof(valores);
  variableCONSUMO = atof(sep2 + 1);

  Serial.printf("Flujo: %.3f\n", variableFLUJO);
  Serial.printf("Consumo: %.3f\n", variableCONSUMO);

  enviarPaquete();
}

void OnRxTimeout(void)
{
  Serial.println("RX Timeout, no llegó nada");
  // Igual, pasar a enviar
  enviarPaquete();
}


// ======================================================
// CALLBACKS LORA
// ======================================================
void OnTxDone(void)
{
  Serial.println("TX completado");
  //volvemos a escuchar en el canal de recepcion
  Radio.SetChannel(915000000);

  Radio.Rx(0);
}

void OnTxTimeout(void)
{
  Serial.println("TX Timeout");
  //volvemos a escuchar en el canal de recepcion
  Radio.SetChannel(915000000);
  Radio.Rx(0);
}

void enviarPaquete() {
  Radio.SetChannel(916000000); //para que llege al receptor final
  //el puente corrige antes de enviar
  variableFLUJO = variableFLUJO/3.6;
  variableCONSUMO = variableCONSUMO*1000;
  sprintf(txpacket, "T04|%0.3fC%0.3f", variableFLUJO, variableCONSUMO);
  Serial.printf("\nEnviando paquete: \"%s\"\n", txpacket);
  Radio.Send((uint8_t *)txpacket, strlen(txpacket));
}


