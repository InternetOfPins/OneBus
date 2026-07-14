/**
 * @file uart.h
 * @brief UART protocol component.
 *
 * Board-specific Serial<> aliases are defined in the chip headers:
 *   - chips/avr/avrUart.h for AVR families
 *   - chips/stm32/stm32Uart.h for STM32 families
 *   - chips/esp32/esp32Uart.h for ESP32
 *
 * Usage (AVR):
 *   #include <chips/avr/avrDevice.h>   // sets chip:: alias
 *   using Ser = chip::Serial0<9600>;   // namespace alias from avrUart.h
 *   Ser::begin();
 *   Ser::println("hello");
 */

#pragma once
#include <hapi/hapi.h>
#include <oneBus/busAPI.h>

namespace oneBus {

  /// @brief UART protocol component; begin() initialises the baud rate via the hardware core
  template<uint32_t BaudRate>
  struct Uart {
    template<typename O>
    struct Part : O {
      using Base = O;

      static void begin() {
        Base::uart_init(BaudRate);
        Base::begin();
      }

      static void write(uint8_t c)       { Base::putch(c); }
      static void print(const char* s)   { while (*s) Base::putch(uint8_t(*s++)); }
      static void println(const char* s) { print(s); Base::putch('\r'); Base::putch('\n'); }
    };
  };

} // oneBus
