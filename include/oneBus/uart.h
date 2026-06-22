/**
 * @file uart.h
 * @brief UART protocol component + chip-family Serial aliases.
 *
 * Usage (AVR):
 *   #include <chips/avr/avrDevice.h>   // sets chip:: alias
 *   #include <oneBus/uart.h>            // adds chip::Serial0<>
 *   using Ser = chip::Serial0<9600>;
 *   Ser::begin();
 *   Ser::println("hello");
 */

#pragma once
#include <hapi/hapi.h>
#include <oneBus/busAPI.h>

namespace oneBus {

  // Protocol component — sits above the hardware core in the chain.
  // putch/getch/available come from the core (via this->), not wrapped here.
  // Only begin() reaches Base:: — for the uart_init private init primitive.
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

// ── Chip-family Serial aliases ────────────────────────────────────────────────
// chip::Serial0<9600> resolves via the namespace alias set in the chip header.

#if defined(__AVR__)
  #include <chips/avr/avrUart.h>
  namespace hw::avr {

    namespace mega {
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial0 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0xC0u, CpuHz>>;
    }

    namespace mega2560 {
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial0 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0xC0u, CpuHz>>;
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial1 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0xC8u, CpuHz>>;
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial2 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0xD0u, CpuHz>>;
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial3 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0x130u, CpuHz>>;
    }

    namespace mega1284 {
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial0 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0xC0u, CpuHz>>;
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial1 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0xC8u, CpuHz>>;
    }

  } // hw::avr

#elif defined(__arm__)
  #include <chips/stm32/stm32Uart.h>
  namespace hw::stm32 {

    namespace f1 {
      template<uint32_t BaudRate, uint32_t CpuHz = 72000000UL>
      using Serial0 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                  Stm32UsartCore<0x40013800u, Stm32F1_Usart1_PA9_PA10, CpuHz>>;
      template<uint32_t BaudRate, uint32_t CpuHz = 72000000UL>
      using Serial1 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                  Stm32UsartCore<0x40004400u, Stm32F1_Usart2_PA2_PA3, CpuHz>>;
      template<uint32_t BaudRate, uint32_t CpuHz = 72000000UL>
      using Serial2 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                  Stm32UsartCore<0x40004800u, Stm32F1_Usart3_PB10_PB11, CpuHz>>;
    }

    namespace f4 {
      template<uint32_t BaudRate, uint32_t CpuHz = 168000000UL>
      using Serial0 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                  Stm32UsartCore<0x40011000u, Stm32F4_Usart1_PA9_PA10, CpuHz>>;
      template<uint32_t BaudRate, uint32_t CpuHz = 168000000UL>
      using Serial1 = hapi::APIOf<oneBus::UartAPI, oneBus::Uart<BaudRate>,
                                  Stm32UsartCore<0x40004400u, Stm32F4_Usart2_PA2_PA3, CpuHz>>;
    }

  } // hw::stm32
#endif
