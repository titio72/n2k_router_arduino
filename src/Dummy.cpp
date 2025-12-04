#include "Dummy.h"
#include "Context.h"

Dummy::Dummy(): enabled(false) {}

void Dummy::enable() { enabled = true; }
void Dummy::disable() { enabled = false; }
bool Dummy::is_enabled() { return enabled; }
void Dummy::loop(unsigned long time, Context &ctx) {}
void Dummy::setup(Context &ctx) {}
