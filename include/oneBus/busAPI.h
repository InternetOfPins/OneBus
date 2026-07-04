#pragma once

namespace oneBus {

  // Universal chain terminal — begin() is the only shared contract across all buses.
  struct BusAPI {
    BusAPI() = delete;
    static void begin() {}
  };

  // UART terminal — deletes primitives so a missing core is a compile error.
  // putch/getch/available are the public API; uart_init is the private init primitive.
  struct UartAPI : BusAPI {
    static void    uart_init(uint32_t) = delete;
    static void    putch(uint8_t)      = delete;
    static uint8_t getch()             = delete;
    static bool    available()         = delete;
  };

  // I2C/TWI terminal
  struct TwiAPI : BusAPI {
    static void    twi_init(uint32_t)  = delete;
    static void    twi_start()         = delete;
    static void    twi_stop()          = delete;
    static void    twi_write(uint8_t)  = delete;
    static uint8_t twi_read(bool)      = delete;
  };

  // SPI terminal
  struct SpiAPI : BusAPI {
    static void    spi_init(uint32_t)    = delete;
    static uint8_t spi_transfer(uint8_t) = delete;
  };

  // BLE/GATT terminal — deletes primitives so a missing chip core is a compile error.
  // A characteristic is identified by id (GATT handle or a chip-side table index);
  // UUID<->id/handle mapping and stack/advertising bring-up live in the chip override.
  // char_write/char_read/char_written are the public API; connected() gates them.
  struct BleAPI : BusAPI {
    static void    char_write(uint16_t id, const uint8_t*, uint8_t len) = delete;
    static uint8_t char_read(uint16_t id, uint8_t*, uint8_t maxLen)     = delete;
    static bool    char_written(uint16_t id)                           = delete;
    static bool    connected()                                         = delete;
  };

} // oneBus
