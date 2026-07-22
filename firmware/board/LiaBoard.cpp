#include "LiaBoard.h"

#include "configuration.h"
#include "sleep.h"

namespace
{
constexpr int kRedLedLedcChannel = 7; // Arbitrary channel, unused elsewhere in this build.
constexpr int kRedLedFreqHz = 1000;
constexpr int kRedLedResolutionBits = 8;
} // namespace

LiaBoard &LiaBoard::instance()
{
    static LiaBoard board;
    return board;
}

void LiaBoard::begin()
{
    if (began_)
        return;
    began_ = true;

    pinMode(LIA_PIN_PPC, OUTPUT);
    disablePeripherals();

    pinMode(LIA_PIN_BMS, INPUT);
    pinMode(LIA_PIN_CHG, INPUT_PULLUP);
    pinMode(LIA_PIN_STBY, INPUT_PULLUP);

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ledcAttach(LIA_PIN_LED_RED, kRedLedFreqHz, kRedLedResolutionBits);
#else
    ledcSetup(kRedLedLedcChannel, kRedLedFreqHz, kRedLedResolutionBits);
    ledcAttachPin(LIA_PIN_LED_RED, kRedLedLedcChannel);
#endif
    setRedLed(0);
}

void LiaBoard::armDeepSleepHook()
{
    deepSleepObserver_.observe(&notifyDeepSleep);
}

void LiaBoard::enablePeripherals()
{
    digitalWrite(LIA_PIN_PPC, HIGH);
    peripheralsEnabled_ = true;
}

void LiaBoard::disablePeripherals()
{
    digitalWrite(LIA_PIN_PPC, LOW);
    peripheralsEnabled_ = false;
}

void LiaBoard::setRedLed(uint8_t brightness)
{
    // Was assumed common-anode (channel lights when driven LOW, so inverted
    // the duty cycle) -- real hardware showed the opposite: with BMS read
    // LOW, TrackerService calls setRedLed(0) intending OFF, but the LED was
    // observed ON (2026-07-22). The channel actually lights when driven
    // HIGH, so no inversion.
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ledcWrite(LIA_PIN_LED_RED, brightness);
#else
    ledcWrite(kRedLedLedcChannel, brightness);
#endif
}

bool LiaBoard::isCharging() const
{
    return digitalRead(LIA_PIN_CHG) == LOW;
}

bool LiaBoard::isChargeComplete() const
{
    return digitalRead(LIA_PIN_STBY) == HIGH;
}

bool LiaBoard::isBmsHigh() const
{
    return digitalRead(LIA_PIN_BMS) == HIGH;
}

int LiaBoard::onDeepSleep(void *)
{
    // Cuts power to both the SX1262 and SAM-M10Q together (shared PPC rail --
    // this board has no way to sleep one without the other). Meshtastic only
    // fires notifyDeepSleep when shouldLoraWake() is false, which is only
    // true for ROUTER roles, so this always runs for LIA's TRACKER role.
    disablePeripherals();
    return 0;
}
