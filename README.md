# OneBus

HAPI bus protocol components — SPI, I2C/TWI, UART, 1-Wire, and I2C GPIO expander. Parameterized on chip from [OneChip](https://github.com/InternetOfPins/OneChip); zero dynamic allocation, pure-static dispatch.

Part of the [InternetOfPins](https://github.com/InternetOfPins) project family.

## SPI

```cpp
#include <oneBus/spiMaster.h>
using namespace oneBus;

// AVR hardware SPI — clock = F_CPU / speed_divisor
using MySpi = SpiMaster<SPI_CLOCK_DIV4>;
MySpi::begin();
MySpi::select();             // assert CS (your OutPin)
uint8_t b = MySpi::transfer(0xFF);
MySpi::deselect();
```

For ESP32: `Esp32SpiMaster<>` in [OneChip](https://github.com/InternetOfPins/OneChip), which also provides `transfer(buf, len)`.

## I2C / TWI

```cpp
#include <oneBus/twiMaster.h>

// AVR hardware TWI — 100 kHz at 16 MHz
using MyI2c = hw::avr::AvrTwiMaster<100000UL, F_CPU>;
MyI2c::begin();
MyI2c::begin_write(0x3C);
MyI2c::write_byte(0x00);
MyI2c::write_byte(data);
MyI2c::end_write();
```

## UART

```cpp
#include <oneBus/uart.h>
```

Chip-family Serial aliases live in `OneBus/uart.h`. Chip definitions (baud rates, register addresses) come from [OneChip](https://github.com/InternetOfPins/OneChip).

## 1-Wire

Two cores are provided:

### `OneWire<PinN>` — Arduino bit-bang (any target)

```cpp
#include <oneBus/oneWire.h>

using Bus = oneBus::OneWire<4>;  // Arduino pin 4
Bus::begin();
bool present = Bus::reset();
Bus::skip();                      // 0xCC — address single device
Bus::writeByte(0x44);             // Convert T (DS18B20)
```

### `AvrOneWire<Port, Bit>` — direct register, cycle-accurate (AVR only)

```cpp
#include <oneBus/oneWire.h>
#include <chips/avr/avrPort.h>

// PC4 = Arduino A4 on ATmega328P
using Bus = oneBus::AvrOneWire<hw::avr::chip::PortC, 4>;
```

`AvrOneWire` uses direct DDR/PORT/PIN register access and `_delay_us()` (cycle-counted at `F_CPU`). `cli()`/`sei()` guard only the critical edges of each bit slot; the 480 µs reset pulse runs with interrupts enabled.

Both cores expose the same protocol API: `begin()`, `reset()`, `writeByte()`, `readByte()`, `skip()`.

## I2C GPIO expander — PCF8574

```cpp
#include <oneBus/i2cGpio.h>

// InitShadow = 0x08 → bit 3 high (backlight on for LCD backpacks)
using Port = oneBus::I2cGpio<MyI2c, 0x27, 0x08>;
using RS   = typename Port::Pin<0>;
using EN   = typename Port::Pin<2>;

Port::begin();
RS::on();
EN::off();
```

`I2cGpio<>` manages a shadow register for read-modify-write; pin changes are batched into a single I2C write per `flush()` (or immediately if you prefer).

## Dependencies

- [HAPI](https://github.com/InternetOfPins/HAPI)
- [OneChip](https://github.com/InternetOfPins/OneChip)

## License

MIT — see [LICENSE](LICENSE).

*Author: Rui Azevedo (neu-rah) · Azores, Portugal*
