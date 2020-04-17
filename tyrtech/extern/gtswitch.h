#pragma once


extern "C" {

void gtswitch(const void* from_regs, void* to_regs);
void gtjump(void* to_regs);

}
