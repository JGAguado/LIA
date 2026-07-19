#include "LiaBoard.h"

#include "configuration.h"

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
