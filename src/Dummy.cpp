#include "Dummy.h"

Dummy::Dummy(): enabled(false) {}

void Dummy::enable() { enabled = true; }
void Dummy::disable() { enabled = false; }
bool Dummy::is_enabled() { return enabled; }
void Dummy::loop(unsigned long time) {}
void Dummy::setup() {}
