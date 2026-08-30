
/******************************************************************************
 *
 * encoders.cpp
 *
 * ESP32 Smart Radio V2
 *
 * CONTROLS
 * ---------------------------------------------------------------------------
 *
 * GPIO 1  = physical button
 *            Short press -> next widget
 *            Long press  -> Favourites
 *
 * GPIO 2  = capacitive touch -> previous station
 * GPIO 6  = capacitive touch -> next station
 *
 * GPIO 35 = capacitive touch -> volume down
 * GPIO 36 = capacitive touch -> volume up
 *
 ******************************************************************************/

#include <Arduino.h>

#include "encoders.h"
#include "station_manager.h"
#include "audio_engine.h"
#include "widget_manager.h"
#include "widget_favourites.h"
#include "widget_audio.h"

// ============================================================
// GPIO
// ============================================================

constexpr uint8_t CONTROL_BUTTON = 1;

constexpr uint8_t TOUCH_PREVIOUS = 2;
constexpr uint8_t TOUCH_NEXT     = 6;

constexpr uint8_t TOUCH_VOLUME_DOWN = 4;
constexpr uint8_t TOUCH_VOLUME_UP   = 5;

// ============================================================
// BUTTON SETTINGS
// ============================================================

constexpr unsigned long BUTTON_DEBOUNCE_MS = 40;
constexpr unsigned long BUTTON_LONG_PRESS_MS = 2000;

// ============================================================
// TOUCH SETTINGS
// ============================================================

constexpr unsigned long TOUCH_COOLDOWN_MS = 500;

// Touch is considered active when reading falls below this
// percentage of the calibrated idle value.
//
// 75 = 25% drop from idle
//
constexpr uint8_t TOUCH_THRESHOLD_PERCENT = 75;

// ============================================================
// BUTTON STATE
// ============================================================

static bool lastButtonState = HIGH;

static unsigned long buttonPressStart = 0;
static unsigned long lastButtonEvent = 0;

// ============================================================
// TOUCH BASELINES
// ============================================================

static uint16_t touch2Baseline  = 0;
static uint16_t touch6Baseline  = 0;
static uint16_t touch35Baseline = 0;
static uint16_t touch36Baseline = 0;

// ============================================================
// TOUCH ACTIVE STATES
// ============================================================

static bool touch2Active  = false;
static bool touch6Active  = false;
static bool touch35Active = false;
static bool touch36Active = false;

// ============================================================
// TOUCH EVENT TIMERS
// ============================================================

static unsigned long lastTouch2Event  = 0;
static unsigned long lastTouch6Event  = 0;
static unsigned long lastTouch35Event = 0;
static unsigned long lastTouch36Event = 0;

// ============================================================
// READ TOUCH AVERAGE
// ============================================================

static uint16_t readTouchAverage(uint8_t pin)
{
    uint32_t total = 0;

    for (uint8_t i = 0; i < 20; i++)
    {
        total += touchRead(pin);
        delay(5);
    }

    return static_cast<uint16_t>(
        total / 20
    );
}

// ============================================================
// TOUCH DETECTION
// ============================================================

static bool touchDetected(
    uint16_t reading,
    uint16_t baseline
)
{
    if (baseline == 0)
        return false;

    uint32_t threshold =
        ((uint32_t)baseline *
         TOUCH_THRESHOLD_PERCENT) / 100;

    return reading < threshold;
}

// ============================================================
// INITIALISE
// ============================================================

void encodersInit()
{
    Serial.println();
    Serial.println(
        "[INPUT] Initialising controls"
    );

    // ========================================================
    // GPIO 1 BUTTON
    // ========================================================

    pinMode(
        CONTROL_BUTTON,
        INPUT_PULLUP
    );

    lastButtonState =
        digitalRead(CONTROL_BUTTON);

    Serial.print(
        "[INPUT] GPIO 1 state: "
    );

    Serial.println(
        lastButtonState
    );

    // ========================================================
    // TOUCH CALIBRATION
    // ========================================================

    Serial.println(
        "[TOUCH] Calibrating GPIO 2, 6, 35 and 36..."
    );

    Serial.println(
        "[TOUCH] DO NOT TOUCH THE INPUTS"
    );

    delay(1000);

    touch2Baseline =
        readTouchAverage(
            TOUCH_PREVIOUS
        );

    touch6Baseline =
        readTouchAverage(
            TOUCH_NEXT
        );

    touch35Baseline =
        readTouchAverage(
            TOUCH_VOLUME_DOWN
        );

    touch36Baseline =
        readTouchAverage(
            TOUCH_VOLUME_UP
        );

    // ========================================================
    // DISPLAY CALIBRATION VALUES
    // ========================================================

    Serial.println();

    Serial.print(
        "[TOUCH] GPIO 2 baseline  : "
    );

    Serial.println(
        touch2Baseline
    );

    Serial.print(
        "[TOUCH] GPIO 6 baseline  : "
    );

    Serial.println(
        touch6Baseline
    );

    Serial.print(
        "[TOUCH] GPIO 35 baseline : "
    );

    Serial.println(
        touch35Baseline
    );

    Serial.print(
        "[TOUCH] GPIO 36 baseline : "
    );

    Serial.println(
        touch36Baseline
    );

    Serial.println();

    Serial.println(
        "[INPUT] Controls ready"
    );

    Serial.println();
}

// ============================================================
// BUTTON HANDLER
// ============================================================

static void handleButton(
    unsigned long now
)
{
    bool buttonState =
        digitalRead(
            CONTROL_BUTTON
        );

    // --------------------------------------------------------
    // PRESSED
    // --------------------------------------------------------

    if (lastButtonState == HIGH &&
        buttonState == LOW &&
        now - lastButtonEvent >= BUTTON_DEBOUNCE_MS)
    {
        buttonPressStart = now;
        lastButtonEvent = now;

        widgetUserActivity();

        Serial.println(
            "[BUTTON] GPIO 1 pressed"
        );
    }

    // --------------------------------------------------------
    // RELEASED
    // --------------------------------------------------------

    if (lastButtonState == LOW &&
        buttonState == HIGH &&
        now - lastButtonEvent >= BUTTON_DEBOUNCE_MS)
    {
        unsigned long held =
            now - buttonPressStart;

        lastButtonEvent = now;

        // ====================================================
        // LONG PRESS -> FAVOURITES
        // ====================================================

        if (held >= BUTTON_LONG_PRESS_MS)
        {
            Serial.println(
                "[BUTTON] GPIO 1 LONG PRESS"
            );

            Serial.println(
                "[BUTTON] Opening Favourites"
            );

            currentWidget = WIDGET_AUDIO;

            favouritesActive:
            favWidgetDraw();

            widgetUserActivity();
        }

        // ====================================================
        // SHORT PRESS -> NEXT WIDGET
        // ====================================================

        else
        {
            Serial.println(
                "[BUTTON] GPIO 1 SHORT PRESS"
            );

            widgetNext();
        }
    }

    lastButtonState =
        buttonState;
}

// ============================================================
// GPIO 2
// PREVIOUS STATION
// ============================================================

static void handleTouchPrevious(
    unsigned long now
)
{
    uint16_t reading =
        touchRead(
            TOUCH_PREVIOUS
        );

    bool detected =
        touchDetected(
            reading,
            touch2Baseline
        );

    if (detected &&
        !touch2Active &&
        now - lastTouch2Event >= TOUCH_COOLDOWN_MS)
    {
        touch2Active = true;
        lastTouch2Event = now;

        Serial.print(
            "[TOUCH] GPIO 2 -> PREVIOUS STATION  "
        );

        Serial.println(
            reading
        );

        widgetUserActivity();

        stationPrevious();
        stationConnectCurrent();
    }

    if (!detected)
    {
        touch2Active = false;
    }
}

// ============================================================
// GPIO 6
// NEXT STATION
// ============================================================

static void handleTouchNext(
    unsigned long now
)
{
    uint16_t reading =
        touchRead(
            TOUCH_NEXT
        );

    bool detected =
        touchDetected(
            reading,
            touch6Baseline
        );

    if (detected &&
        !touch6Active &&
        now - lastTouch6Event >= TOUCH_COOLDOWN_MS)
    {
        touch6Active = true;
        lastTouch6Event = now;

        Serial.print(
            "[TOUCH] GPIO 6 -> NEXT STATION  "
        );

        Serial.println(
            reading
        );

        widgetUserActivity();

        stationNext();
        stationConnectCurrent();
    }

    if (!detected)
    {
        touch6Active = false;
    }
}

// ============================================================
// GPIO 35
// VOLUME DOWN
// ============================================================

static void handleTouchVolumeDown(
    unsigned long now
)
{
    uint16_t reading =
        touchRead(
            TOUCH_VOLUME_DOWN
        );

    bool detected =
        touchDetected(
            reading,
            touch35Baseline
        );

    if (detected &&
        !touch35Active &&
        now - lastTouch35Event >= TOUCH_COOLDOWN_MS)
    {
        touch35Active = true;
        lastTouch35Event = now;

        uint8_t volume =
            audioGetVolume();

        if (volume > MIN_VOLUME)
        {
            volume--;

            audioSetVolume(
                volume
            );

            audioWidgetUpdateVolume();
        }

        Serial.print(
            "[TOUCH] GPIO 35 -> VOLUME DOWN: "
        );

        Serial.println(
            volume
        );

        widgetUserActivity();
    }

    if (!detected)
    {
        touch35Active = false;
    }
}

// ============================================================
// GPIO 36
// VOLUME UP
// ============================================================

static void handleTouchVolumeUp(
    unsigned long now
)
{
    uint16_t reading =
        touchRead(
            TOUCH_VOLUME_UP
        );

    bool detected =
        touchDetected(
            reading,
            touch36Baseline
        );

    if (detected &&
        !touch36Active &&
        now - lastTouch36Event >= TOUCH_COOLDOWN_MS)
    {
        touch36Active = true;
        lastTouch36Event = now;

        uint8_t volume =
            audioGetVolume();

        if (volume < MAX_VOLUME)
        {
            volume++;

            audioSetVolume(
                volume
            );

            audioWidgetUpdateVolume();
        }

        Serial.print(
            "[TOUCH] GPIO 36 -> VOLUME UP: "
        );

        Serial.println(
            volume
        );

        widgetUserActivity();
    }

    if (!detected)
    {
        touch36Active = false;
    }
}

// ============================================================
// MAIN LOOP
// ============================================================

void encodersLoop()
{
    unsigned long now =
        millis();

    handleButton(now);

    handleTouchPrevious(now);

    handleTouchNext(now);

    handleTouchVolumeDown(now);

    handleTouchVolumeUp(now);
}

