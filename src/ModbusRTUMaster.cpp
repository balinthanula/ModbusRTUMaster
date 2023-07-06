#include "ModbusRTUMaster.h"

ModbusRTUMaster::ModbusRTUMaster(HardwareSerial& serial, uint8_t dePin) {
  _hardwareSerial = &serial;
  #ifdef __AVR__
  _softwareSerial = 0;
  #endif
  #ifdef HAVE_CDCSERIAL
  _usbSerial = 0;
  #endif
  _serial = &serial;
  _dePin = dePin;
}

#ifdef __AVR__
ModbusRTUMaster::ModbusRTUMaster(SoftwareSerial& serial, uint8_t dePin) {
  _hardwareSerial = 0;
  _softwareSerial = &serial;
  #ifdef HAVE_CDCSERIAL
  _usbSerial = 0;
  #endif
  _serial = &serial;
  _dePin = dePin;
}
#endif

#ifdef HAVE_CDCSERIAL
ModbusRTUMaster::ModbusRTUMaster(Serial_& serial, uint8_t dePin) {
  _hardwareSerial = 0;
  #ifdef __AVR__
  _softwareSerial = 0;
  #endif
  _usbSerial = &serial;
  _serial = &serial;
  _dePin = dePin;
}
#endif

void ModbusRTUMaster::setTimeout(uint32_t timeout) {
  _responseTimeout = timeout;
}

void ModbusRTUMaster::begin(unsigned long baud, uint8_t config) {
  if (_hardwareSerial) {
    _calculateTimeouts(baud, config);
    _hardwareSerial->begin(baud, config);
  }
  #ifdef __AVR__
  else if (_softwareSerial) {
    _calculateTimeouts(baud, SERIAL_8N1);
    _softwareSerial->begin(baud);
  }
  #endif
  #ifdef HAVE_CDCSERIAL
  else if (_usbSerial) {
    _calculateTimeouts(baud, config);
    _usbSerial->begin(baud, config);
    while (!_usbSerial);
  }
  #endif
  if (_dePin != NO_DE_PIN) {
    pinMode(_dePin, OUTPUT);
    digitalWrite(_dePin, LOW);
  }
  _clearRxBuffer();
}



bool ModbusRTUMaster::readCoils(uint8_t id, uint16_t startAddress, bool *buf, uint16_t quantity) {
  const uint8_t functionCode = 1;
  uint8_t byteCount = _div8RndUp(quantity);
  if (id < 1 || id > 247 || !buf || quantity == 0 || quantity > 2000) return false;
  _buf[0] = id;
  _buf[1] = functionCode;
  _buf[2] = highByte(startAddress);
  _buf[3] = lowByte(startAddress);
  _buf[4] = highByte(quantity);
  _buf[5] = lowByte(quantity);
  _writeRequest(6);
  uint16_t responseLength = _readResponse(id, functionCode);
  if (responseLength != (3 + byteCount) || _buf[2] != byteCount) return false;
  else {
    for (uint16_t i = 0; i < quantity; i++) {
      buf[i] = bitRead(_buf[3 + (i >> 3)], i & 7);
    }
    return quantity;
  }
  return true;
}

bool ModbusRTUMaster::readDiscreteInputs(uint8_t id, uint16_t startAddress, bool *buf, uint16_t quantity) {
  const uint8_t functionCode = 2;
  uint8_t byteCount = _div8RndUp(quantity);
  if (id < 1 || id > 247 || !buf || quantity == 0 || quantity > 2000) return false;
  _buf[0] = id;
  _buf[1] = functionCode;
  _buf[2] = highByte(startAddress);
  _buf[3] = lowByte(startAddress);
  _buf[4] = highByte(quantity);
  _buf[5] = lowByte(quantity);
  _writeRequest(6);
  uint16_t responseLength = _readResponse(id, functionCode);
  if (responseLength != (3 + byteCount) || _buf[2] != byteCount) return false;
  else {
    for (uint16_t i = 0; i < quantity; i++) {
      buf[i] = bitRead(_buf[3 + (i >> 3)], i & 7);
    }
    return quantity;
  }
  return true;
}

bool ModbusRTUMaster::readHoldingRegisters(uint8_t id, uint16_t startAddress, uint16_t *buf, uint16_t quantity) {
  const uint8_t functionCode = 3;
  uint8_t byteCount = quantity * 2;
  if (id < 1 || id > 247 || !buf || quantity == 0 || quantity > 125) return false;
  _buf[0] = id;
  _buf[1] = functionCode;
  _buf[2] = highByte(startAddress);
  _buf[3] = lowByte(startAddress);
  _buf[4] = highByte(quantity);
  _buf[5] = lowByte(quantity);
  _writeRequest(6);
  uint16_t responseLength = _readResponse(id, functionCode);
  if (responseLength != (3 + byteCount) || _buf[2] != byteCount) return false;
  else {
    for (uint16_t i = 0; i < quantity; i++) {
      buf[i] = _bytesToWord(_buf[3 + (i * 2)], _buf[4 + (i * 2)]);
    }
    return true;
  }
}

bool ModbusRTUMaster::readInputRegisters(uint8_t id, uint16_t startAddress, uint16_t *buf, uint16_t quantity) {
  const uint8_t functionCode = 4;
  uint8_t byteCount = quantity * 2;
  if (id < 1 || id > 247 || !buf || quantity == 0 || quantity > 125) return false;
  _buf[0] = id;
  _buf[1] = functionCode;
  _buf[2] = highByte(startAddress);
  _buf[3] = lowByte(startAddress);
  _buf[4] = highByte(quantity);
  _buf[5] = lowByte(quantity);
  _writeRequest(6);
  uint16_t responseLength = _readResponse(id, functionCode);
  if (responseLength != (3 + byteCount) || _buf[2] != byteCount) return false;
  else {
    for (uint16_t i = 0; i < quantity; i++) {
      buf[i] = _bytesToWord(_buf[3 + (i * 2)], _buf[4 + (i * 2)]);
    }
    return true;
  }
}

bool ModbusRTUMaster::writeSingleCoil(uint8_t id, uint16_t address, bool value) {
  const uint8_t functionCode = 5;
  if (id > 247) return false;
  _buf[0] = id;
  _buf[1] = functionCode;
  _buf[2] = highByte(address);
  _buf[3] = lowByte(address);
  _buf[4] = value * 255;
  _buf[5] = 0;
  _writeRequest(6);
  if (id == 0) return true;
  uint16_t responseLength = _readResponse(id, functionCode);
  if (responseLength != 6 || _bytesToWord(_buf[2], _buf[3]) != address || _buf[4] != (value * 255) || _buf[5] != 0) return false;
  else return true;
}

bool ModbusRTUMaster::writeSingleHoldingRegister(uint8_t id, uint16_t address, uint16_t value) {
  const uint8_t functionCode = 6;
  if (id > 247) return false;
  _buf[0] = id;
  _buf[1] = functionCode;
  _buf[2] = highByte(address);
  _buf[3] = lowByte(address);
  _buf[4] = highByte(value);
  _buf[5] = lowByte(value);
  _writeRequest(6);
  if (id == 0) return true;
  uint16_t responseLength = _readResponse(id, functionCode);
  if (responseLength != 6 || _bytesToWord(_buf[2], _buf[3]) != address || _bytesToWord(_buf[4], _buf[5]) != value) return false;
  else return true;
}

bool ModbusRTUMaster::writeMultipleCoils(uint8_t id, uint16_t startAddress, bool *buf, uint16_t quantity) {
  const uint8_t functionCode = 15;
  uint8_t byteCount = _div8RndUp(quantity);
  if (id > 247 || !buf || quantity == 0 || quantity > 1968) return false;
  _buf[0] = id;
  _buf[1] = functionCode;
  _buf[2] = highByte(startAddress);
  _buf[3] = lowByte(startAddress);
  _buf[4] = highByte(quantity);
  _buf[5] = lowByte(quantity);
  _buf[6] = byteCount;
  for (uint16_t i = 0; i < quantity; i++) {
    bitWrite(_buf[7 + (i >> 3)], i & 7, buf[i]);
  }
  for (uint16_t i = quantity; i < (byteCount * 8); i++) {
    bitClear(_buf[7 + (i >> 3)], i & 7);
  }
  _writeRequest(7 + byteCount);
  if (id == 0) return true;
  uint16_t responseLength = _readResponse(id, functionCode);
  if (responseLength != 6 || _bytesToWord(_buf[2], _buf[3]) != startAddress || _bytesToWord(_buf[4], _buf[5]) != quantity) return false;
  else return true;
}

bool ModbusRTUMaster::writeMultipleHoldingRegisters(uint8_t id, uint16_t startAddress, uint16_t *buf, uint16_t quantity) {
  const uint8_t functionCode = 16;
  uint8_t byteCount = quantity * 2;
  if (id > 247 || !buf || quantity == 0 || quantity > 123) return false;
  _buf[0] = id;
  _buf[1] = functionCode;
  _buf[2] = highByte(startAddress);
  _buf[3] = lowByte(startAddress);
  _buf[4] = highByte(quantity);
  _buf[5] = lowByte(quantity);
  _buf[6] = byteCount;
  for (uint16_t i = 0; i < quantity; i++) {
    _buf[7 + (i * 2)] = highByte(_buf[i]);
    _buf[8 + (i * 2)] = lowByte(_buf[i]);
  }
  _writeRequest(7 + byteCount);
  uint16_t responseLength = _readResponse(id, functionCode);
  if (responseLength != 6 || _bytesToWord(_buf[2], _buf[3]) != startAddress || _bytesToWord(_buf[4], _buf[5]) != quantity) return false;
  else return true;
}

uint8_t ModbusRTUMaster::getExceptionCode() {
  return _exceptionCode;
}


void ModbusRTUMaster::_setTimeouts(unsigned long baud, uint8_t config) {
  if (baud > 19200) {
    _charTimeout = 750;
    _frameTimeout = 1750;
  }
  else if (_hardwareSerial) {
    if (config == 0x2E || config == 0x3E) {
      _charTimeout = 18000000/baud;
      _frameTimeout = 42000000/baud;
    }
    else if (config == 0x0E || config == 0x26 || config == 0x36) {
      _charTimeout = 16500000/baud;
      _frameTimeout = 38500000/baud;
    }
  }
  else {
    _charTimeout = 15000000/baud;
    _frameTimeout = 35000000/baud;
  }
}

void ModbusRTUMaster::_clearRxBuf() {
  unsigned long startTime = micros();
  do {
    if (_serial->available() > 0) {
      startTime = micros();
      _serial->read();
    }
  } while (micros() - startTime < _frameTimeout);
}


void ModbusRTUMaster::_transmit(size_t len) {
  uint16_t crc = _crc(len);
  _buf[len] = lowByte(crc);
  _buf[len + 1] = highByte(crc);
  if (_dePin != NO_DE_PIN) digitalWrite(_dePin, HIGH);
  _serial->write(_buf, len + 2);
  _serial->flush();
  if (_dePin != NO_DE_PIN) digitalWrite(_dePin, LOW);
  _exceptionCode = 0;
}

size_t ModbusRTUMaster::_receive() {
  size_t i = 0;
  unsigned long startTime = 0;
  do {
    if (_serial->available() > 0) {
      startTime = micros();
      _buf[i] = _serial->read();
      i++;
    }
  } while (micros() - startTime < _charTimeout && i < MODBUS_RTU_MASTER_BUF_SIZE);
  while (micros() - startTime < _frameTimeout);
  if (_serial->available() == 0 && _crc(i - 2) == _bytesToWord(_buf[i - 1], _buf[i - 2])) return i - 2;
  else return 0;
}

uint16_t ModbusRTUMaster::_crc(size_t len) {
  uint16_t value = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    value ^= (uint16_t)_buf[i];
    for (uint8_t j = 0; j < 8; j++) {
      bool lsb = value & 1;
      value >>= 1;
      if (lsb == true) value ^= 0xA001;
    }
  }
  return value;
}

uint16_t ModbusRTUMaster::_div8RndUp(uint16_t value) {
  return (value + 7) >> 3;
}

uint16_t ModbusRTUMaster::_bytesToWord(uint8_t high, uint8_t low) {
  return (high << 8) | low;
}