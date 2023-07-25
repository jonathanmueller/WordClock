#include "Firmware.h"

#include "ButtonHandler.h"
#include "Configuration.h"
#include "Apps.h"
#include "LEDs.h"
#include "Patterns.h"
#include "Network.h"
#include "SerialHandler.h"
#include "TemperatureKeeper.h"
#include "TimeKeeper.h"
#include "UpdateHandler.h"
#include "WebCommandHandler.h"
#include "WebServer.h"

#ifdef ESPHOME
void WordClock::setup()
#else
void setup()
#endif
{
    Serial.begin(115200);
    Serial.println();
    ESP.wdtEnable(500);

    setupButtonHandler();
    setupSerialHandler();
    setupConfiguration();
    setupLEDs();
    setupNetwork();
    setupWebServer();
    setupUpdateHandler();
    setupTimeKeeper();

#ifdef ESPHOME
    this->setupEspHome();
#endif
    // setupTemperatureKeeper();
}

#ifdef ESPHOME
void WordClock::loop()
#else
void loop()
#endif
{
    if (Update.isRunning()) {
        return;
    }
    rebootIfNecessary();

    updateWebServer();
    updateSerialHandler();
    updateTimeKeeper();
    // updateTemperatureKeeper();
    updateButtonHandler();

    EVERY_N_MILLIS(1000 / FRAMES_PER_SECOND) {
        updateLEDs();
    }
}

#ifdef ESPHOME
void WordClock::on_set_brightness(int value) {
    configuration.brightness = min(max(value, 0), 255);
    writeConfiguration();
}

void WordClock::on_execute_command(std::string command) {
    executeCommand(String(command.c_str()));
}

void WordClock::setupEspHome() {
    register_service(&WordClock::on_execute_command, "execute_command", {"command"});
}


class NamedEffect : public esphome::light::LightEffect {
    public:
    explicit NamedEffect(const std::string &name) : esphome::light::LightEffect(name) {}

    void start() override {
        currentApp = app_solid;
        appSwitchedAt = millis();
        executeCommand("display pattern " + String(get_name().c_str()));
    }

    void apply() override {
    }

    void stop() override {
        currentApp = app_time;
        appSwitchedAt = millis();
    }
};

class CycleApps : public NamedEffect {
    public:
    explicit CycleApps(const std::string &name) : NamedEffect(name) {}

    void start() override {
        currentApp = app_cycle;
        appSwitchedAt = millis();
        currentPattern = pattern_cycle;
        appCycleTimer.reset();
        patternCycleTimer.reset();
    }
};

esphome::light::LightTraits WordClock::get_traits() {
    // return the traits this light supports
    auto traits = esphome::light::LightTraits();
    traits.set_supported_color_modes({esphome::light::ColorMode::RGB, esphome::light::ColorMode::BRIGHTNESS});
    return traits;
}

void WordClock::setup_state(esphome::light::LightState *state) {
    state->add_effects({
        new CycleApps("cycleApps"),
        new NamedEffect("rainbow"),
        new NamedEffect("rainbowWithGlitter"),
        new NamedEffect("radar"),
        new NamedEffect("solid"),
        new NamedEffect("waves"),
        new NamedEffect("cycle"),
        new NamedEffect("sinelon"),
        new NamedEffect("bpm"),
        new NamedEffect("confetti"),
        new NamedEffect("noise")
    });

    state->set_restore_mode(esphome::light::LightRestoreMode::LIGHT_RESTORE_DEFAULT_ON);
}

void WordClock::write_state(esphome::light::LightState *state) {
    esphome::light::LightColorValues v = state->current_values;
    
    solidLEDColor = CRGB(v.get_red() * 255, v.get_green() * 255, v.get_blue() * 255);

    configuration.brightness = 255 * (v.get_state() * v.get_brightness() * v.get_color_brightness());
    FastLED.setBrightness(configuration.brightness); /* Avoid slow fade */
}

#endif