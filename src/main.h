#pragma once

#include <Fsm.h>
#include <TaskSchedulerDeclarations.h>

extern Fsm fsm;
extern Scheduler runner;

void handleButtonOnOff();
void handleButtonReset();