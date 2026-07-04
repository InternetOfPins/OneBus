#pragma once
#include <stdint.h>

// Compile-time UUID tags for oneBus::BleAPI characteristics + the id<->uuid pairing
// consumed by chip-side BLE cores to build a GATT table. Platform-neutral: no BLE
// stack types here, only plain values. Chip files (chips/<chip>/*Ble.h) convert these
// to their own runtime UUID type.

namespace oneBus {

  // 128-bit UUID, 16 bytes in the order printed in the UUID string
  // (e.g. 6e400001-b5a3-f393-e0a9-e50e24dcca9e -> Uuid128<0x6e,0x40,0x00,0x01, 0xb5,0xa3, ...>).
  template<uint8_t... B>
  struct Uuid128 {
    static_assert(sizeof...(B) == 16, "Uuid128 needs exactly 16 bytes");
    static constexpr uint8_t bytes[16] = {B...};
    static constexpr bool is128        = true;
  };

  // The standard Bluetooth SIG base UUID (00000000-0000-1000-8000-00805F9B34FB) that
  // 16-bit assigned numbers overlay into. Default Base for Uuid16 below — pass a
  // different Uuid128 as Base for vendor short-id schemes (e.g. Nordic UART Service's
  // 6E400000-B5A3-F393-E0A9-E50E24DCCA9E base, whose chars are Uuid16<0x0001,NusBase> etc.).
  using StdBase = Uuid128<0x00,0x00,0x00,0x00, 0x00,0x00, 0x10,0x00,
                          0x80,0x00, 0x00,0x80,0x5F,0x9B,0x34,0xFB>;

  // 16-bit UUID overlaid into Base (bytes[2..3] of Base, big-endian) — same schema as
  // every other optional component here: omit Base and StdBase (plain Bluetooth
  // SIG-assigned UUIDs, e.g. Uuid16<0x2A19> = Battery Level) is the default.
  template<uint16_t V, typename Base = StdBase>
  struct Uuid16 {
    static constexpr uint16_t value = V;
    using base                      = Base;
    static constexpr bool is128     = false;
  };

  // Pairs a characteristic's BleAPI id (GATT table index / handle) with its UUID tag.
  // Consumed by chip BLE cores to build the GATT table at begin(), and by whatever
  // addresses BleAPI::char_write/char_read/char_written by id.
  template<uint16_t Id, typename UuidTag>
  struct Characteristic {
    static constexpr uint16_t id = Id;
    using uuid = UuidTag;
  };

} // oneBus
