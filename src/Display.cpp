#include "Display.h"
#include "Log.h"

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include "TwoWireProvider.h"

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels


EVODisplay::EVODisplay(): init(false), display(NULL), blink_time(0), blink_period(0), enabled(false) {
}

EVODisplay::~EVODisplay() {
    if (display) delete display;
}

void EVODisplay::setup() {
    if (!init) {
        display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, TwoWireProvider::get_two_wire(), OLED_RESET);
        init = display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
        //init = true;
        Log::trace("[DS] Initialized {%d}\n", init);

        if (init) display->clearDisplay();

        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, HIGH);
    }
}

void EVODisplay::draw_text(const char* text) {
    if (enabled && init && text) {
        display->clearDisplay();
        display->setTextSize(2);      // Normal 1:1 pixel scale
        display->setTextColor(SSD1306_WHITE); // Draw white text
        display->setCursor(0, 0);     // Start at top-left corner
        display->cp437(true);         // Use full 256 char 'Code Page 437' font
        display->write(text, strlen(text));
        display->display();
    }
}

void EVODisplay::blink(unsigned long ms, unsigned long period)
{
    if (enabled)
    {
        digitalWrite(LED_PIN, HIGH);
        blink_time = ms;
        blink_period = period;
    }
}

void EVODisplay::loop(unsigned long ms)
{
    if ((ms-blink_time)>=blink_period)
    {
        digitalWrite(LED_PIN, LOW);
    }
}

void EVODisplay::enable()
{
    enabled = true;
}

void EVODisplay::disable()
{
    enabled = false;
}

bool EVODisplay::is_enabled()
{
    return enabled;
}