#pragma once

#include <cstdint>

/// Hardware abstraction for the LIA-specific peripherals that stock
/// Meshtastic has no concept of: peripheral power gating (PPC), the RED
/// status LED, the BMS mode switch, and TP4056 charger status.
///
/// The SX1262 radio and SAM-M10Q GPS are stock Meshtastic behaviour, wired up
/// purely through the pin #defines in variant.h -- LiaBoard does not touch
/// them directly. It also never touches I2C or the green/blue LED channels:
/// both are unavailable on this PCB revision (see firmware/AGENTS.md).
class LiaBoard
{
  public:
    static LiaBoard &instance();

    LiaBoard(const LiaBoard &) = delete;
    LiaBoard &operator=(const LiaBoard &) = delete;

    /// Configure all LIA-owned GPIOs. Must run before the SX1262 or SAM-M10Q
    /// are touched -- called from earlyInitVariant(), not lateInitVariant().
    void begin();

    /// Power the SX1262 and SAM-M10Q (PPC high).
    void enablePeripherals();

    /// Cut power to the SX1262 and SAM-M10Q (PPC low), e.g. before deep sleep.
    void disablePeripherals();

    bool peripheralsEnabled() const { return peripheralsEnabled_; }

    /// Drive the RED channel of the status LED. 0 = off, 255 = full brightness.
    void setRedLed(uint8_t brightness);

    /// TP4056 CHRG output (open-drain, active low): true while charging.
    bool isCharging() const;

    /// TP4056 STBY output (open-drain, active low): true once charge is complete.
    bool isChargeComplete() const;

    /// BMS switch reading. Raw HIGH/LOW only -- firmware/AGENTS.md Phase 6
    /// ties BMS HIGH to continuous/no-sleep behaviour, which is the opposite
    /// polarity from the original "BMS high = Tracker mode" note in MOD.md.
    /// Resolve that naming conflict with the user before wiring Phase 6's
    /// state machine to this; don't assume which label is current.
    bool isBmsHigh() const;

  private:
    LiaBoard() = default;

    bool began_ = false;
    bool peripheralsEnabled_ = false;
};
