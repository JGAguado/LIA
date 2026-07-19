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

    deepSleepObserver_.observe(&notifyDeepSleep);

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
    // Common-anode: the channel lights when driven LOW, so invert the duty cycle.
    uint8_t duty = 255 - brightness;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ledcWrite(LIA_PIN_LED_RED, duty);
#else
    ledcWrite(kRedLedLedcChannel, duty);
#endif
}

bool LiaBoard::isCharging() const
{
    return digitalRead(LIA_PIN_CHG) == LOW;
}

bool LiaBoard::isChargeComplete() const
{
    return digitalRead(LIA_PIN_STBY) == LOW;
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
