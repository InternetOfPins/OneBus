/**
 * @file spi.h
 * @brief SPI protocol component + chip-family Spi<> aliases.
 *
 * Usage (AVR):
 *   #include <chips/avr/avrDevice.h>
 *   #include <oneBus/spi.h>
 *   using Bus = chip::Spi<4000000>;
 *   Bus::begin();
 *   uint8_t r = Bus::transfer(0xAB);
 *   Bus::send(buf, len);
 *
 * For per-device chip-select, add ChipSelect<> above SpiMaster<>:
 *   using Dev = hapi::APIOf<SpiAPI, ChipSelect<0x25, PB0>,   // PORTB=0x25 on ATmega328
 *                           SpiMaster<4000000>, AvrSpiCore<16000000>>;
 *   Dev::select();   Dev::send(buf, n);   Dev::deselect();
 */

#pragma once
#include <hapi/hapi.h>
#include <oneBus/busAPI.h>

namespace oneBus {

  /// @brief SPI master protocol component; transfer/send/fill over the hardware core
  template<uint32_t Speed = 4000000UL>
  struct SpiMaster {
    template<typename O>
    struct Part : O {
      using Base = O;

      static void begin() {
        Base::spi_init(Speed);
        Base::begin();
      }

      static uint8_t transfer(uint8_t b)                  { return Base::spi_transfer(b); }
      static void    send(const uint8_t* buf, uint16_t n)  { while (n--) Base::spi_transfer(*buf++); }
      static void    fill(uint8_t b, uint16_t n)            { while (n--) Base::spi_transfer(b); }
    };
  };

  /// @brief SPI chip-select layer: asserts CS low on begin, deasserts on end
  template<uintptr_t CsPortAddr, uint8_t CsBit>
  struct ChipSelect {
    template<typename O>
    struct Part : O {
      using Base = O;

      static void begin() {
        volatile uint8_t& ddr  = *reinterpret_cast<volatile uint8_t*>(CsPortAddr - 1);
        volatile uint8_t& port = *reinterpret_cast<volatile uint8_t*>(CsPortAddr);
        ddr  |= uint8_t(1u << CsBit);
        port |= uint8_t(1u << CsBit);   // CS idle high
        Base::begin();
      }

      static void select()   { *reinterpret_cast<volatile uint8_t*>(CsPortAddr) &= ~uint8_t(1u << CsBit); }
      static void deselect() { *reinterpret_cast<volatile uint8_t*>(CsPortAddr) |=  uint8_t(1u << CsBit); }

      static uint8_t transfer(uint8_t b)                  { return Base::transfer(b); }
      static void    send(const uint8_t* buf, uint16_t n)  { Base::send(buf, n); }
      static void    fill(uint8_t b, uint16_t n)            { Base::fill(b, n); }
    };
  };

} // oneBus

// ── Chip-family Spi<> aliases ─────────────────────────────────────────────────
// chip::Spi<4000000> resolves via the namespace alias set in the chip header.

#if defined(__AVR__)
  #include <chips/avr/avrSpi.h>
  namespace hw::avr {

    namespace mega {
      // Speed: SCK frequency in Hz, Mode: 0-3 (CPOL/CPHA), MSBFirst: bit order
      template<uint32_t Speed = 4000000UL, uint8_t Mode = 0,
               bool MSBFirst = true, uint32_t CpuHz = 16000000UL>
      using Spi = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>,
                              AvrSpiCore<CpuHz, Mode, MSBFirst>>;
    }

    namespace mega2560 {
      template<uint32_t Speed = 4000000UL, uint8_t Mode = 0,
               bool MSBFirst = true, uint32_t CpuHz = 16000000UL>
      using Spi = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>,
                              AvrSpiCore<CpuHz, Mode, MSBFirst>>;
    }

    namespace mega1284 {
      template<uint32_t Speed = 4000000UL, uint8_t Mode = 0,
               bool MSBFirst = true, uint32_t CpuHz = 16000000UL>
      using Spi = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>,
                              AvrSpiCore<CpuHz, Mode, MSBFirst>>;
    }

  } // hw::avr

#elif defined(__arm__)
  #include <chips/stm32/stm32Spi.h>
  namespace hw::stm32 {

    namespace f1 {
      template<uint32_t Speed = 4000000UL, uint8_t Mode = 0,
               bool MSBFirst = true, uint32_t ApbHz = 72000000UL>
      using Spi_ = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>,
                               Stm32SpiCore<0x40013000u, Stm32F1_Spi1_PA5_PA6_PA7,
                                            ApbHz, Mode, MSBFirst>>;
    }

    namespace f4 {
      template<uint32_t Speed = 4000000UL, uint8_t Mode = 0,
               bool MSBFirst = true, uint32_t ApbHz = 84000000UL>
      using Spi_ = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>,
                               Stm32SpiCore<0x40013000u, Stm32F4_Spi1_PA5_PA6_PA7,
                                            ApbHz, Mode, MSBFirst>>;
    }

    namespace f0 {
      // ApbHz default 8MHz matches f0::SysClk's HSI-reset default; override for
      // framework=arduino builds, whose actual running clock differs (see
      // feedback_stm32_usart_v2 in project memory — f0::SysClk doesn't configure
      // the PLL, so under framework=arduino the true clock is whatever the vendor
      // startup set, commonly 48MHz).
      template<uint32_t Speed = 4000000UL, uint8_t Mode = 0,
               bool MSBFirst = true, uint32_t ApbHz = 8000000UL>
      using Spi_ = hapi::APIOf<oneBus::SpiAPI, oneBus::SpiMaster<Speed>,
                               Stm32SpiCore<0x40013000u, Stm32F0_Spi1_PA5_PA6_PA7,
                                            ApbHz, Mode, MSBFirst>>;
    }

  } // hw::stm32
#endif
