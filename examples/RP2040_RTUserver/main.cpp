// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#include <Arduino.h>
#include "ModbusServerRTU.h"

// Adjust these pins to your RP2040 board wiring.
constexpr uint8_t UART_TX_PIN = 0;
constexpr uint8_t UART_RX_PIN = 1;
constexpr uint8_t RS485_REDE_PIN = 2;
constexpr uint32_t MODBUS_BAUDRATE = 9600;
constexpr uint8_t MODBUS_SERVER_ID = 1;

constexpr uint16_t REGISTER_COUNT = 4;
uint16_t holdingRegisters[REGISTER_COUNT] = {100, 200, 0, 0};

// Create RTU server with a 2000ms timeout and explicit RE/DE control pin.
ModbusServerRTU MBserver(2000, RS485_REDE_PIN);

ModbusMessage FC03(ModbusMessage request) {
  uint16_t address = 0;
  uint16_t words = 0;
  ModbusMessage response;

  request.get(2, address);
  request.get(4, words);

  if (words == 0 || (address + words) > REGISTER_COUNT) {
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    return response;
  }

  response.add(request.getServerID(), request.getFunctionCode(), static_cast<uint8_t>(words * 2));
  for (uint16_t i = address; i < address + words; ++i) {
    response.add(holdingRegisters[i]);
  }

  return response;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    delay(10);
  }

#if defined(ARDUINO_ARCH_RP2040)
  Serial1.setTX(UART_TX_PIN);
  Serial1.setRX(UART_RX_PIN);
#endif

  RTUutils::prepareHardwareSerial(Serial1);
  Serial1.begin(MODBUS_BAUDRATE, SERIAL_8N1);

  MBserver.registerWorker(MODBUS_SERVER_ID, READ_HOLD_REGISTER, &FC03);
  MBserver.begin(Serial1, MODBUS_BAUDRATE);

  Serial.println("RP2040 RTU server started");
}

void loop() {
  // Update some demo values that a client can read.
  holdingRegisters[2] = static_cast<uint16_t>(millis() / 1000UL);
  holdingRegisters[3]++;
  delay(1000);
}
