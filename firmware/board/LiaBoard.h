#pragma once

#include <cstdint>

#include "Observer.h"

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

    /// Registers the PPC-cutoff deep-sleep hook. Must run from
    /// lateInitVariant(), NOT begin()/earlyInitVariant(): RadioInterface and
    /// GPS both register their own notifyDeepSleep observers during their own
    /// init (before lateInitVariant() runs), to gracefully command the SX1262
    /// and GNSS to standby over SPI/UART while they still have power.
    /// Observable fires observers in registration order, so registering this
    /// any earlier cuts PPC *before* those graceful-shutdown commands can
    /// reach the hardware -- confirmed via a real crash: RadioInterface's
    /// SX126xInterface::sleep() -> setStandby() SPI command failed
    /// (RadioLib err=-707) once PPC had already gone low first.
    void armDeepSleepHook();

    /// Power the SX1262 and SAM-M10Q (PPC high).
    void enablePeripherals();

    /// Cut power to the SX1262 and SAM-M10Q (PPC low), e.g. before deep sleep.
    void disablePeripherals();

    bool peripheralsEnabled() const { return peripheralsEnabled_; }

    /// Drive the RED channel of the status LED. 0 = off, 255 = full brightness.
    void setRedLed(uint8_t brightness);

    /// TP4056 CHRG output (open-drain, active low): true while charging.
    bool isCharging() const;

    /// TP4056 STBY: true (LOW) once charge is complete. STBY is pulled HIGH
    /// by an external resistor while charging and pulled LOW by an internal
    /// N-channel MOSFET on completion -- confirmed against the actual board
    /// wiring (2026-07-22), which briefly had this inverted mid-Phase-7
    /// before the wiring was clarified.
    bool isChargeComplete() const;

    /// BMS switch reading. Per firmware/AGENTS.md (the authoritative source --
    /// MOD.md's opposite "BMS high = Tracker mode" note is superseded): BMS
    /// HIGH means continuous/no-sleep behaviour, BMS LOW means the
    /// wake-every-minute sleep cycle. See TrackerService for the state
    /// machine that acts on this.
    bool isBmsHigh() const;

  private:
    LiaBoard() = default;

    /// Cuts PPC before the system actually deep sleeps, regardless of what
    /// triggered it (TrackerService's own doDeepSleep() calls, or in
    /// principle any other stock sleep path). Registered by armDeepSleepHook().
    int onDeepSleep(void *unused);

    bool began_ = false;
    bool peripheralsEnabled_ = false;
    CallbackObserver<LiaBoard, void *> deepSleepObserver_ = CallbackObserver<LiaBoard, void *>(this, &LiaBoard::onDeepSleep);
};
