# <img src="logo.png" alt="OneBus logo" width="32" height="32"> OneBus

**HAPI Compatibility:** Works with new Check/Apply/ApplyPack API (2026-Q2)

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

`end_write()` returns `true` iff everything was acknowledged, and `begin_write()` returns `false` when the address is known not to have
answered (a buffered master, such as Arduino Wire, can only say at `end_write()`). `request_from()` returns `0` when nothing answered.
`cause()` says why the last operation failed (`oneBus::TwiCause`: `Nack`, `Timeout`, `BusError`, `ArbLost`, …); a NACK is not a bus fault.

```cpp
if (oneBus::probe<MyI2c>(0x27)) { /* something answers at 0x27 */ }   // SLA+W then STOP; read-probe in 0x30-0x37, 0x50-0x5F
auto r = oneBus::scan<MyI2c>([](uint8_t addr) { /* present */ });   // 0x08..0x77; r.found addresses answered
if (r.stopped != oneBus::TwiCause::None) { /* the bus failed (Timeout, BusError, ArbLost) and ended the scan there */ }
```

The read-probe in 0x30-0x37 and 0x50-0x5F is there because a zero-length write can disturb some EEPROMs (AT24RF08-class parts, DIMM SPD).
**On ESP32 those ranges are probed with a write-probe by default**: its I2C driver takes at least a second to give up on a read whose
address is not acknowledged (measured on Arduino-ESP32 2.0.6), which would make a scan cost 24 s. Define `ONEBUS_ESP32_READ_PROBE` to
get the read-probe there, at that price per absent address; a build with a sensitive part on the bus can also leave those ranges out
of its own scan.

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

## License

MIT — see [LICENSE](LICENSE).

*Author: Rui Azevedo (neu-rah) · Azores, Portugal*
