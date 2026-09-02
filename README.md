# ESP32-C3 + GC9A01 圆形表盘

ESP32-C3 Super Mini 驱动 1.28" GC9A01（240×240）圆形 TFT，实现模块化模拟表盘，并通过 Wi‑Fi + NTP 校时。

## 功能

- 七种表盘：Classic / Lume / Skeleton / Calendar / Photo / Crown / **Dash**（天气+日历多功能；默认仍为 Photo）
- Wi‑Fi STA 连接 + NTP 校时（默认东八区）
- 离屏 Canvas 双缓冲，减轻秒针刷新闪烁
- **EC11 旋钮**：黑底缩小悬浮预览、短按确认、**长按进 Settings**
- 串口切换表盘、手动校时、跳过/重试配网

## 硬件

| 部件 | 说明 |
|------|------|
| MCU | ESP32-C3 Super Mini（USB-C） |
| 屏 | 1.28" TFT 240×240，驱动 IC **GC9A01** |
| 旋钮 | EC11 旋转编码器（CLK / DT / SW） |
| 连接 | 杜邦线母对母 |

### 接线（与 `src/pins.h` 一致）

| 屏引脚 | ESP32-C3 | 建议线色 |
|--------|----------|----------|
| VCC | **3.3**（勿接 5V） | 红 |
| GND | **G** | 黑 |
| SCL | **6** | 橙 |
| SDA | **7** | 黄 |
| DC | **2** | 绿 |
| CS | **10** | 蓝 |
| RST | **3** | 紫 |

| TF 卡（SPI 与屏共用 SCK/MOSI） | ESP32-C3 |
|------|----------|
| CS | **4** |
| MISO | **20** |
| MOSI | **7**（与屏 SDA） |
| SCK | **6**（与屏 SCL） |
| VCC / GND | **3.3** / **G** |

| EC11 | ESP32-C3 |
|------|----------|
| CLK | **0** |
| DT | **1** |
| SW | **5** |
| + | **3.3** |
| GND | **G** |

信号脚可改，但必须与代码宏一致。尽量避开 GPIO 8/9（启动相关）。

## 软件栈

- [PlatformIO](https://platformio.org/) + `espressif32` / Arduino-ESP32 2.0.x
- 板型：`esp32-c3-devkitm-1`，分区 `huge_app.csv`
- 库：`GFX Library for Arduino@1.4.7`、`QRCode@0.0.1`  
  **不要升 GFX 到 1.5.x**（需要 Arduino-ESP32 3.x）

## 快速开始

### 1. 安装 PlatformIO

```bash
curl -fsSL -o /tmp/get-platformio.py \
  https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 /tmp/get-platformio.py
export PATH="$HOME/.platformio/penv/bin:$PATH"
```

### 2. 配置 Wi‑Fi（本地，不提交）

```bash
cp src/config.local.h.example src/config.local.h
# 编辑 WIFI_SSID / WIFI_PASS
```

`src/config.local.h` 已在 `.gitignore` 中，请勿提交密码。

推荐优先用手机个人热点（2.4 GHz / WPA2），例如默认提示热点名 `WatchESP`。家宽路由器在部分环境下接屏时可能握手失败，可先空载对比。

### 3. 确认串口并烧录

```bash
ls /dev/cu.usbmodem*          # macOS；数字可能不同
# 按需修改 platformio.ini 中的 upload_port / monitor_port

pio run -t upload
pio device monitor            # 115200
```

烧录成功后串口大致流程：

```text
Wi-Fi OK → NTP OK → Watch OK [NTP] Calendar …
```

## 表盘

| 键 | 风格 | 说明 |
|----|------|------|
| `1` | Classic | 经典指针 |
| `2` | Lume | 夜光风格 |
| `3` | Skeleton | 镂空 |
| `4` | Calendar | 日历窗 |
| `5` | Photo | 照片背景（默认） |
| `6` | Crown | 照片背景（皇冠肖像） |
| `7` | Dash | 多功能：大数字时钟 + 周几/月日 + Open‑Meteo 天气 |

编译期默认：`DEFAULT_FACE`（见 `src/config.h` / `config.local.h`）。运行期串口发 `1`–`7` 或 `n` 切换；也可用 EC11。

照片类表盘在 **EC11 悬浮预览** 时会做圆形遮罩，避免方形位图四角漏出。

Dash 天气默认坐标北京（`WEATHER_LAT` / `WEATHER_LON`），可在 `config.local.h` 覆盖；约每 20 分钟拉取 Open‑Meteo（无需 API Key）。串口 `r` 立即刷新。

## EC11 操作

| 操作 | 作用 |
|------|------|
| 旋转 | **缩小悬浮预览**（约 82%、黑底、静态冻结）上一张 / 下一张 |
| 短按 | 确认当前预览 → 全屏；屏上 Toast |
| 停转约 4s | 自动确认 |
| 长按 | 进入 **Settings** |

预览期间不按秒刷新。串口 `e` 可查看 SW 电平 / 极性。

### Settings（长按）

| 手势 | 作用 |
|------|------|
| 旋转 | 上下选项；Timezone 页加减 `UTC±N` |
| 短按 | 进入 / 执行 / 选表盘 |
| 长按 | 返回上一级；根菜单则退出 |
| 15s 无操作 | 自动退出 |

| 菜单项 | 说明 |
|--------|------|
| Face | 选表盘并立即生效 |
| Seconds | 开关秒针/秒数；关闭后约每分钟刷新 |
| Sync | 立即 NTP 校时（需 Wi‑Fi） |
| Timezone | 运行时区 UTC−12…+14 |
| Wi‑Fi | 状态 / Reconnect（等手机热点，会阻塞） |
| SD Card | 浏览 `/roms`（旋钮滚动；短按进目录/看大小；`..` 返回上级） |
| About | heap、时间源、IP |
| Back | 退出 |

### 功耗（简述）

- **已做**：Settings → **Seconds OFF** → 隐藏秒、约每分钟刷新（NVS 记住）。  
- **更大头**：常亮屏、Wi‑Fi 常连；背光/休眠、校时后关 Wi‑Fi、降 CPU、light sleep 等见 [HANDOFF.md](HANDOFF.md)「功耗与降耗手段」。

## 串口命令

| 命令 | 作用 |
|------|------|
| `1`–`7` / `n` | 选表盘 / 下一个 |
| `r` | 立即刷新天气 |
| `e` | EC11 诊断（SW raw / activeLow / 当前面） |
| `w SSID PASS` | 设置 Wi‑Fi 并连接 |
| `s` | 跳过 Wi‑Fi（软时钟） |
| `t YYYY-MM-DD HH:MM:SS` | 手动校时 |
| `p` | 重新进入热点等待 |
| `h` / `?` | 帮助 |

## 工程结构

```text
src/main.cpp           # 编排与串口 / EC11 / Settings
src/config.h           # 默认配置
src/config.local.h     # 本地密钥（gitignore）
src/pins.h             # GC9A01 + EC11 引脚
src/input/             # EC11 正交解码与按键
src/time/              # NTP / Soft / Manual / 时区
src/wifi/              # STA 连接与 RF 调参
src/ui/                # 配网提示 / Settings
src/prefs/             # NVS 偏好（如 Seconds）
src/sd/                # TF 卡（与屏共用 SPI）
src/face/              # Classic…Crown / Dash
src/weather/           # Open-Meteo 天气
```

## 文档

- [HANDOFF.md](HANDOFF.md) — 当前状态、踩坑与后续计划
- [STEPS.md](STEPS.md) — 从接线到点亮的完整操作记录

## 许可

个人/学习用途；第三方库遵循各自许可证。
