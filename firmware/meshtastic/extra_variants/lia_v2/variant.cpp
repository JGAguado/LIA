#include "configuration.h"

#ifdef _VARIANT_LIA_V2

#include "esp_sleep.h"
#include "lia/LiaBoard.h"

// Identical to lia_v1's earlyInitVariant() -- see that file for why PPC is
// enabled here and why there's no logging. This revision's I2C bus (battery
// gauge) isn't touched here: Meshtastic's stock main.cpp calls
// Wire.begin(I2C_SDA, I2C_SCL) and scans it independently of anything in
// this file, since variant.h defines those macros.
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

    // See lia_v1/variant.cpp's lateInitVariant() for why this must run here
    // and not from begin()/earlyInitVariant() -- unchanged on this revision.
    LiaBoard::instance().armDeepSleepHook();

    new TrackerService();
    new ChargeStatusService();
}

// See lia_v1/variant.cpp's variant_shutdown() for the full rationale --
// unchanged on this revision (BMS/CHG pins are the same).
void variant_shutdown()
{
    esp_sleep_enable_ext0_wakeup((gpio_num_t)LIA_PIN_CHG, LOW);
    esp_sleep_enable_ext1_wakeup(1ULL << LIA_PIN_BMS, ESP_EXT1_WAKEUP_ANY_HIGH);
}

#endif // _VARIANT_LIA_V2
