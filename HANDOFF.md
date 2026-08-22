# Handoff — ESP32-C3 Super Mini + GC9A01

> 交接文档。下一会话可直接按本文继续，不必重走摸索过程。

## 当前状态（已验证）

| 项 | 状态 |
|----|------|
| 硬件接线 | 已接好并点亮 |
| macOS 识别 | Espressif USB JTAG/serial，无需额外驱动 |
| 工具链 | PlatformIO Core 6.1.19（`~/.platformio/penv/bin`） |
| 固件 | 模块化表盘；**Vista(8)** 可用；STA + NTP；**EC11 预览切面（ISR）+ Settings 已通** |
| Wi‑Fi | 热点 `WatchESP` 已通；家宽 `TP-LINK_D6B1` **空载（拔屏）已通** |
| 工程路径 | `/Users/lizhenhe/vscode/esp32-GC9A01` |

## 硬件

- **MCU**：ESP32-C3 Super Mini（USB-C，板标 V1601 / Super Mini）
- **屏**：1.28" TFT 240×240，驱动 IC **GC9A01**，7 针排针
- **连接**：杜邦线母对母

### 已采用引脚（与 `src/pins.h` 一致）

| 屏引脚 | ESP32-C3 | 杜邦线颜色 |
|--------|----------|------------|
| VCC | **3.3**（勿接 5V） | 红 |
| GND | **G** | 黑 |
| SCL | **6** | 橙 |
| SDA | **7** | 黄 |
| DC | **2** | 绿 |
| CS | **10** | 蓝 |
| RST | **3** | 紫 |

说明：信号脚可改，但必须与代码宏一致。尽量避开 GPIO 8/9（启动相关）。

### EC11（与 `src/pins.h` 一致）

| EC11 | ESP32-C3 |
|------|----------|
| CLK | **0** |
| DT | **1** |
| SW | **5** |
| + | **3.3** |
| GND | **G** |

交互：旋转 → **黑底缩小悬浮预览**（静态冻结）→ 短按确认全屏（停转约 4s 自动确认）→ **长按进 Settings**。  
SW 上电自动判极性；串口 `e` 诊断。驱动：`src/input/Ec11.*`（**正交用 CLK/DT 边沿 ISR**，按键仍 `poll()`）；菜单：`src/ui/Settings.*`。

Settings 高频项：Face / Sync(NTP) / Timezone / Wi‑Fi / **Seconds** / About；旋钮滚、短按进、长按返回；15s 空闲退出。

### 屏 PCB 备注

板上有「此模块可不接 CS、RST」字样；本工程仍接全，驱动更省事。

## 软件栈

- Platform：`espressif32`（当前 7.0.1）→ Arduino-ESP32 **2.0.17**
- Board：`esp32-c3-devkitm-1`，`board_build.partitions = huge_app.csv`
- Framework：Arduino
- 库：`GFX Library for Arduino@1.4.7`、`QRCode@0.0.1`  
  - **不要升 GFX 到 1.5.x**（需 Arduino-ESP32 3.x）
- USB CDC：`ARDUINO_USB_MODE=1` + `ARDUINO_USB_CDC_ON_BOOT=1`

## 工程结构

```
src/main.cpp                 # 编排 / 串口 / EC11 / Settings
src/config.h                 # 默认；DEFAULT_FACE（可被 local 覆盖）
src/config.local.h           # gitignore：Wi‑Fi 与可选覆盖
src/config.local.h.example
src/pins.h                   # 屏 + EC11
src/input/Ec11.*             # 旋钮：正交 ISR + 按键 poll 消抖
src/time/TimeService.*       # NTP / Soft / Manual / 运行时区
src/time/Lunar.h             # 农历 1900–2049（Vista 用）
src/wifi/WifiProvision.*     # STA 连接（8.5dBm、关省电、多 mode 重试）
src/ui/ProvQr.*              # 配网提示 / 二维码（历史 SoftAP 用）
src/ui/Settings.*            # 长按设置菜单
src/face/                    # Classic…Crown / Dash / Vista
src/weather/                 # Open-Meteo（含当日 min/max）
```

## 表盘

| 键 | 风格 | 说明 |
|----|------|------|
| 1 | Classic | 经典指针 |
| 2 | Lume | 夜光 |
| 3 | Skeleton | 镂空 |
| 4 | Calendar | 日历窗 |
| 5 | Photo | 照片背景 |
| 6 | Crown | 照片背景（皇冠肖像） |
| 7 | Dash | 多功能：时钟 + 周几/月日 + 天气 |
| **8** | **Vista** | **白底多功能：指针 + 天气/农历/日期 + 底部数字时**（本地常作默认） |

- 编译期默认：`config.h` 为 `Photo`；本机可用 `config.local.h` → `FaceId::Vista`
- 运行期：串口 `1`–`8` / `n` / `r`(刷天气)，或 EC11 旋转/短按
- Dash / Vista：Open‑Meteo（默认北京 lat/lon；Vista 另显示当日低/高与短天气文案）
- Vista：**不画**心率/步数/睡眠/电量（无对应硬件）；农历为 ASCII（无中文字库）如 `L7/10`
- Photo / Crown 预览：`flushPreviewFloating` 圆形遮罩，方形图四角不漏

## Wi‑Fi / NTP（已验证路径）

### A. 手机热点（接屏也稳）

**`WatchESP` / `12345678`（WPA2）** → ESP STA → NTP。

### B. 家宽空载（2026-08-15 验证）

拔掉 GC9A01（仅 USB 供电、无外接屏）后连 **`TP-LINK_D6B1`**：

| 项 | 结果 |
|----|------|
| 条件 | 空载、无杜邦线外设 |
| mode | **0（Plain）一次成功** |
| IP | `192.168.0.109` |
| RSSI | **-43 dBm** |
| NTP | OK → `Watch OK [NTP] Calendar` |
| 耗时 | 约数秒 |

密码等密钥只写在 `config.local.h`（gitignore），勿提交。

```text
Wi-Fi OK → NTP OK → Watch OK [NTP] Calendar …
```

**推论**：接屏时对该 SSID 的 `AUTH_EXPIRE` / `4WAY_HANDSHAKE_TIMEOUT` 更像是 **屏/杜邦线供电或 GPIO 干扰**，而非密码错或家宽完全不可用。有载对比尚未复测。

RF 调参（`WifiProvision.cpp`）：`WiFi.setTxPower(WIFI_POWER_8_5dBm)` + `WiFi.setSleep(WIFI_PS_NONE)`。

本地配置示例（勿提交）：

```bash
# src/config.local.h
#define WIFI_SSID "TP-LINK_D6B1"   # 或 WatchESP
#define WIFI_PASS "********"
#define DEFAULT_FACE FaceId::Calendar
```

串口辅助：`w SSID PASS` · `s` 跳过 Wi‑Fi · `t YYYY-MM-DD HH:MM:SS` 手动校时 · `p` 重新进热点等待。

### 踩坑（Wi‑Fi）

1. `AUTH_EXPIRE(2)` / `4WAY_HANDSHAKE_TIMEOUT(15)` **≠ 一定密码错**；C3 Super Mini 上极常见（射频/功率/天线/双频路由/**外设干扰**）。
2. 家宽 `TP-LINK_D6B1`：**空载可连**；接屏时曾多次握手失败。小米 IoT 能连不代表 ESP 接屏也能连。
3. ESP SoftAP / BLE 配网在本板不可靠（SoftAP 难被手机发现；BLE WiFiProv 曾 `abort`）。
4. 华为连 ESP 热点常失败；反向「手机开热点、ESP 去连」更稳。
5. 排障顺序：降功率 8.5dBm、关 sleep、专用 2.4G WPA2、**拔屏空载对比**（已证实有效）。

## 常用命令

```bash
ls /dev/cu.usbmodem*
export PATH="$HOME/.platformio/penv/bin:$PATH"
export http_proxy=http://127.0.0.1:7890 https_proxy=http://127.0.0.1:7890
cd /Users/lizhenhe/vscode/esp32-GC9A01
pio run -t upload
# 串口：python 读 /dev/cu.usbmodem* 115200，或 pio device monitor
```

## 功耗与降耗手段

当前板子主要耗电：**常亮 GC9A01 + Wi‑Fi STA**。软件侧已落地一项：

| 项 | 状态 | 说明 |
|----|------|------|
| **Seconds ON/OFF** | **已做** | Settings 根菜单短按切换；NVS 持久化（`WatchPrefs`）。OFF：不画秒针/秒数，约**每分钟**全屏刷新，空闲 `delay(50)` |

按收益大致排序的其他手段（多数未实现，供后续）：

1. **降刷新频率**（已做关秒）— SPI 全屏刷新是大头之一；关秒从 ~1 Hz 降到 ~1/60 Hz。
2. **背光 PWM / 空闲灭屏** — 需 BL 脚或模组可控背光；本接线若无背光 GPIO，收益有限。
3. **Wi‑Fi 策略** — NTP 成功后关 STA 或拉长重连；常连热点最耗电。天气已约 20 min 拉一次，可再拉长或仅 Dash 才拉。
4. **降 CPU 频率** — 空闲 `setCpuFrequencyMhz(80)` 等（需验证 Wi‑Fi / 显示稳定）。
5. **Light sleep** — 分针间隙短睡，定时器 / EC11 GPIO 唤醒；要处理好 Wi‑Fi 与旋钮。
6. **少画全屏** — 脏区更新；Photo/Crown 全图拷贝更费，可考虑缓存静态底。
7. **RF** — 已用较低 TX（约 8.5 dBm）+ 关 modem sleep（握手期）；保持即可。
8. **硬件** — 稳压效率、杜邦压降、电池内阻；屏供电干净可减少 Wi‑Fi 重试（间接省电）。

## 后续可做

- Settings：亮度 / 休眠（需背光硬件）；Wi‑Fi Reconnect 非阻塞化
- Calendar / Photo 表盘专用旋钮交互；断网保时（RTC）
- Dash 天气：更多城市预设 / Settings 里手动刷新
- BLE / App 配网（本板历史不稳定，优先官方 ESP BLE Prov 验证）
- **接回 GC9A01 有载复测** `TP-LINK_D6B1`（对照空载成功）
- 有载仍失败时：缩短杜邦线、加近端去耦、或家宽单独 2.4G IoT SSID
- Arduino_GFX 1.5+ 需换 pioarduino（ESP32 core 3.x）

## 其他踩坑

1. macOS 对 C3 原生 USB 一般免驱。  
2. 下载走代理 `7890`。  
3. 屏供电只用 **3.3V**。  
4. Wi‑Fi 密码只放 `config.local.h`。

## EC11 表盘旋转失效（2026-08-22 排查）

### 现象

- 表盘界面：**旋转大多切不了面**（偶发成功）；长按进 **Settings 后旋转正常**（菜单滚动跟手）。
- 串口 `n` / `1`–`8` 切面正常 → **软件切面路径与 Face 注册没坏**。
- 串口 `e`：`settings=0 preview=0`，SW 电平正常 → **不是卡在 Settings、也不是按键死粘**。

### 排查思路（由易到难）

1. **排除硬件**：Settings 内旋转可靠 ⇒ CLK/DT 接线与 EC11 本体基本正常；勿先换线。
2. **排除 Face 逻辑**：串口 `n` 能切 ⇒ `nextFace` / `getFace` / `selectFace` OK。
3. **对比两条路径的差异**：
   - Settings：几乎只画文字列表，SPI 很短，主循环 `delay(2)` 高频 `Ec11::poll()`。
   - 表盘：每秒（或预览时）`render` + `canvas->flush()` / `flushPreviewFloating`（全屏 240×240 缩放拷贝），**SPI 阻塞可达数十～上百 ms**。
4. **根因**：原实现在 `loop()` 里 **轮询** 正交 Gray 码。SPI 阻塞期间若漏掉中间电平跳变，`accum` 失步，之后整格 detent 可能永远凑不齐 ±4 → **表现为「转了没反应」**；偶发对上相位就「有时候行」。Settings 几乎不堵 SPI，故表象像「只有表盘坏了」。
5. **次要加重因素**：Vista 等复杂全屏绘制会拉长阻塞窗口，漏脉冲概率上升（但根因是轮询，不是 Vista 逻辑本身）。

### 解决

- `src/input/Ec11.cpp`：CLK/DT 使用 **`attachInterrupt(..., CHANGE)` ISR** 更新 `prevAb` / `accum` / `rotPending`；`poll()` 只负责 SW 消抖。
- `takeRotation()` 读写 `rotPending` 时短关中断。
- 验证：表盘旋转应稳定出缩小预览；Settings 行为不变。串口启动日志含 `isr=1`。

### 以后若再「旋钮失灵」

| 观察 | 更可能 |
|------|--------|
| Settings 也转不动 | 硬件 / 引脚 / 上拉 |
| 仅表盘、串口 `n` 仍可切 | 再查是否有人改回纯 poll、或 ISR 被卸 |
| 转一下跳很多面 | 正常（阻塞期间 ISR 已累加多格 detent） |

## 会话复盘（2026-08-22：Crown / Dash / Seconds）

本次会话里多次出现「以为源码是 A、实际磁盘是 B」「写完又对不上、编译反复不对」的情况。根因不是硬件，而是**工作流与工具使用**：

1. **未以磁盘为准核对 API**  
   改 Settings / Weather / Face 时，有时凭对话记忆或单次 Read 印象写下一套符号（函数名、Hooks 字段、include 路径），与仓库真实命名不一致。正确做法：改前用 `cat` / `python Path.read_text()` 确认现用符号，再写。

2. **同一文件多次 Write 互相覆盖**  
   FaceDash、WeatherService、Settings 曾连续写多版，中间版本 API 不一致（例如 `poll()` 返回值、`refreshNow`、字段名）。后一次 Write 若只改一半，会留下半新旧状态，编译或链接阶段才爆。

3. **PlatformIO 增量编译掩盖坏源码**  
   `pio run` 有时未重编刚改的 `.cpp`（时间戳 / 缓存），旧 `.o` 仍能链上，表现为「源码已坏但 SUCCESS」。应用 `rm …/*.o` 或 `pio run -t clean` 后再编，才能暴露真实错误。

4. **粗糙字符串替换污染邻接代码**  
   往 `main.cpp` 插 `WatchPrefs::begin()` 时用过宽的 `Settings::begin(` 替换，一度差点写进错误函数体。大文件改动应小范围、带足够上下文，改完立刻打印相关片段核对。

5. **「看起来像编译不过」有时是代理流程噪音**  
   部分失败来自脚本/tee/路径假设，而非编译器报错；应以 `pio run` 末尾 `SUCCESS`/`Error` 与首条 `error:` 为准。

**以后约定**：磁盘源码是唯一真相 → 改前核对符号 → 小步改、立刻重编（必要时清 `.o`）→ 改后抽查关键片段。

## 会话复盘（2026-08-22：Vista + EC11 ISR）

1. 新增 **Vista** 白底多功能表盘（去掉心率/步数/睡眠/电量等无硬件项）；天气补当日 min/max；农历 `Lunar.h`。
2. EC11 表盘旋转失灵：用「Settings 正常 / 串口切面正常」快速排除硬件与 Face 注册，定位到 **SPI 长阻塞下轮询正交失步**，改为 **ISR**（见上一节）。
3. 布局迭代：信息区字号/粗细、温度靠中、图标下短天气文案；避免 `textSize 2` 双描过重拖慢刷新。
