/**
 * @file oneWire.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief 1-Wire bus: protocol component + Arduino bit-bang core.
 */

#pragma once
#include <hapi/hapi.h>
#include <oneBus/busAPI.h>
#include <stdint.h>

namespace oneBus {

  // 1-Wire bus terminal — primitives a hardware core must implement.
  struct OneWireAPI : BusAPI {
    OneWireAPI() = delete;
    static bool   ow_reset()            = delete;  // reset pulse; returns true if device present
    static void   ow_write_bit(bool)    = delete;
    static bool   ow_read_bit()         = delete;
  };

  // 1-Wire protocol layer — sits on top of a OneWireCore.
  // Provides reset/writeByte/readByte used by sensor drivers.
  /// @brief 1-Wire protocol component; writeByte/readByte/skip over ow_reset/ow_write_bit/ow_read_bit
  struct OneWireMaster {
    template<typename O>
    struct Part : O {
      using Base = O;

      static void begin() { Base::begin(); }
      static bool reset() { return Base::ow_reset(); }

      static void writeByte(uint8_t b) {
        for (uint8_t i = 0; i < 8; ++i) { Base::ow_write_bit(b & 1); b >>= 1; }
      }
      static uint8_t readByte() {
        uint8_t b = 0;
        for (uint8_t i = 0; i < 8; ++i) {
          if (Base::ow_read_bit()) b |= uint8_t(1u << i);
        }
        return b;
      }
      // Skip ROM — address the single device on the bus without reading its 64-bit ID
      static void skip() { writeByte(0xCC); }
    };
  };

} // oneBus

// ── Arduino bit-bang core ─────────────────────────────────────────────────────
#ifdef ARDUINO
#include <Arduino.h>

namespace oneBus {

  // Bit-bang 1-Wire for any Arduino-compatible target (AVR, ESP32, …).
  // PinN: Arduino GPIO number; requires INPUT_PULLUP and OUTPUT capability.
  // noInterrupts()/interrupts() guard each slot for timing accuracy on AVR.
  // On ESP32, noInterrupts() maps to portDISABLE_INTERRUPTS (same effect).
  /// @brief Arduino bit-bang 1-Wire core; HAPI component providing ow_reset/ow_write_bit/ow_read_bit
  template<int PinN>
  struct ArduinoOneWireCore {
    template<typename O>
    struct Part : O {
      static void begin() { pinMode(PinN, INPUT_PULLUP); O::begin(); }

      static bool ow_reset() {
        noInterrupts();
        pinMode(PinN, OUTPUT);
        digitalWrite(PinN, LOW);
        interrupts();
        delayMicroseconds(480);
        noInterrupts();
        pinMode(PinN, INPUT_PULLUP);
        delayMicroseconds(70);
        bool present = !digitalRead(PinN);
        interrupts();
        delayMicroseconds(410);
        return present;
      }

      static void ow_write_bit(bool bit) {
        noInterrupts();
        pinMode(PinN, OUTPUT);
        if (bit) {
          digitalWrite(PinN, LOW);
          delayMicroseconds(6);
          pinMode(PinN, INPUT_PULLUP);
          interrupts();
          delayMicroseconds(64);
        } else {
          digitalWrite(PinN, LOW);
          interrupts();
          delayMicroseconds(60);
          noInterrupts();
          pinMode(PinN, INPUT_PULLUP);
          interrupts();
          delayMicroseconds(10);
        }
      }

      static bool ow_read_bit() {
        noInterrupts();
        pinMode(PinN, OUTPUT);
        digitalWrite(PinN, LOW);
        delayMicroseconds(6);
        pinMode(PinN, INPUT_PULLUP);
        delayMicroseconds(9);
        bool bit = digitalRead(PinN);
        interrupts();
        delayMicroseconds(55);
        return bit;
      }
    };
  };

  // Ready-to-use assembled 1-Wire bus for Arduino targets.
  // Usage: using MyBus = oneBus::OneWire<2>;  // Arduino pin 2
  template<int PinN>
  using OneWire = hapi::APIOf<OneWireAPI, OneWireMaster, ArduinoOneWireCore<PinN>>;

} // oneBus

#endif // ARDUINO
