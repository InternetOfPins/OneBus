#pragma once
#include <stdint.h>

namespace oneBus {

  // Universal chain terminal — begin() is the only shared contract across all buses.
  struct BusAPI {
    BusAPI() = delete;
    static void begin() {}
  };

  // UART terminal — deletes primitives so a missing core is a compile error.
  // putch/getch/available/ready are the public API; uart_init is the private init primitive.
  // available() = RX has a byte to read; ready() = TX will accept a byte without busy-waiting.
  struct UartAPI : BusAPI {
    static void    uart_init(uint32_t) = delete;
    static void    putch(uint8_t)      = delete;
    [[nodiscard]] static uint8_t getch()             = delete;
    [[nodiscard]] static bool    available()         = delete;
    [[nodiscard]] static bool    ready()             = delete;
  };

  // Why the last TWI operation did not complete. Each core keeps it and is asked after the call;
  // it is reset when the next operation starts. Unknown: the platform reports failure without a cause.
  enum class TwiCause : uint8_t { None, Nack, Timeout, BusError, ArbLost, Overflow, Unknown };

  // A failure that belongs to the bus, not to the address that was being tried.
  constexpr bool isBusFault(TwiCause c) { return c == TwiCause::Timeout || c == TwiCause::BusError || c == TwiCause::ArbLost; }

  // Arduino Wire's endTransmission() return codes (0 ok, 1 too long, 2/3 NACK on address/data, 4 other, 5 timeout).
  constexpr TwiCause twiCauseFromWire(uint8_t e) {
    return e == 0 ? TwiCause::None     : e == 1 ? TwiCause::Overflow : (e == 2 || e == 3) ? TwiCause::Nack
         : e == 4 ? TwiCause::BusError : e == 5 ? TwiCause::Timeout  : TwiCause::Unknown;
  }

  // I2C/TWI terminal. twi_start/twi_write return false when nothing acknowledged or the bus failed
  // (twi_cause() says which); a core whose twi_start/twi_write return void is treated as always acknowledging.
  struct TwiAPI : BusAPI {
    static void    twi_init(uint32_t)  = delete;
    static bool    twi_start()         = delete;
    static void    twi_stop()          = delete;
    static bool    twi_write(uint8_t)  = delete;
    [[nodiscard]] static uint8_t twi_read(bool)      = delete;
    static TwiCause twi_cause()        = delete;
  };

  // SPI terminal
  struct SpiAPI : BusAPI {
    static void    spi_init(uint32_t)    = delete;
    [[nodiscard]] static uint8_t spi_transfer(uint8_t) = delete;
  };

  // BLE/GATT terminal — deletes primitives so a missing chip core is a compile error.
  // A characteristic is identified by id (GATT handle or a chip-side table index);
  // UUID<->id/handle mapping and stack/advertising bring-up live in the chip override.
  // char_write/char_read/char_written are the public API; connected() gates them.
  struct BleAPI : BusAPI {
    static void    char_write(uint16_t id, const uint8_t*, uint8_t len) = delete;
    [[nodiscard]] static uint8_t char_read(uint16_t id, uint8_t*, uint8_t maxLen)     = delete;
    [[nodiscard]] static bool    char_written(uint16_t id)                           = delete;
    [[nodiscard]] static bool    connected()                                         = delete;
  };

} // oneBus
