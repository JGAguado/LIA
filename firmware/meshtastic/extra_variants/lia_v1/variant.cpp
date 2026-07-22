#include "configuration.h"

#ifdef _VARIANT_LIA_V1

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
}

#endif // _VARIANT_LIA_V1
