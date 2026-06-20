/**
 * @file uart.h
 * @author Rui Azevedo (ruihfazevedo@gmail.com)
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

namespace oneBus {

  // Chain terminal — closes the UART chain (mirrors onePin::BootDef).
  struct UartDef {
    UartDef() = delete;
    static void begin() {}
  };

  // Protocol component — sits above the hardware core in the chain.
  // BaudRate is compile-time; hardware core computes UBRR at compile time.
  template<uint32_t BaudRate>
  struct Uart {
    template<typename O>
    struct Part : O {
      using Base = O;
      using Base::Base;

      static void begin() {
        Base::uart_init(BaudRate);
        Base::begin();
      }

      static void    putch(uint8_t c) { Base::uart_putch(c); }
      static uint8_t getch()          { return Base::uart_getch(); }
      static bool    available()      { return Base::uart_available(); }

      static void write(uint8_t c)        { Base::uart_putch(c); }
      static void print(const char* s)    { while (*s) putch((uint8_t)*s++); }
      static void println(const char* s)  { print(s); putch('\r'); putch('\n'); }
    };
  };

} // oneBus

// ============================================================
// Chip-family Serial aliases — wires Uart<> with hardware core.
// chip::Serial0<9600> resolves via the namespace alias set by
// the chip device header (avrDevice.h / stm32Device.h).
// ============================================================

#if defined(__AVR__)
  #include <chips/avr/avrUart.h>
  namespace hw::avr {

    namespace mega {
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial0 = hapi::APIOf<oneBus::UartDef, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0xC0u, CpuHz>>;
    }

    namespace mega2560 {
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial0 = hapi::APIOf<oneBus::UartDef, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0xC0u, CpuHz>>;
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial1 = hapi::APIOf<oneBus::UartDef, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0xC8u, CpuHz>>;
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial2 = hapi::APIOf<oneBus::UartDef, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0xD0u, CpuHz>>;
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial3 = hapi::APIOf<oneBus::UartDef, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0x130u, CpuHz>>;
    }

    namespace mega1284 {
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial0 = hapi::APIOf<oneBus::UartDef, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0xC0u, CpuHz>>;
      template<uint32_t BaudRate, uint32_t CpuHz = 16000000UL>
      using Serial1 = hapi::APIOf<oneBus::UartDef, oneBus::Uart<BaudRate>,
                                  AvrUsartCore<0xC8u, CpuHz>>;
    }

  } // hw::avr
  // chip::Serial0<> resolves via the chip namespace alias (mega/mega2560/mega1284)

#elif defined(__arm__)
  #include <chips/stm32/stm32Uart.h>
  namespace hw::stm32 {

    namespace f1 {
      // USART1: PA9 TX / PA10 RX  (default on Blue Pill)
      template<uint32_t BaudRate, uint32_t CpuHz = 72000000UL>
      using Serial0 = hapi::APIOf<oneBus::UartDef, oneBus::Uart<BaudRate>,
                                  Stm32UsartCore<0x40013800u, Stm32F1_Usart1_PA9_PA10, CpuHz>>;
      // USART2: PA2 TX / PA3 RX
      template<uint32_t BaudRate, uint32_t CpuHz = 72000000UL>
      using Serial1 = hapi::APIOf<oneBus::UartDef, oneBus::Uart<BaudRate>,
                                  Stm32UsartCore<0x40004400u, Stm32F1_Usart2_PA2_PA3, CpuHz>>;
      // USART3: PB10 TX / PB11 RX
      template<uint32_t BaudRate, uint32_t CpuHz = 72000000UL>
      using Serial2 = hapi::APIOf<oneBus::UartDef, oneBus::Uart<BaudRate>,
                                  Stm32UsartCore<0x40004800u, Stm32F1_Usart3_PB10_PB11, CpuHz>>;
    }

    namespace f4 {
      // USART1: PA9 TX / PA10 RX
      template<uint32_t BaudRate, uint32_t CpuHz = 168000000UL>
      using Serial0 = hapi::APIOf<oneBus::UartDef, oneBus::Uart<BaudRate>,
                                  Stm32UsartCore<0x40011000u, Stm32F4_Usart1_PA9_PA10, CpuHz>>;
      // USART2: PA2 TX / PA3 RX
      template<uint32_t BaudRate, uint32_t CpuHz = 168000000UL>
      using Serial1 = hapi::APIOf<oneBus::UartDef, oneBus::Uart<BaudRate>,
                                  Stm32UsartCore<0x40004400u, Stm32F4_Usart2_PA2_PA3, CpuHz>>;
    }

  } // hw::stm32
  // chip::Serial0<> resolves via the chip namespace alias (f1/f4)
#endif
