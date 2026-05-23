#ifndef BLUEPAD32_BRIDGE_H
#define BLUEPAD32_BRIDGE_H

#include <Arduino.h>
#include "ps_controller.h"

void onConnectedController(void* controller, void* userData);
void onDisconnectedController(void* controller, void* userData);
void processControllers(PSController* psCtrl);

#endif
