#include "Display.h"
#include "Log.h"

#ifdef ESP32_ARCH
#include <Arduino.h>
#include "TwoWireProvider.h"
#include <Adafruit_SSD1306.h>

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels


EVODisplay::EVODisplay(): init(false), display(NULL), blink_time(0), blink_period(0), enabled(false) {
    display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &TwoWireProvider::get_two_wire(), OLED_RESET);
}

EVODisplay::~EVODisplay() {
    delete display;
}

void EVODisplay::setup() {
    if (!init) {

        init = display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
        Log::trace("[DS] Enabled {%d}\n", init);

        if (init) display->clearDisplay();

        pinMode(15, OUTPUT);
        digitalWrite(15, HIGH);
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
        digitalWrite(15, HIGH);
        blink_time = ms;
        blink_period = period;
    }
}

void EVODisplay::loop(unsigned long ms) 
{
  if ((ms-blink_time)>=blink_period) 
  {
    digitalWrite(15, LOW);
  }
}

#else
EVODisplay::EVODisplay(): init(false), display(0), blink_time(0), blink_period(0), enabled(false) {
}

EVODisplay::~EVODisplay() {
}

void EVODisplay::setup() {
    init = true;
}

void EVODisplay::draw_text(const char* text) {
    if (init && text) {
        //Log::trace("[Display] Message {%s}\n", text);
    }
}

void EVODisplay::blink(unsigned long ms, unsigned long period) 
{
}

void EVODisplay::loop(unsigned long ms) 
{
}
#endif


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