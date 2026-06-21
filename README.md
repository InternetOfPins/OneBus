# OneBus

HAPI bus protocol components — I2C, SPI, UART, 1-Wire, I2C GPIO expander. Parameterized on chip from [OneChip](https://github.com/InternetOfPins/OneChip).

## Components

### `I2c<Chip>` — I2C/TWI master

```cpp
static void    init(uint32_t freq = 100000UL);
static bool    start(uint8_t addr, bool write);
static void    stop();
static bool    write(uint8_t data);
static uint8_t read(bool ack);
```

### `Spi<Chip>` — SPI master

```cpp
static void    init(uint8_t clockDiv = 4);
static void    select();
static void    deselect();
static uint8_t transfer(uint8_t data);
static void    transferBuf(uint8_t* buf, uint16_t len);
```

### `Uart<Chip>` — UART

```cpp
static void    init(uint32_t baud);
static void    write(uint8_t data);
static uint8_t read();
static bool    available();
```

### `OneWire<Chip>` — 1-Wire bus

```cpp
static void reset();
static void write(uint8_t data);
static uint8_t read();
```

### `I2cGpio<TwiMaster, Addr, InitShadow>` — PCF8574 I2C GPIO expander

Virtual output pins over I2C. Used by [OneIO](https://github.com/InternetOfPins/OneIO) LCD backpack driver.

```cpp
using Port = I2cGpio<MyI2c, 0x27, 0x08>;   // InitShadow=0x08: backlight on
using RS   = Port::Pin<0>;
using EN   = Port::Pin<2>;
RS::on(); EN::off();
```

## Usage

Components compose with [HAPI](https://github.com/InternetOfPins/HAPI) chains.
Chip implementations live in [OneChip](https://github.com/InternetOfPins/OneChip).

```cpp
#include <oneBus/i2c.h>
using MyI2c = oneBus::I2c<hw::avr::AvrChip>;
MyI2c::Part::init(400000UL);
```
