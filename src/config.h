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
#define WIFI_CONNECT_TIMEOUT_MS 15000
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

// Boot face. Runtime: Serial '1'..'4' or 'n' to cycle.
#ifndef DEFAULT_FACE
#define DEFAULT_FACE FaceId::Classic
#endif
