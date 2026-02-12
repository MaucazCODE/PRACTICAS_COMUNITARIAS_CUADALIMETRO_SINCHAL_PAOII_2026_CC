from time import strftime, sleep
from datetime import date
from typing import Any
from digitalio import DigitalInOut
import requests
import adafruit_rfm9x
import busio
import board
import os

CS = DigitalInOut(board.CE1)
RESET = DigitalInOut(board.D25)
spi = busio.SPI(board.SCK, MOSI = board.MOSI, MISO = board.MISO)
rfm9x = adafruit_rfm9x.RFM9x(spi, CS, RESET, 915.0)

ARCHIVO_DIR: str = os.path.expanduser('~')
LORA_TIMEOUT: int = 15
HTTP_TIMEOUT: int = 15

#Para el primer arduino
LINK_ARD1_1: str = "https://api.thingspeak.com/update?api_key=CPVEVA5ND36992WC&field1=0"
LINK_ARD1_2: str = "https://api.thingspeak.com/update?api_key=CPVEVA5ND36992WC&field2=0"

#Para el segundo arduino TODO: cambiarlo
LINK_ARD2_1: str = "https://api.thingspeak.com/update?api_key=CPVEVA5ND36992WC&field1=0"
LINK_ARD2_2: str = "https://api.thingspeak.com/update?api_key=CPVEVA5ND36992WC&field2=0"

def receivePacket() -> str:
  packet: Any = rfm9x.receive(with_ack = True, timeout = LORA_TIMEOUT)
  if packet is not None:
    packet_text: str = str(packet, "utf-8")
    return packet_text

def saveToFile(dir: str, mode: str, data: str) -> None:
  with open(dir, mode) as file:
    date: str = format(strftime("%Y-%m-%d %H:%M:%S"))
    toWrite: str = data + "," + date + "\n"
    file.write(toWrite)
    file.flush()

def setLinks(sentFrom: str) -> Any:
  if (sentFrom == "ARD-1"):
    link1: str = LINK_ARD1_1
    link2: str = LINK_ARD1_2
  elif (sentFrom == "ARD-2"):
    link1: str = LINK_ARD2_1
    link2: str = LINK_ARD2_2
  return link1, link2

def sendToThingspeak(message: str, sentFrom: str) -> None:
  flujo, caudal = message.split(",")
  link1, link2 = setLinks(sentFrom)
  try:
    sleep(15)
    requests.get(link1 + flujo, timeout = HTTP_TIMEOUT)
    sleep(15)
    requests.get(link2 + caudal, timeout = HTTP_TIMEOUT)
  except requests.exceptions.HTTPError as e:
    print("[" + format(strftime("%Y-%m-%d %H:%M:%S")) + "] " +
    "Error en el request: " + str(e) + " [FAIL]")

def main():
  alternar: str = "ARD-1"
  textoReceived: str = None
  while True:
    rfm9x.send(bytes(alternar, "utf-8"))
    textoReceived = receivePacket()
    if (not textoReceived):
      continue
    sendToThingspeak(textoReceived, alternar)
    saveToFile(ARCHIVO_DIR + "/tabla" + date.today().strftime("%Y-%m-%d") + ".csv", "a", textoReceived)
    print("[" + format(strftime("%Y-%m-%d %H:%M:%S")) + "] " +
    "Dato de " + alternar + ": " + textoReceived + " [SUCCESS]")
    if (alternar == "ARD-1"):
      alternar = "ARD-2"
    elif (alternar == "ARD-2"):
      alternar = "ARD-1"
    textoReceived = None
    sleep(1)

if __name__ == "__main__":
  try:
    main()
  except KeyboardInterrupt:
    print("[" + format(strftime("%Y-%m-%d %H:%M:%S")) + "] " +
    "KeyboardInterrupt [CLOSED]")
