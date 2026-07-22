#include "configuration.h"

#ifdef _VARIANT_LIA_V1

#include "esp_sleep.h"
#include "lia/LiaBoard.h"

// Runs at the start of setup(), directly after waitUntilPowerLevelSafe() and
// before initSPI() / the LoRa radio init that happens inside setupModules().
// PPC must already be HIGH by the time either of those touch the SX1262, so
// peripheral power is enabled here rather than in lateInitVariant() (see
// firmware/AGENTS.md hardware constraints and firmware/board/README.md).
//
// No logging here: this is the second call in setup() (main.cpp), before the
// console/logging subsystem is up. A LOG_INFO() call here crashes
// (LoadProhibited in RedirectablePrint::log()) on every boot -- confirmed by
// addr2line against the .elf. Log the outcome from lateInitVariant() instead,
// which runs after logging is known-good (radio init already logs by then).
void earlyInitVariant()
{
    LiaBoard::instance().begin();
    LiaBoard::instance().enablePeripherals();
}

#include "lia/services/ChargeStatusService.h"
#include "lia/services/CommandService.h"
#include "lia/services/TrackerService.h"

void lateInitVariant()
{
    LOG_INFO("LiaBoard: peripherals=%d (PPC=GPIO%d)", LiaBoard::instance().peripheralsEnabled(), LIA_PIN_PPC);

    // Must run after initLoRa() (already true here) and after GPS::setup(),
    // both of which register their own notifyDeepSleep observers to command
    // their hardware to standby/sleep gracefully over SPI/UART while it still
    // has power. Observable fires in registration order, so registering ours
    // any earlier cuts PPC before those graceful commands can land -- see
    // LiaBoard::armDeepSleepHook()'s comment for the crash this caused.
    LiaBoard::instance().armDeepSleepHook();

    new TrackerService();
    new ChargeStatusService();
    // Must come after ChargeStatusService: CommandService reaches it via
    // ChargeStatusService::instance() to act on CHG_ON/OFF and STB_ON/OFF.
    new CommandService();
}

// Weak-overridden per firmware/main-esp32.cpp's cpuDeepSleep(): called right
// before it configures the RTC-domain wake sources and calls
// esp_deep_sleep_start() (the timer wakeup for kSleepCycleMs is enabled
// immediately after this returns, so it always applies alongside whichever
// of these fires). Phase 8 "BMS wake" / "USB wake": without this, the
// sleep-cycle branch only wakes on its kSleepCycleMs timer, so flipping the
// BMS switch or plugging in a charger while the board is mid-sleep would sit
// unnoticed for up to that long.
//
// GPIO1 (CHG) and GPIO15 (BMS) are both RTC-capable on ESP32-S3 (0-21), but
// ext0 is a single-pin/single-level source and ext1 applies one shared level
// to its whole bitmask -- since CHG needs LOW and BMS needs HIGH, they can't
// share one call, so each gets its own independent HW wake source instead.
// There's no separate VBUS-sense pin on this board (see firmware/AGENTS.md
// hardware table), so CHG (active low, asserted whenever a charger is
// actively charging the battery) is the closest available proxy for "USB
// connected" -- confirm this reads as intended once real hardware is
// available to test a sleep-then-plug-in cycle.
void variant_shutdown()
{
    esp_sleep_enable_ext0_wakeup((gpio_num_t)LIA_PIN_CHG, LOW);
    esp_sleep_enable_ext1_wakeup(1ULL << LIA_PIN_BMS, ESP_EXT1_WAKEUP_ANY_HIGH);
}

#endif // _VARIANT_LIA_V1
