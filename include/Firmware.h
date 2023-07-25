#pragma once
#include <Arduino.h>

#ifdef ESPHOME
#include "esphome.h"
class WordClock : public esphome::Component, public esphome::api::CustomAPIDevice, public esphome::light::LightOutput {
   public:
    void setup() override;
    void loop() override;
    esphome::light::LightTraits get_traits() override;

    void setup_state(esphome::light::LightState *state) override;
    void write_state(esphome::light::LightState *state) override;

   private:
    void setupEspHome();
    void on_set_brightness(int value);
    void on_execute_command(std::string command);

    
};
#else
void setup();
void loop();
#endif
