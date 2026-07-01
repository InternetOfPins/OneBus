/**
 * @file arduinoI2C.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
 * @brief ArduinoWire<wire, sda, scl> — TwiMaster adapter for the Arduino Wire library.
 *
 * Wraps any TwoWire instance (Wire, Wire1, …) as a static-method TwiMaster
 * satisfying oneBus::is_twi_master<>.
 *
 * Auto-detects Wire::begin(sda, scl) (ESP32/RP2040) vs Wire::begin() (AVR/AVR-DA)
 * via SFINAE so the same template works across all Arduino targets.
 *
 * Usage:
 *   #include <oneBus/arduinoI2C.h>
 *   using I2c = oneBus::ArduinoWire<Wire>;          // AVR — default pins
 *   using I2c = oneBus::ArduinoWire<Wire, 21, 22>;  // ESP32 SDA=21 SCL=22
 *   AHT<I2c> sensor; sensor.begin(); sensor.read();
 */

#pragma once
#ifdef ARDUINO
#include <Wire.h>
#include <stdint.h>
#ifdef __AVR__
  #include <hapi/platform/avr/avr_std.h>  // no <type_traits> on AVR, even under Arduino
#else
  #include <type_traits>
#endif

namespace oneBus {

  /// @brief Arduino Wire TwiMaster adapter; SFINAE-detects begin(sda,scl) vs begin()
  template<TwoWire& wire, int sda = -1, int scl = -1>
  struct ArduinoWire {
    static void    begin()                                { _begin(wire); }
    static void    begin_write(uint8_t addr)              { wire.beginTransmission(addr); }
    static void    write_byte(uint8_t b)                  { wire.write(b); }
    static void    end_write()                            { wire.endTransmission(); }
    static uint8_t request_from(uint8_t addr, uint8_t n) {
      return (uint8_t)wire.requestFrom(addr, (uint8_t)n);
    }
    static uint8_t read_byte()                            { return (uint8_t)wire.read(); }

  private:
    template<typename W, typename = void>
    struct _HasPinBegin : std::false_type {};
    template<typename W>
    struct _HasPinBegin<W, std::void_t<decltype(std::declval<W&>().begin(0, 0))>>
      : std::true_type {};

    template<typename W>
    static void _begin(W& w) {
      if constexpr (sda >= 0 && _HasPinBegin<W>::value)
        w.begin(sda, scl);
      else
        w.begin();
    }
  };

} // oneBus
#endif // ARDUINO
