#pragma once

#include <Arduino.h>

namespace WifiProvision {

using StatusFn = void (*)(const char *msg);

// Connect via config.local.h / NVS, else SoftAP web portal until GOT_IP.
bool ensureConnected(StatusFn status = nullptr);

// Serial 'p': clear creds and open SoftAP portal again.
void forceReprovision(StatusFn status = nullptr);

bool isConnected();

}  // namespace WifiProvision
