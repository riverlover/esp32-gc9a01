#pragma once

#include "face/FaceId.h"

// Local secrets / overrides (gitignored). See config.local.h.example.
#if __has_include("config.local.h")
#include "config.local.h"
#endif

// --- Wi-Fi / NTP ---
#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif
#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif

// China Standard Time (UTC+8). Change if needed.
#ifndef NTP_TZ_OFFSET_SEC
#define NTP_TZ_OFFSET_SEC (8 * 3600)
#endif

#ifndef NTP_SERVER_1
#define NTP_SERVER_1 "ntp.aliyun.com"
#endif
#ifndef NTP_SERVER_2
#define NTP_SERVER_2 "pool.ntp.org"
#endif

#ifndef WIFI_CONNECT_TIMEOUT_MS
#define WIFI_CONNECT_TIMEOUT_MS 20000
#endif

// BLE Wi-Fi provisioning (Espressif "ESP BLE Provisioning" app)
#ifndef PROV_SERVICE_NAME
#define PROV_SERVICE_NAME "GC9A01-Setup"
#endif
#ifndef PROV_POP
#define PROV_POP "12345678"
#endif
#ifndef PROV_AP_CHANNEL
#define PROV_AP_CHANNEL 6
#endif

// Huawei SoftAP-join is unreliable; ESP joins phone personal hotspot instead.
#ifndef PHONE_HOTSPOT_SSID
#define PHONE_HOTSPOT_SSID "WatchESP"
#endif
#ifndef PHONE_HOTSPOT_PASS
#define PHONE_HOTSPOT_PASS "12345678"
#endif

// Soft-clock fallback when NTP is unavailable (classic demo pose).
#ifndef SOFT_START_H
#define SOFT_START_H 10
#endif
#ifndef SOFT_START_M
#define SOFT_START_M 8
#endif
#ifndef SOFT_START_S
#define SOFT_START_S 0
#endif

// Boot face. Runtime: Serial '1'..'5' or 'n' to cycle.
#ifndef DEFAULT_FACE
#define DEFAULT_FACE FaceId::Photo
#endif
