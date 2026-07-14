/**
 * @file spi.h
 * @brief SPI protocol component.
 *
 * Board-specific Spi<> aliases are defined in the chip headers:
 *   - chips/avr/avrSpi.h for AVR families
 *   - chips/stm32/stm32Spi.h for STM32 families
 *
 * Usage (AVR):
 *   #include <chips/avr/avrDevice.h>
 *   using Bus = chip::Spi<4000000>;  // namespace alias from avrSpi.h
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
