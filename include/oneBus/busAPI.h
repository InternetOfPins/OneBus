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

} // oneBus
