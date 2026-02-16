#include "Dummy.h"
#include "Context.h"

Dummy::Dummy(): enabled(false) {}

void Dummy::enable(Context &ctx) { enabled = true; }
void Dummy::disable(Context &ctx) { enabled = false; }
bool Dummy::is_enabled() { return enabled; }
void Dummy::loop(unsigned long time, Context &ctx) {}
void Dummy::setup(Context &ctx) {}
