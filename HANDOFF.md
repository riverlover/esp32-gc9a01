# Handoff — ESP32-C3 Super Mini + GC9A01

> 交接文档。下一会话直接按本文继续，不必重走摸索过程。  
> 更新：2026-08-22 — 稳定版已打标签；当前在 BLE 试验分支。

## 新会话先看这里

| 项 | 值 |
|----|-----|
| 工程路径 | `/Users/lizhenhe/vscode/esp32-GC9A01` |
| **稳定版标签** | **`v0.2.0`**（Photo + EC11 悬浮预览 + Settings；**无 BLE**） |
| **当前分支** | **`feat/ble-wifiprov`**（与 `main` / `v0.2.0` 同提交起点，专做 BLE 试验） |
| `main` | 与 `v0.2.0` 同提交 `6fd204a` |
| 硬件 | **仅一块板**；试验前请先能烧回 `v0.2.0` |

### 一块硬件：稳定固件 ↔ BLE 试验

```bash
# 回到稳定版并烧录
git checkout v0.2.0   # 或 main
pio run -t upload

# 继续 BLE 试验
git checkout feat/ble-wifiprov
# …改代码…
pio run -t upload
```

可选备份到远程：`git push origin v0.2.0` · `git push -u origin feat/ble-wifiprov`

### 下一会话优先任务（BLE）

**目标**：在本板验证官方 **ESP BLE Provisioning** App 能否配网（历史曾 `abort`，需重验）。

**注意**：只在 `setup()`「打开 BLE」**不够**；官方 App 需要 `WiFiProv.beginProvision(… SCHEME_BLE …)` + PoP。

**建议步骤**（勿一上来改挂表现有配网）：

1. 在 `feat/ble-wifiprov` 做**最小 WiFiProv BLE 试验**（可临时简化 `main` / 旁路表盘与 Canvas，省堆）。
2. 手机装 Espressif **ESP BLE Provisioning** → Provision Device → 无二维码 → 选 `PROV_…` → 填 PoP → 选 2.4G Wi‑Fi。
3. 通了再考虑并回 Settings「BLE 配网」；不通则保留手机热点方案，合并回 `main` 时可不带 BLE。

`config.h` 里已有残留宏（尚未接线到 WiFiProv）：

- `PROV_SERVICE_NAME` 默认 `GC9A01-Setup`（App 侧常期望 `PROV_` 前缀，试验时建议改名）
- `PROV_POP` 默认 `12345678`

当前固件**运行时未开 BLE**，无配对模式可进。

---

## 当前状态（已验证）

| 项 | 状态 |
|----|------|
| 硬件接线 | 已接好并点亮 |
| macOS 识别 | Espressif USB JTAG/serial，无需额外驱动 |
| 工具链 | PlatformIO Core 6.1.19（`~/.platformio/penv/bin`） |
| 稳定固件 `v0.2.0` | 默认 Photo；STA + NTP；**EC11 预览切面 + Settings 已通** |
| Wi‑Fi | 热点 `WatchESP` 已通；家宽 `TP-LINK_D6B1` **空载（拔屏）已通** |
| BLE | **未合入**；历史 WiFiProv 曾 abort；分支 `feat/ble-wifiprov` 待做 |

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

Reconnect = `forceReprovision` → 等手机热点（**阻塞**）。

## 软件栈

- Platform：`espressif32` 7.0.1 → Arduino-ESP32 **2.0.17**
- Board：`esp32-c3-devkitm-1`，`huge_app.csv`
- 库：`GFX Library for Arduino@1.4.7`、`QRCode@0.0.1`（**勿升 GFX 1.5.x**）
- USB CDC：`ARDUINO_USB_MODE=1` + `ARDUINO_USB_CDC_ON_BOOT=1`

## 工程结构

```
src/main.cpp                 # 编排 / 串口 / EC11 / Settings
src/config.h                 # DEFAULT_FACE=Photo；PROV_* 残留宏
src/config.local.h           # gitignore
src/pins.h                   # 屏 + EC11
src/input/Ec11.*
src/time/TimeService.*       # NTP / Soft / Manual / 运行时区 / syncNtpNow
src/wifi/WifiProvision.*     # STA；8.5dBm；关 sleep
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

**首选（不改代码）**：手机开热点 **`WatchESP` / `12345678`** → 上电或 Settings→Wi‑Fi→Reconnect / 串口 `p`。

| 方式 | 场景 |
|------|------|
| 手机热点 `WatchESP` | 换地方校时，最稳 |
| 串口 `w SSID PASS` | 临时连任意网（仅当次；在等热点循环里） |
| `config.local.h` 烧录 | 长期固定路由器 |
| 串口 `s` | 跳过 Wi‑Fi，软时钟 / 手动 `t` |

家宽 `TP-LINK_D6B1`：**空载可连**；接屏曾握手失败 → 疑外设干扰。RF：`WIFI_POWER_8_5dBm` + `WIFI_PS_NONE`。

串口：`w` · `s` · `t YYYY-MM-DD HH:MM:SS` · `p` · `e`（EC11）· `h`

### Wi‑Fi 踩坑摘要

1. AUTH_EXPIRE / 4WAY_TIMEOUT ≠ 一定密码错。  
2. SoftAP / BLE Prov 本板历史不稳；华为连 ESP 热点差；**手机开热点让 ESP 连**更稳。  
3. 排障：降功率、关 sleep、2.4G WPA2、拔屏对比。

## 常用命令

```bash
ls /dev/cu.usbmodem*
export PATH="$HOME/.platformio/penv/bin:$PATH"
export http_proxy=http://127.0.0.1:7890 https_proxy=http://127.0.0.1:7890
cd /Users/lizhenhe/vscode/esp32-GC9A01
pio run -t upload
# monitor 115200
```

`platformio.ini` 端口示例：`/dev/cu.usbmodem1101`（数字可能变）。

## 后续可做（稳定线 / 试验线）

**`feat/ble-wifiprov`（当前）**

- [ ] 最小 WiFiProv BLE + 官方 App 验证  
- [ ] 通过后再设计与现有 `WifiProvision` 的启动分流  

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
- [STEPS.md](./STEPS.md) — 从接线到 EC11 / Settings 的步骤记录  
