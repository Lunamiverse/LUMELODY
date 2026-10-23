// Prints the effects schema JSON (used to build the web UI before the engine starts).
#include "../../engine/LumiFx.h"
#include <cstdio>
int main() { std::puts(lumi::fxSchemaJson().c_str()); }
