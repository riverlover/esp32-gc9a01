#include "WifiProvision.h"

#include <WiFi.h>
#include <esp_wifi.h>
#include <string.h>

#include "config.h"

namespace WifiProvision {

static StatusFn gStatus = nullptr;
static volatile bool gGotIp = false;
static volatile uint8_t gLastDisconnectReason = 0;
static volatile bool gSkipWifi = false;

static void say(const char *msg) {
  Serial.println(msg);
  if (gStatus) {
    gStatus(msg);
  }
}

static const char *reasonText(uint8_t r) {
  switch (r) {
    case 2:
      return "AUTH_EXPIRE";
    case 15:
      return "4WAY_HANDSHAKE_TIMEOUT";
    case 201:
      return "NO_AP_FOUND";
    case 202:
      return "AUTH_FAIL";
    case 204:
      return "HANDSHAKE_TIMEOUT";
    default:
      return "?";
  }
}

static const char *encText(wifi_auth_mode_t e) {
  switch (e) {
    case WIFI_AUTH_OPEN:
      return "OPEN";
    case WIFI_AUTH_WPA_PSK:
      return "WPA";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA/WPA2";
    case WIFI_AUTH_WPA3_PSK:
      return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:
      return "WPA2/WPA3";
    default:
      return "OTHER";
  }
}

static bool wifiConfiguredInLocal() {
  return WIFI_SSID[0] != '\0' && strcmp(WIFI_SSID, "your-ssid") != 0;
}

static void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
    gGotIp = true;
  } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    gLastDisconnectReason = info.wifi_sta_disconnected.reason;
    Serial.printf("STA disconnect reason=%u (%s)\n", gLastDisconnectReason,
                  reasonText(gLastDisconnectReason));
  }
}

static void prepareStaRadio(bool bgOnly) {
  WiFi.persistent(false);
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  delay(200);
  WiFi.mode(WIFI_STA);
  // Super Mini: lower TX reduces self-interference; disable modem sleep for handshake.
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  wifi_country_t country = {};
  memcpy(country.cc, "CN", 2);
  country.schan = 1;
  country.nchan = 13;
  country.policy = WIFI_COUNTRY_POLICY_AUTO;
  esp_wifi_set_country(&country);

  if (bgOnly) {
    // Some phone hotspots are happier without HT40/11n.
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);
  } else {
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
  }
  Serial.println("RF: TX=8.5dBm sleep=OFF");
}

static bool waitConnected(uint32_t timeoutMs) {
  const uint32_t start = millis();
  while ((millis() - start) < timeoutMs) {
    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      line.trim();
      if (line.equalsIgnoreCase("s") || line.equalsIgnoreCase("skip")) {
        Serial.println("Skip requested during connect");
        gSkipWifi = true;
        WiFi.disconnect(true, true);
        return false;
      }
      if (line.startsWith("w ") || line.startsWith("W ")) {
        line = line.substring(2);
        line.trim();
        const int sp = line.indexOf(' ');
        if (sp > 0) {
          const String ssid = line.substring(0, sp);
          const String pass = line.substring(sp + 1);
          Serial.printf("Serial STA -> \"%s\" pass_len=%d\n", ssid.c_str(), pass.length());
          gGotIp = false;
          WiFi.begin(ssid.c_str(), pass.c_str());
        }
      }
    }
    if (WiFi.status() == WL_CONNECTED || gGotIp) {
      return true;
    }
    delay(200);
  }
  return WiFi.status() == WL_CONNECTED;
}

struct ApInfo {
  bool found = false;
  int32_t channel = 0;
  int32_t rssi = 0;
  wifi_auth_mode_t enc = WIFI_AUTH_OPEN;
  uint8_t bssid[6] = {0};
};

static ApInfo scanAp(const char *ssid) {
  ApInfo info;
  const int n = WiFi.scanNetworks(false, true);
  for (int i = 0; i < n; i++) {
    if (!WiFi.SSID(i).equals(ssid)) {
      continue;
    }
    info.found = true;
    info.channel = WiFi.channel(i);
    info.rssi = WiFi.RSSI(i);
    info.enc = WiFi.encryptionType(i);
    memcpy(info.bssid, WiFi.BSSID(i), 6);
    Serial.printf("  found \"%s\" ch=%d rssi=%d enc=%s(%d) bssid=%02X:%02X:%02X:%02X:%02X:%02X\n",
                  ssid, (int)info.channel, (int)info.rssi, encText(info.enc), (int)info.enc,
                  info.bssid[0], info.bssid[1], info.bssid[2], info.bssid[3], info.bssid[4],
                  info.bssid[5]);
    break;
  }
  WiFi.scanDelete();
  return info;
}

enum class ConnectMode : uint8_t { Plain = 0, ChannelOnly = 1, Bssid = 2, BgOnly = 3 };

static bool connectOnce(const char *ssid, const char *pass, const ApInfo &ap, ConnectMode mode,
                        uint32_t timeoutMs) {
  const bool bgOnly = (mode == ConnectMode::BgOnly);
  prepareStaRadio(bgOnly);
  gGotIp = false;
  gLastDisconnectReason = 0;

  Serial.printf("STA try mode=%u ssid=\"%s\" pass_len=%u\n", static_cast<unsigned>(mode), ssid,
                (unsigned)strlen(pass));

  switch (mode) {
    case ConnectMode::ChannelOnly:
      if (ap.found && ap.channel > 0) {
        WiFi.begin(ssid, pass, ap.channel);
        break;
      }
      // fallthrough
    case ConnectMode::Plain:
    case ConnectMode::BgOnly:
      WiFi.begin(ssid, pass);
      break;
    case ConnectMode::Bssid:
      if (ap.found && ap.channel > 0) {
        WiFi.begin(ssid, pass, ap.channel, ap.bssid, true);
      } else {
        WiFi.begin(ssid, pass);
      }
      break;
  }

  if (waitConnected(timeoutMs)) {
    Serial.print("IP=");
    Serial.println(WiFi.localIP());
    Serial.printf("RSSI=%d mode=%u\n", WiFi.RSSI(), static_cast<unsigned>(mode));
    say("Wi-Fi OK");
    return true;
  }

  Serial.printf("STA fail mode=%u status=%d reason=%u (%s)\n", static_cast<unsigned>(mode),
                (int)WiFi.status(), gLastDisconnectReason, reasonText(gLastDisconnectReason));
  WiFi.disconnect(true, false);
  delay(250);
  return false;
}

static bool connectSta(const char *ssid, const char *pass, const ApInfo &ap, const char *tag) {
  say(tag);
  Serial.println("If phone shows Allow/Connect popup — TAP ALLOW now");

  const ConnectMode modes[] = {ConnectMode::Plain, ConnectMode::ChannelOnly, ConnectMode::BgOnly,
                               ConnectMode::Bssid};
  for (ConnectMode mode : modes) {
    if (gSkipWifi) {
      return false;
    }
    if (connectOnce(ssid, pass, ap, mode, 15000)) {
      return true;
    }
  }
  return false;
}

static bool tryLocalConfig() {
  if (!wifiConfiguredInLocal()) {
    return false;
  }
  ApInfo empty;
  return connectSta(WIFI_SSID, WIFI_PASS, empty, "Trying config.local.h...");
}

static bool waitPhoneHotspot() {
  gSkipWifi = false;
  say("Open phone hotspot");
  Serial.println();
  Serial.println("=== Huawei hotspot ===");
  Serial.printf("SSID=\"%s\" PASS=\"%s\"\n", PHONE_HOTSPOT_SSID, PHONE_HOTSPOT_PASS);
  Serial.println("IMPORTANT: when ESP tries to join, Huawei often shows");
  Serial.println("  'Allow this device to connect?' -> tap Allow");
  Serial.println("Also disable any 'allowed devices only' / whitelist");
  Serial.println("Or serial: w SSID PASS | s = skip Wi-Fi");
  Serial.println("======================");

  prepareStaRadio(false);

  uint32_t lastScan = 0;
  while (!gSkipWifi) {
    if ((millis() - lastScan) > 5000) {
      lastScan = millis();
      say("Scanning hotspot...");
      Serial.println("Scanning for phone hotspot...");
      const ApInfo ap = scanAp(PHONE_HOTSPOT_SSID);
      if (ap.found) {
        say("Approve on phone!");
        if (connectSta(PHONE_HOTSPOT_SSID, PHONE_HOTSPOT_PASS, ap, "Joining phone hotspot...")) {
          return true;
        }
        if (gSkipWifi) {
          break;
        }
        say("AUTH fail — check Allow popup");
        prepareStaRadio(false);
      } else {
        Serial.printf("  \"%s\" not seen yet\n", PHONE_HOTSPOT_SSID);
        say("Waiting phone hotspot");
      }
    }

    if (Serial.available()) {
      String line = Serial.readStringUntil('\n');
      line.trim();
      if (line.equalsIgnoreCase("s") || line.equalsIgnoreCase("skip")) {
        say("Skip Wi-Fi");
        Serial.println("Skipping Wi-Fi; using soft/manual clock");
        gSkipWifi = true;
        break;
      }
      if (line.startsWith("w ") || line.startsWith("W ")) {
        line = line.substring(2);
        line.trim();
        const int sp = line.indexOf(' ');
        if (sp > 0) {
          const String ssid = line.substring(0, sp);
          const String pass = line.substring(sp + 1);
          ApInfo ap = scanAp(ssid.c_str());
          if (connectSta(ssid.c_str(), pass.c_str(), ap, "Serial Wi-Fi...")) {
            return true;
          }
          if (gSkipWifi) {
            break;
          }
          prepareStaRadio(false);
        }
      }
    }
    delay(50);
  }

  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
  return false;
}

bool isConnected() { return WiFi.status() == WL_CONNECTED; }

bool ensureConnected(StatusFn status) {
  gStatus = status;
  WiFi.onEvent(onWifiEvent);

  if (tryLocalConfig()) {
    return true;
  }
  return waitPhoneHotspot();
}

void forceReprovision(StatusFn status) {
  gStatus = status;
  say("Re-enter hotspot wait");
  WiFi.disconnect(true, true);
  delay(300);
  waitPhoneHotspot();
}

}  // namespace WifiProvision
