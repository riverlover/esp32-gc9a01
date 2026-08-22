# 全程步骤记录 — ESP32-C3 + GC9A01 点亮

按实际操作顺序整理，便于复现。

---

## 1. 识别硬件

1. MCU：ESP32-C3 Super Mini（USB-C，排针两侧丝印含 5V / G / 3.3 / 0–10 / 20 / 21）。
2. 屏：圆形 1.28" TFT，背面标 **GC9A01**、240×240，7 针：RST / CS / DC / SDA / SCL / GND / VCC。
3. 材料：杜邦线（彩虹排线 + 电源线）。

---

## 2. 确定接线原则

1. **VCC / GND 固定**：VCC → ESP32 **3.3**，GND → **G**。不要接 5V。
2. **SCL / SDA / DC / CS / RST 不固定**：可任意合法 GPIO，但必须与程序一致。
3. 采用首套推荐（省事、避开 BOOT 相关脚）：

| 屏 | ESP32-C3 | 颜色 |
|----|----------|------|
| VCC | 3.3 | 红 |
| GND | G | 黑 |
| SCL | 6 | 橙 |
| SDA | 7 | 黄 |
| DC | 2 | 绿 |
| CS | 10 | 蓝 |
| RST | 3 | 紫 |

4. 上电前检查：无短路、杜邦插紧、仅用 USB-C 给 ESP32 供电（屏从 3.3 取电）。

---

## 3. macOS 确认设备（烧录前）

1. 插入 USB 后执行：

```bash
ls /dev/cu.*
```

2. 成功时应出现类似：`/dev/cu.usbmodem1101`（数字可能不同）。
3. USB 信息应为 **Espressif / USB JTAG/serial debug unit** → **无需另装串口驱动**。
4. 若完全没有 `usbmodem` 设备：换线、换口；仅当确认是 CH340 芯片板时才考虑装 WCH 驱动（本板不是）。

---

## 4. 安装 PlatformIO（本机原先没有）

1. 系统代理已开：HTTP/HTTPS/SOCKS → `127.0.0.1:7890`（Flash/Clash 等）。
2. 带代理安装官方脚本：

```bash
export http_proxy=http://127.0.0.1:7890
export https_proxy=http://127.0.0.1:7890
export ALL_PROXY=socks5://127.0.0.1:7890

curl -fsSL -o /tmp/get-platformio.py \
  https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 /tmp/get-platformio.py

export PATH="$HOME/.platformio/penv/bin:$PATH"
pio --version   # 验证，例如 6.1.19
```

3. 建议把 `~/.platformio/penv/bin` 永久加入 shell PATH。

---

## 5. 创建工程

目录：`/Users/lizhenhe/vscode/esp32-GC9A01`

### `platformio.ini` 要点

- `board = esp32-c3-devkitm-1`
- `upload_port` / `monitor_port` 指向当前 `cu.usbmodem*`
- `build_flags`：`ARDUINO_USB_MODE=1`、`ARDUINO_USB_CDC_ON_BOOT=1`
- 库钉死：`GFX Library for Arduino@1.4.7`

### `src/main.cpp` 行为（初期 bring-up，已替换）

1. 初始化 GC9A01（引脚见上表）。
2. 全屏红 → 绿 → 蓝 → 黑。
3. 显示文字 `GC9A01` / `ESP32-C3`。
4. `loop` 中心半径 30 的圆按七色循环填充。

---

## 6. 编译与烧录

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
export http_proxy=http://127.0.0.1:7890
export https_proxy=http://127.0.0.1:7890
export ALL_PROXY=socks5://127.0.0.1:7890

cd /Users/lizhenhe/vscode/esp32-GC9A01
pio run -t upload
```

首次会下载 platform / toolchain / framework，代理可明显加速。

成功标志：日志出现 `Hash of data verified` 与 `[SUCCESS]`，板子复位。

---

## 7. 验收

| 现象 | 含义 |
|------|------|
| 红绿蓝依次刷屏 | SPI + 驱动初始化成功 |
| 显示 GC9A01 / ESP32-C3 | 文字绘制正常 |
| 中心圆彩色闪烁 | `loop` 在跑，整体链路 OK |

可选串口监视：`pio device monitor -b 115200`，应看到 `Display OK`。

---

## 8. 曾失败再修复的一步

第一次使用 `GFX Library for Arduino@^1.5.0` 时报错：

```text
fatal error: esp32-hal-periman.h: No such file or directory
```

原因：1.5.x 需要 Arduino-ESP32 3.x，当前 PlatformIO `espressif32` 自带 2.x。

处理：改为 `@1.4.7`，清 `libdeps` 后重新 `pio run -t upload` → 成功。

---

## 9. 拟真模拟表盘（当前固件）

在点亮成功后，将 `src/main.cpp` 从色条 bring-up 改为模拟表盘。

### 画面元素

| 元素 | 说明 |
|------|------|
| 表圈 | 外圈多层细环 |
| 刻度 | 60 分刻度；整点加长加粗 |
| 数字 | 仅 12 / 3 / 6 / 9 |
| 指针 | 时针（粗）、分针（长）、秒针（红细针 + 短尾） |
| 轴心 | 双层圆心 |
| 文字 | `QUARTZ` / `ESP32-C3` |

### 时间来源（第 9 节当时）

- **软时钟**：上电从 `10:08:00` 起按 `millis()` 累加（经典表盘演示姿态）。

### 绘制策略

- 每秒整点变化时全屏重绘表盘 + 指针（无帧缓冲，避免指针残影）。
- 同一秒内 `delay(20)` 空转，降低 SPI 占用。

### 验收

| 现象 | 含义 |
|------|------|
| 表盘刻度与 12/3/6/9 清晰 | 静态面绘制 OK |
| 三针位置合理，秒针每秒跳动 | `loop` 与软时钟 OK |
| 串口 `Watch OK @ hh:mm:ss` | 初始化成功 |

烧录命令同第 6 节：`pio run -t upload`。

---

## 10. 模块化表盘 + Wi‑Fi NTP（当前）

将单文件表盘拆成可切换风格，并支持 NTP 真时；未配置 Wi‑Fi 时仍回退软时钟。

### 目录结构

```
src/
├── main.cpp                 # 编排：显示 / 时间 / 串口切面
├── config.h                 # 默认配置；可被 config.local.h 覆盖
├── config.local.h.example   # 复制为 config.local.h 填 Wi‑Fi
├── pins.h                   # 屏引脚
├── time/TimeService.*       # Wi‑Fi + NTP / 软时钟
└── face/
    ├── IWatchFace.h         # 表盘接口 render(gfx, tm)
    ├── FaceId.h             # Classic / Lume / Skeleton / Calendar
    ├── FaceRegistry.*       # 按 FaceId 取实例
    ├── gfx_util.h           # 共用极坐标 / 指针 / 刻度
    ├── FaceClassic.*
    ├── FaceLume.*           # 夜光
    ├── FaceSkeleton.*       # 镂空
    └── FaceCalendar.*       # 3 点日历窗
```

`src/config.local.h` 已加入 `.gitignore`，勿提交密码。

### 启用真实时间

```bash
cp src/config.local.h.example src/config.local.h
# 编辑 WIFI_SSID / WIFI_PASS，可选 DEFAULT_FACE
pio run -t upload
```

时区默认 `NTP_TZ_OFFSET_SEC = 8*3600`（CST）。串口应见 `NTP OK ...` 与 `Watch OK [NTP] ...`。

### 切换表盘

| 方式 | 操作 |
|------|------|
| 编译期 | `config.local.h` / `config.h`：`#define DEFAULT_FACE FaceId::Calendar`（**当前默认 4**） |
| 运行期 | 串口 `1`/`2`/`3`/`4` 或 `n` |

| 键 | 风格 |
|----|------|
| 1 | Classic 经典 |
| 2 | Lume 夜光 |
| 3 | Skeleton 镂空 |
| **4** | **Calendar 日历窗（默认）** |

新增风格：实现 `IWatchFace` → 注册到 `FaceRegistry` → 扩展 `FaceId`。

### Wi‑Fi 验证结论（补充）

- 已通：手机热点 `WatchESP`/`12345678` + `TX=8.5dBm` + `sleep=OFF` → NTP。
- **空载家宽（2026-08-15）**：拔掉 GC9A01 后连 `TP-LINK_D6B1` → mode=0 一次成功，IP `192.168.0.109`，RSSI **-43 dBm**，NTP OK。接屏时曾对该 SSID 握手失败 → 更像外设/供电干扰，非密码问题。有载对比待做。
- ESP SoftAP/BLE 配网在本板仍不稳定；详见 [HANDOFF.md](./HANDOFF.md)。

---

## 11. 空载家宽 STA 对比（2026-08-15）

目的：排除「外接屏 GPIO / 杜邦线干扰」是否导致连不上家宽。

1. 拔掉全部屏线，仅留 ESP32-C3 USB 供电。
2. `config.local.h` 设 `WIFI_SSID=TP-LINK_D6B1`（密码仅本地文件）。
3. `pio run -t upload`，串口 115200 观察。

**结果**：`STA try mode=0` → `Wi-Fi OK` → `NTP OK` → `Watch OK [NTP] Calendar`（约数秒）。

下一步：接回屏重复同一 SSID，确认有载是否再现握手失败。

---

## 时间线摘要

1. 硬件选型确认 → 2. 接线方案与颜色约定 → 3. 上电安全检查 →  
4. macOS 识别 USB（免驱）→ 5. 代理安装 PlatformIO →  
6. 写 bring-up 工程 → 7. 遇 GFX 版本冲突并钉版本 → 8. 烧录成功并点亮 →  
9. 拟真模拟表盘 → 10. 模块化多风格 + Wi‑Fi NTP →  
11. C3 Super Mini Wi‑Fi 排障（AUTH_EXPIRE / 热点反向连接）→ 默认表盘固定 Calendar →  
12. **拔屏空载**：家宽 `TP-LINK_D6B1` 可连且 NTP 成功（疑外设干扰）→  
13. Photo 表盘 + **EC11（CLK0/DT1/SW5）悬浮预览切面**（静态黑底、按键 Toast）→  
14. **Settings 高频菜单**（Face / Sync / TZ / Wi‑Fi / About）→  
15. **Crown** 照片表盘 + 预览圆形遮罩（防方形图四角漏出）。

更精简的交接信息见 [HANDOFF.md](./HANDOFF.md)。

---

## 12. EC11 旋钮切面（2026-08-22）

接线（与屏共用 3.3 / GND）：

| EC11 | GPIO |
|------|------|
| CLK | 0 |
| DT | 1 |
| SW | 5 |

固件：`src/input/Ec11.*` + `main`。

| 行为 | 说明 |
|------|------|
| 旋转 | 黑底、约 82% 缩小悬浮预览；**静态冻结**（不按秒刷新） |
| 短按 / 长按 | 确认全屏 / **进 Settings**；SW 自动判极性 |
| 性能 | 预览缩放用 TFT `writeAddrWindow` + `writePixels` 批量写出 |

串口：`EC11 OK …`；诊断命令 `e`。

---

## 13. Settings 高频菜单（2026-08-22）

长按旋钮进入；`src/ui/Settings.*`。

| 项 | 行为 |
|----|------|
| Face | 列表选面，立即 commit |
| Sync | `TimeService::syncNtpNow()` |
| Timezone | 旋转调 `UTC±N`（−12…+14） |
| Wi‑Fi | 显示 SSID/RSSI；Reconnect = 等手机热点（阻塞） |
| About | heap / 时间源 / IP |
| 退出 | 根菜单 Back、长按返回、或 15s 空闲 |

---

## 14. Crown 表盘 + 预览圆遮罩（2026-08-22）

- 新增 `FaceId::Crown`（串口 `6`）：`src/face/FaceCrown.*` + `assets/bg_crown_240.h`（240×240 RGB565）
- `flushPreviewFloating`：缩放写出时按半径裁圆，Photo / Crown 预览不再漏出方形四角
