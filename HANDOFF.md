# Handoff — ESP32-C3 Super Mini + GC9A01

> 交接文档。下一会话直接按本文继续，不必重走摸索过程。  
> 更新：2026-08-22 — **官方 WiFiProv BLE 实机已通 `GOT_IP`**；稳定版仍为 `v0.2.0`（无 BLE）。

## 新会话先看这里

| 项 | 值 |
|----|-----|
| 工程路径 | `/Users/lizhenhe/vscode/esp32-GC9A01` |
| **稳定版标签** | **`v0.2.0`**（Photo + EC11 悬浮预览 + Settings；**无 BLE**） |
| **当前分支** | **`feat/ble-wifiprov`** |
| `main` | 与 `v0.2.0` 同提交 `6fd204a`（产品线未合 BLE） |
| 硬件 | **仅一块板**；试验前请先能烧回 `v0.2.0` |

### 一块硬件：稳定固件 ↔ BLE 试验

```bash
# 回到稳定版并烧录
git checkout v0.2.0   # 或 main
pio run -e esp32-c3-supermini -t upload

# BLE 最小试验固件
git checkout feat/ble-wifiprov
pio run -e ble-wifiprov-min -t upload
```

可选备份：`git push origin v0.2.0` · `git push -u origin feat/ble-wifiprov`

---

## 会话纪要：官方 WiFiProv BLE（2026-08-22）

### 结论（一句话）

**本板可用官方 App「ESP BLE Provisioning」完成 BLE 配网并拿到 IP**；必须用 `WiFiProv.beginProvision(SCHEME_BLE)+PoP`，不能只 `BLE.begin`。最小固件 env=`ble-wifiprov-min`；**尚未**并入手表主固件 / Settings。

### 验证结果

| 步骤 | 结果 |
|------|------|
| 板端 `PROV_START` / 广播 `PROV_GC9A01` | ✅ |
| App 发现设备、PoP 握手 | ✅（PoP 必须 `12345678`） |
| App 列出 / 选择 Wi‑Fi | ✅（曾空列表；刷新 / 重试后可扫到） |
| 下发凭据 | ✅ |
| STA 连上并 `GOT_IP` | ✅（串口 `got_ip=1 wifi=3`；屏显 SSID + IP） |
| 屏上 BLE 配网二维码 | ✅（官方 App 可扫；见踩坑 §9） |
| 历史 `abort()` 复现 | ⚠️ 曾因错误关 sleep 复现；根因已定位并规避 |

### 怎么烧 / 怎么配（可复现）

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
export http_proxy=http://127.0.0.1:7890 https_proxy=http://127.0.0.1:7890
cd /Users/lizhenhe/vscode/esp32-GC9A01
pio run -e ble-wifiprov-min -t upload
# monitor 115200 → === ble-wifiprov-min === / PROV_START
```

| 项 | 值 |
|----|-----|
| PIO env | `ble-wifiprov-min` |
| 源码 | `src/ble_prov_main.cpp`（无 Canvas / 表盘 / EC11；省堆） |
| 屏上 QR | 官方 JSON + `"security":1`；`src/vendor/ricmoo_qrcode/`（勿用过小 version） |
| 默认手表 env | `esp32-c3-supermini`（`build_src_filter` **排除** `ble_prov_main.cpp` 与 `vendor/`） |
| 设备名 | `PROV_GC9A01`（`config.h` / 编译宏） |
| PoP | `12345678`（**不是** Wi‑Fi 密码，也不是示例默认 `abcd1234`） |

**手机**（两种入口均可）：

1. **扫屏上二维码**（推荐，已通）→ 自动带上 name/PoP/security。  
2. 或 Provision Device → **无二维码** → 选 `PROV_GC9A01` → PoP `12345678` → 选 **2.4G** Wi‑Fi。

成功后屏显：`Wi-Fi OK` / `SSID …` / `IP x.x.x.x`（须等 DHCP，勿停在 `0.0.0.0`）。

```text
PROV_START — open ESP BLE Provisioning app
Prov SSID="..."
GOT_IP x.x.x.x
hb started=1 got_ip=1 wifi=3
```

### 下一会话优先（产品化）

1. 决定：手表固件内嵌 BLE 配网入口（Settings），还是继续只保留热点 `WatchESP`。  
2. 若合入：启动分流（未配网 → BLE Prov；已配网 → STA）；**BLE 期间禁止 `WIFI_PS_NONE`**。  
3. 合入前用 Canvas(~112KB)+表盘复测堆与共存；不通则 BLE 不进 `main`。

---

## BLE / WiFiProv 踩坑详录（本会话 + 历史对照）

### 1. 不能只开 BLE

只在 `setup()`「打开 BLE / `BLE.begin`」**不够**。官方 App 认的是 Espressif **WiFiProv 协议**（固定 GATT + Security1 + PoP）：

```cpp
WiFiProv.beginProvision(
  WIFI_PROV_SCHEME_BLE,
  WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
  WIFI_PROV_SECURITY_1,
  PROV_POP,            // "12345678"
  PROV_SERVICE_NAME,   // "PROV_GC9A01"
  nullptr, uuid,
  true                 // reset_provisioned：试验时每次进配网
);
```

Arduino 官方示例里 BLE 分支常写成 `CONFIG_IDF_TARGET_ESP32`（经典 ESP32）；**C3 同样有 Bluedroid**，应显式走 `SCHEME_BLE`（本试验已这样做）。

### 2. PoP ≠ Wi‑Fi 密码

| 名称 | 何时填 | 本工程值 |
|------|--------|----------|
| **PoP** | App「Proof of Possession」 | **`12345678`** |
| Wi‑Fi 密码 | 「Select Wi‑Fi Network」之后 | 路由器 / 热点真实密码 |

填错 PoP（例如示例默认 **`abcd1234`**）→ 串口：

```text
E (...) security1: Key mismatch. Close connection
E (...) security1: Session setup error -1
```

### 3. App 扫到设备但 Wi‑Fi 列表为空

已知现象（Espressif App / provisioning 社区常见，Android 更明显）：

- 列表空白 ≠ BLE 失败；会话可能已建立。  
- 可点刷新；或找 **Join Other Network** 手填 SSID/密码。  
- 社区：附近 AP 很多（>16）时库有时返回空列表；或需忽略蓝牙配对 + 强杀 App + 复位板子后再试「第一次」。  
- 本会话：空列表后重试/刷新后扫到了网。

### 4.「Wi-Fi Authentication failed」≠ BLE 失败

App 勾选：Sending credentials ✅ → Applying ✅ → Checking ❌ **Wi-Fi Authentication failed**。

串口对应：

```text
wifi_prov_mgr: Disconnect reason : 2
wifi_prov_mgr: STA Auth Error
PROV_CRED_FAIL AUTH
```

`reason=2` = **AUTH_EXPIRE**。在本板历史上**经常不是密码错**，而是 RF / 共存 / 家宽策略（与 STA 直连家宽时同类）。

**建议**：先用已验证热点 **`WatchESP` / `12345678`** 证明端到端；再试家宽 2.4G；必要时拔屏对比。

### 5. 【致命】BLE 并存时禁止 `WiFi.setSleep(WIFI_PS_NONE)`

本会话为改善 STA 握手，在 `beginProvision` 之后调用了手表路径同款 `WIFI_PS_NONE`，立刻：

```text
E (...) wifi:Error! Should enable WiFi modem sleep when both WiFi and Bluetooth are enabled!!!!!!
abort() was called at PC ...
```

**这很可能就是更早「BLE Prov abort」的根因之一。**

正确做法（已写入 `ble_prov_main.cpp`）：

| 阶段 | 允许 |
|------|------|
| BLE 仍开启（配网中） | 可 `setTxPower(8.5dBm)`、国家码 CN、HT20；**保持 modem sleep 开启** |
| `PROV_END` / `GOT_IP` 且 BLE 已释放后 | 再 `WiFi.setSleep(WIFI_PS_NONE)` |

手表纯 STA（无 BLE）路径继续用 `WIFI_PS_NONE` 没问题。

### 6. 编译：`WiFiProv.printQR` + ricmoo `QRCode` 头文件冲突

`lib_deps` 里的 `ricmoo/QRCode` 的 `qrcode.h` 会让 `WiFiProv.cpp` 误判 `__has_include("qrcode.h")`，去用 ESP-IDF 的 `esp_qrcode_*` → 编译失败。

`ble-wifiprov-min` 使用：

```ini
lib_ignore = QRCode
```

并自行在屏上画 BLE QR（`src/vendor/ricmoo_qrcode/`），不调用 `WiFiProv.printQR()`。默认手表 env 仍依赖 lib `ricmoo/QRCode`（`ProvQr`）。

### 7. 资源 / 工程结构

- Canvas ≈112KB；BLE+WiFiProv 也吃堆。试验用 **独立 env + 旁路表盘/Canvas**。  
- `beginProvision` 后 heap 约 **133KB**；`GOT_IP` 后约 **203KB**（BLE 释放后回升）。  
- 分区：`huge_app.csv`（BLE 固件体积较大，本试验 Flash≈46%）。

### 8. 与产品线的关系

| 固件 | BLE | 配网 |
|------|-----|------|
| `v0.2.0` / `esp32-c3-supermini` | 无 | 手机热点 `WatchESP` / 串口 `w` / `config.local.h` |
| `ble-wifiprov-min` | 有 | 官方 App WiFiProv |

**勿把试验 env 当成日常表盘固件。** 合入 Settings 前单独设计启动分流。

### 9. 屏上配网二维码（已通；曾长期扫不上）

**正确载荷**（约 81 字节）：

```json
{"ver":"v1","name":"PROV_GC9A01","pop":"12345678","transport":"ble","security":1}
```

| 坑 | 说明 |
|----|------|
| **过小 version → 损坏码** | ricmoo `qrcode.c` 源码写明 `@TODO: Return error if data is too big`——数据塞不下仍返回成功。本载荷需 **≥ v5**（v5-M≈86 字节；v3-L≈55、v4-L≈80 都不够）。曾用 v3 → 各扫码 App 均无法识别；本机 Vision 解码 v5 正常、v3 无有效 payload。 |
| **缺 `security`:1** | 新版官方 App 缺省当 **Sec2**；固件是 `WIFI_PROV_SECURITY_1`，必须带 `"security":1`。 |
| **圆形屏裁切** | GC9A01 可视区为圆；轴对齐正方形边长须 ≤≈169px（直径 240）。QR+静区曾 >180px → 四角定位点被裁 → 无法识别。现 `kMaxBlock=156`，实测 `block≈123`。 |
| **成功屏 IP** | 勿在 DHCP 前刷「成功」；曾出现 `SSID ok` + `IP 0.0.0.0`。等非零 IP 再显示。 |
| **手机蓝牙缓存** | 重启后 App 可能跳过 PoP：系统蓝牙里忽略 `PROV_*`、强杀 App、再复位板子。 |

屏显成功态：`Wi-Fi OK` + `SSID …` + `IP …`。

---

## 当前状态（已验证）

| 项 | 状态 |
|----|------|
| 硬件接线 | 已接好并点亮 |
| macOS 识别 | Espressif USB JTAG/serial，无需额外驱动 |
| 工具链 | PlatformIO Core 6.1.19（`~/.platformio/penv/bin`） |
| 稳定固件 `v0.2.0` | 默认 Photo；STA + NTP；**EC11 预览切面 + Settings 已通** |
| Wi‑Fi（手表） | 热点 `WatchESP` 已通；家宽 `TP-LINK_D6B1` **空载（拔屏）已通** |
| BLE WiFiProv | **`ble-wifiprov-min` 实机 ✅**（含屏上 QR 扫描 + `GOT_IP`）；未合入 `main` |

## 硬件

- **MCU**：ESP32-C3 Super Mini（USB-C，板标 V1601 / Super Mini）
- **屏**：1.28" TFT 240×240，驱动 IC **GC9A01**，7 针排针
- **旋钮**：EC11
- **连接**：杜邦线母对母

### 屏引脚（`src/pins.h`）

| 屏 | GPIO | 线色 |
|----|------|------|
| VCC | **3.3**（勿 5V） | 红 |
| GND | **G** | 黑 |
| SCL | **6** | 橙 |
| SDA | **7** | 黄 |
| DC | **2** | 绿 |
| CS | **10** | 蓝 |
| RST | **3** | 紫 |

避开 GPIO 8/9（启动相关）。

### EC11（`src/pins.h`）

| EC11 | GPIO |
|------|------|
| CLK | **0** |
| DT | **1** |
| SW | **5** |
| + | **3.3** |
| GND | **G** |

| 手势 | 行为 |
|------|------|
| 旋转 | 黑底约 82% 缩小悬浮预览（**静态冻结**，不按秒刷） |
| 短按 | 确认全屏；Toast |
| 停转 ~4s | 自动确认 |
| 长按 | **Settings** |
| 菜单内 | 旋转滚动；短按进入/执行；长按返回；15s 空闲退出 |

驱动：`src/input/Ec11.*`（SW 上电自动判极性；串口 `e` 诊断）。

### Settings 高频项（`src/ui/Settings.*`）

Face · Sync(NTP) · Timezone(UTC±N) · Wi‑Fi(状态/Reconnect) · About · Back  

Reconnect = `forceReprovision` → 等手机热点（**阻塞**）。尚无「BLE 配网」项。

## 软件栈

- Platform：`espressif32` 7.0.1 → Arduino-ESP32 **2.0.17**
- Board：`esp32-c3-devkitm-1`，`huge_app.csv`
- 库：`GFX Library for Arduino@1.4.7`、`QRCode@0.0.1`（**勿升 GFX 1.5.x**）
- USB CDC：`ARDUINO_USB_MODE=1` + `ARDUINO_USB_CDC_ON_BOOT=1`

## 工程结构

```
platformio.ini               # env:esp32-c3-supermini + env:ble-wifiprov-min
src/main.cpp                 # 手表编排（默认 env）
src/ble_prov_main.cpp        # BLE 最小试验（仅 ble-wifiprov-min）
src/config.h                 # DEFAULT_FACE=Photo；PROV_GC9A01 / PROV_POP
src/config.local.h           # gitignore
src/pins.h                   # 屏 + EC11
src/input/Ec11.*
src/time/TimeService.*
src/wifi/WifiProvision.*     # 热点 STA；8.5dBm；关 sleep（无 BLE 时）
src/ui/ProvQr.*              # 热点提示（非 BLE）
src/ui/Settings.*
src/face/                    # Classic / Lume / Skeleton / Calendar / Photo
```

## 表盘

| 键 | 风格 |
|----|------|
| 1–4 | Classic / Lume / Skeleton / Calendar |
| **5** | **Photo（默认）** |

串口 `1`–`5` / `n`，或 EC11。

## Wi‑Fi / NTP（换环境最方便）

**首选（手表固件、不改代码）**：手机开热点 **`WatchESP` / `12345678`** → 上电或 Settings→Wi‑Fi→Reconnect / 串口 `p`。

| 方式 | 场景 |
|------|------|
| 手机热点 `WatchESP` | 换地方校时，最稳 |
| 官方 App BLE Prov | 仅 `ble-wifiprov-min` 试验固件；已验证可 `GOT_IP` |
| 串口 `w SSID PASS` | 临时连任意网（手表等热点循环里） |
| `config.local.h` 烧录 | 长期固定路由器 |
| 串口 `s` | 跳过 Wi‑Fi，软时钟 / 手动 `t` |

家宽 `TP-LINK_D6B1`：**空载可连**；接屏曾握手失败 → 疑外设干扰。纯 STA RF：`WIFI_POWER_8_5dBm` + `WIFI_PS_NONE`（无 BLE 时）。

串口（手表）：`w` · `s` · `t YYYY-MM-DD HH:MM:SS` · `p` · `e`（EC11）· `h`

### Wi‑Fi 踩坑摘要（通用）

1. AUTH_EXPIRE / 4WAY_TIMEOUT ≠ 一定密码错。  
2. SoftAP：华为连 ESP 热点差；**手机开热点让 ESP 连**更稳。  
3. BLE Prov：见上文「BLE / WiFiProv 踩坑详录」。  
4. 排障：降功率、（无 BLE 时）关 sleep、2.4G WPA2、拔屏对比。

## 常用命令

```bash
ls /dev/cu.usbmodem*
export PATH="$HOME/.platformio/penv/bin:$PATH"
export http_proxy=http://127.0.0.1:7890 https_proxy=http://127.0.0.1:7890
cd /Users/lizhenhe/vscode/esp32-GC9A01
pio run -e esp32-c3-supermini -t upload   # 手表
pio run -e ble-wifiprov-min -t upload     # BLE 试验
# monitor 115200
```

`platformio.ini` 端口示例：`/dev/cu.usbmodem1101`（数字可能变）。

## 后续可做（稳定线 / 试验线）

**`feat/ble-wifiprov`（当前）**

- [x] 最小 WiFiProv BLE 固件（`ble-wifiprov-min`）  
- [x] 官方 App 实机配网至 `GOT_IP`  
- [x] 定位 BLE+`WIFI_PS_NONE` → `abort()`  
- [x] 屏上 BLE QR（v5 + security:1 + 圆内尺寸）可被官方 App 扫描  
- [ ] 设计与现有 `WifiProvision` / Settings 的启动分流（可选合入）  

**`main` / `v0.2.0` 产品线**

- Settings：亮度/休眠；Reconnect 非阻塞  
- Calendar/Photo 专用旋钮交互；RTC  
- 家宽有载复测；GFX 1.5 需换 core 3.x  

## 其他踩坑

1. macOS 对 C3 USB 一般免驱。  
2. 下载走代理 `7890`。  
3. 屏只用 **3.3V**。  
4. Wi‑Fi 密码只放 `config.local.h`。  
5. 预览缩放用 TFT `writeAddrWindow` + `writePixels`；预览中勿按秒重绘。

## 相关文档

- [README.md](./README.md) — 使用说明  
- [STEPS.md](./STEPS.md) — 从接线到 EC11 / Settings / BLE 试验的步骤记录  
