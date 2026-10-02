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
 * For per-device chip-select, add CsPin<> (any pin type with begin/on/off) above SpiMaster<>:
 *   using Dev = hapi::APIOf<SpiAPI, CsPin<MyCsPin>, SpiMaster<4000000>, AvrSpiCore<16000000>>;
 *   Dev::select();   Dev::send(buf, n);   Dev::deselect();
 * ChipSelect<PortAddr, Bit> is the older AVR-only form of the same layer (a raw PORTx address).
 *
 * For a shared bus whose devices are found at runtime, SpiSlots<> declares the chip selects statically, one slot each;
 * which device (if any) sits behind a slot is the runtime question:
 *   using Bus = hapi::APIOf<SpiAPI, SpiSlots<CsA, CsB, CsC>, SpiMaster<4000000>, Core>;
 *   Bus::setup(1000000, 0);                       // a device's own clock and mode, before talking to it
 *   uint8_t tx[2] = {0xD0, 0}, rx[2];
 *   Bus::xfer(1, tx, rx, 2);                      // slot 1 selected for the whole exchange
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

      [[nodiscard]] static uint8_t transfer(uint8_t b)                  { return Base::spi_transfer(b); }
      static void    send(const uint8_t* buf, uint16_t n)  { while (n--) Base::spi_transfer(*buf++); }
      static void    fill(uint8_t b, uint16_t n)            { while (n--) Base::spi_transfer(b); }

      // a device's own clock and mode (0-3), set before it is selected. A core that cannot change its mode at runtime
      // (no spi_setup: the AVR core takes the mode as a template parameter) gets only the clock.
      static void setup(uint32_t hz, uint8_t mode) {
        if constexpr (has_spi_setup<Base>::value) Base::spi_setup(hz, mode);
        else { (void)mode; Base::spi_init(hz); }
      }

    private:
      template<typename T, typename = void> struct has_spi_setup : std::false_type {};
      template<typename T> struct has_spi_setup<T, std::void_t<decltype(T::spi_setup(uint32_t{}, uint8_t{}))>> : std::true_type {};
    };
  };

  /// @brief SPI chip-select layer over a pin type (begin/on/off): CS idles high, select() drives it low
  template<typename Pin>
  struct CsPin {
    template<typename O>
    struct Part : O {
      using Base = O;

      static void begin() { Pin::begin(); Pin::on(); Base::begin(); }
      static void select()   { Pin::off(); }
      static void deselect() { Pin::on(); }
    };
  };

  /// @brief a shared SPI bus with its chip selects declared statically, one slot per pin (begin/on/off), in order.
  /// Every CS is driven high before the bus starts, so no device sees a stray select. select/deselect/xfer take a slot index.
  template<typename... Pins>
  struct SpiSlots {
    static_assert(sizeof...(Pins) > 0 && sizeof...(Pins) < 255, "SpiSlots: one to 254 chip selects");

    template<typename O>
    struct Part : O {
      using Base = O;
      static constexpr uint8_t slots = uint8_t(sizeof...(Pins));

      static void begin() { ((Pins::begin(), Pins::on()), ...); Base::begin(); }

      static void select(uint8_t slot)   { uint8_t i = 0; ((i++ == slot ? Pins::off() : void()), ...); }
      static void deselect(uint8_t slot) { uint8_t i = 0; ((i++ == slot ? Pins::on() : void()), ...); }

      // one full-duplex exchange with slot selected throughout; rx may be null, or the same buffer as tx
      static void xfer(uint8_t slot, const uint8_t* tx, uint8_t* rx, uint16_t n) {
        select(slot);
        for (uint16_t k = 0; k < n; ++k) { const uint8_t r = Base::transfer(tx ? tx[k] : 0); if (rx) rx[k] = r; }
        deselect(slot);
      }
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

      [[nodiscard]] static uint8_t transfer(uint8_t b)                  { return Base::transfer(b); }
      static void    send(const uint8_t* buf, uint16_t n)  { Base::send(buf, n); }
      static void    fill(uint8_t b, uint16_t n)            { Base::fill(b, n); }
    };
  };

} // oneBus
