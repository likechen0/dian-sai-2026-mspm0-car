# lkc - 新扩展板感为八路灰度循迹版

这是以当前最新版 `heyvhao` 为母版新建的独立 CCS 工程。导航状态机、PD 循迹、编码器和 MS901M 解析逻辑保持不变，底层硬件按照官方原理图、PCB 图和实板连通测量重新配置。完整连接基准见 [`PINOUT.md`](PINOUT.md)。

## 当前状态

- 工程名：`lkc`
- 主控：MSPM0G3507
- 电机驱动：TB6612
- 循迹传感器：感为无 MCU 八路灰度
- PWM：左轮 TIMG7、右轮 TIMG12，周期均为 1000，占空比命令范围 0..10000
- OLED：显示 U6/U16 编码器速度 / 八路 0/1 线状态 / 八路原始 ADC / 跟踪误差
- 编译输出：`Debug/lkc.out`

## 2026-07-13 原理图与实板复核

已对照资料目录中的 `原理图.pdf`、`PCB图.png` 和实板信号完成复核：

- `PB14` 实际连接 TB6612 的 `PWMA`，使用 `TIMG12_CCP1`。
- `PA7` 实际连接 TB6612 的 `PWMB`，使用 `TIMG7_CCP1`。
- `PA18` 没有连接 TB6612 PWM，只是普通引出脚。
- 方向脚、U6/U16 编码器、八路灰度、OLED 和 UART0 均与当前 SysConfig 一致。

旧自动提取表中的 `PWMA=PA18、PWMB=PB14` 是错误记录，已经废弃。此前 U16 的
BO1/BO2 没有输出，正是因为真正的 PWMB/PA7 没有收到 PWM。详细核对结果和测量点见
[`PINOUT.md`](PINOUT.md)。

## 本版本备注

这一版是新扩展板的感为传感器调车版，OLED 可查看 U6/U16 两轮编码器速度、八路原始 ADC、八路 0/1 识别和循迹误差。

- **新增双宏开关**：`TRACKING_RUN_MODE`（调试/巡线）和 `TRACKING_DISPLAY_MODE`（01检查/ADC获取），详见下方「双宏开关」章节。
- 当前循迹参数入口在 `Tracking.h`：`TRACKING_BASE_SPEED`、`TRACKING_KP_NUM/DEN`、`TRACKING_KD_NUM/DEN`。
- 当前功能开关入口在 `InertialNav.c`：离线追线、左右固定转、脱线导航转。
- 目前三个高级开关默认都是关闭：先保证普通循迹稳定，再逐个打开测试。

## 左右轮通道

按小车实际安装位置定义逻辑左右：

| 软件命令 | 实际使用通道 |
| --- | --- |
| 左轮 | U16 / R1：PWMB + BIN1/BIN2 + A2/B2 |
| 右轮 | U6 / L1：PWMA + AIN1/AIN2 + A1/B1 |

也就是说，`Motor_SetSpeed_L()` 驱动 B 桥，`Motor_SetSpeed_R()` 驱动 A 桥。
原理图中 A 路芯片输出与 `AO1/AO2` 网络已经交叉连接，而 B 路没有交叉；
这是板级方向处理，当前代码不再额外反转任何一侧电机。

## 接线表

### TB6612

| 软件侧 | TB6612 功能 | MSPM0G3507 |
| --- | --- | --- |
| 左轮 PWM | PWMB | PA7 / TIMG7_CCP1 |
| 右轮 PWM | PWMA | PB14 / TIMG12_CCP1 |
| 左轮方向 | BIN1 | PB9 |
| 左轮方向 | BIN2 | PB6 |
| 右轮方向 | AIN1 | PA13 |
| 右轮方向 | AIN2 | PB10 |
| 使能 | STBY | 硬接 +5V |

这里的 PWM 网络已经同时通过原理图和 PCB 实测确认：`PB14` 连接 `PWMA`，`PA7`
连接 `PWMB`。旧引脚文档中把 `PA18/PB14` 写成两路 PWM 的记录不适用于这块实板。

### 四个电机接口

原理图中的 `L1/L2/R1/R2` 与 PCB 丝印元件号对应如下：

| PCB 接口 | 原理图名称 | 驱动通道 | 编码器反馈 | 两驱用途 |
| --- | --- | --- | --- | --- |
| U6 | L1 | A 桥 AO1/AO2 | A1/B1，PB11/PB12 | 逻辑右轮，必须使用 |
| U7 | L2 | 与 U6 同一个 A 桥 | 无，Pin2..5 为 NC | 不用于两驱编码器轮 |
| U16 | R1 | B 桥 BO1/BO2 | A2/B2，PB4/PB5 | 逻辑左轮，必须使用 |
| U9 | R2 | 与 U16 同一个 B 桥 | 无，Pin2..5 为 NC | 不用于两驱编码器轮 |

两驱车必须把两根六芯电机排线分别插在 `U6` 和 `U16`。如果接 `U6+U7`，
两个电机会被同一个 A 桥同时控制，无法差速；如果接 U7 或 U9，对应电机虽然能转，
但编码器读数必然为 0。

`U6` 六针定义：Pin6=AO1、Pin5=3.3V、Pin4=A1、Pin3=B1、Pin2=GND、Pin1=AO2。

`U16` 六针定义：Pin6=BO1、Pin5=3.3V、Pin4=A2、Pin3=B2、Pin2=GND、Pin1=BO2。

### 感为八路灰度

| 灰度通道 | MSPM0G3507 / ADC |
| --- | --- |
| S1 / OUT_1 | PB19 / ADC1_CH6 |
| S2 / OUT_2 | PB17 / ADC1_CH4 |
| S3 / OUT_3 | PA16 / ADC1_CH1 |
| S4 / OUT_4 | PA14 / ADC0_CH12 |
| S5 / OUT_5 | PB20 / ADC0_CH6 |
| S6 / OUT_6 | PB25 / ADC0_CH4 |
| S7 / OUT_7 | PA25 / ADC0_CH2 |
| S8 / OUT_8 | PA27 / ADC0_CH0 |
| VCC | 扩展板 H10 的 5V |
| GND | 扩展板 H10 的 GND |

八路都是独立模拟输入，不再使用旧版的 `OUT + AD0/AD1/AD2` 轮询接法。MSPM0 ADC 引脚电压不得超过 3.3V，首次接入其他型号传感器前应先测量其 OUT 最大电压。

### 编码器

| 软件侧 | MSPM0G3507 |
| --- | --- |
| LEFT_A / A2 | PB4 |
| LEFT_B / B2 | PB5 |
| RIGHT_A / A1 | PB11 |
| RIGHT_B / B1 | PB12 |

四路编码器都位于 GPIOB，A 相使用双边沿中断，B 相用于判断方向。
软件把 U16 的 A2/B2 作为左轮，把 U6 的 A1/B1 作为右轮。

实测整车向前时两侧原始编码器符号相反，这是镜像安装造成的正常现象。当前尚未加入
轮速 PI；正式加入 PI 或使用里程计前，需要在 `Encoder.h` 中把实际反号的一侧
`ENCODER_*_DIR` 改为 `-1`，使向前时左右反馈都为正。

### MS901M / ATK-IMU901

| UART 功能 | MSPM0G3507 |
| --- | --- |
| UART0_TX | PA10 |
| UART0_RX | PA11 |

实际接线要交叉：模块 TX 接 MSP 的 RX，也就是 PA11；模块 RX 接 MSP 的 TX，也就是 PA10。

### OLED I2C

| 功能 | MSPM0G3507 |
| --- | --- |
| SCL | PA1 |
| SDA | PA0 |
| VCC | 扩展板 OLED 插座 5V |
| GND | 扩展板 OLED 插座 GND |

### 其他

| 功能 | MSPM0G3507 |
| --- | --- |
| RUN_LED | PA15 / LED1（低电平点亮） |

## 灰度传感器与循迹逻辑

当前轮速属于开环控制：导航状态机和循迹 PD 直接给出左右 PWM，编码器只用于 OLED
显示和里程信息，尚未反向修正 PWM。因此本工程是“循迹位置闭环 + 轮速开环”。后续若
加入双轮速度 PI，建议保留当前 PWM 作为前馈，再用两个独立 PI 小幅修正左右轮差异。

当前工程分成两层：

- `Tracking.c`：负责读取感为 8 路灰度、归一化 ADC、判断黑线、计算线的位置误差。
- `InertialNav.c`：负责导航状态机，根据灰度结果决定直行、循迹、丢线追回或陀螺仪转向。

主循环在 `empty.c` 中调用：

```c
while (1) {
#if TRACKING_RUN_MODE
    /* 巡线模式：读传感器 → PD循迹/直行/陀螺仪转向 */
    NAV_ControlStep();
#else
    /* 调试模式：只采集传感器数据刷新 OLED 显示，小车静止 */
    Tracking_Value_Acquire();
    Tracking_CalcError();
    Motor_Stop();
#endif
    Delay_ms(ENCODER_SAMPLE_MS);
    Encoder_Update();
    Display_Update();
    DL_GPIO_togglePins(RUN_LED_PORT, RUN_LED_LED_PIN);
}
```

所以实际小车控制入口由 `TRACKING_RUN_MODE` 决定：
- `0`：调试模式，小车静止，只刷新 OLED
- `1`：巡线模式，走 `NAV_ControlStep()` 导航

## 双宏开关

工程通过两个编译期宏控制运行行为，改完重新编译烧录即可，无需改动代码。

### TRACKING_RUN_MODE（位于 `empty.c`）

| 值 | 模式 | 小车状态 | 说明 |
| --- | --- | --- | --- |
| `0` | 调试模式 | 静止 | 采集传感器，刷新 OLED，电机不转 |
| `1` | 巡线模式 | 运行 | `NAV_ControlStep()` 导航状态机 |

当前默认值：`1U`（巡线模式）。

### TRACKING_DISPLAY_MODE（位于 `Display.h`，仅调试模式生效）

| 值 | 模式 | OLED 显示内容 | 用途 |
| --- | --- | --- | --- |
| `0` | 01 检查 | 八路 0/1 线状态 + 误差 | 验证每路传感器阈值是否正常 |
| `1` | ADC 获取 | 八路原始 ADC 值 | 手工采集黑白标定值 |

当前默认值：`0U`（01 检查模式）。

在模式 `0` 下，OLED 第 1 行显示 U16 左轮、第二行显示 U6 右轮最近约
100 ms 的累计有符号脉冲数，例如 `L:+0200 C/100ms`。该值不是 RPM；
它用于直接确认两侧编码器是否都有反馈以及方向符号是否一致。

### 典型工作流

```
1. TRACKING_RUN_MODE=0, TRACKING_DISPLAY_MODE=1
   → 小车静止，OLED 显示八路原始 ADC
   → 传感器放白底 → 目视记录 8 个白值 → 写入 Tracking.c 的 Tracking_CalWhite[]
   → 传感器放黑线 → 目视记录 8 个黑值 → 写入 Tracking.c 的 Tracking_CalBlack[]

2. TRACKING_RUN_MODE=0, TRACKING_DISPLAY_MODE=0
   → 小车静止，OLED 显示八路 0/1 线状态
   → 放白底上对应位应为 0，放黑线上对应位应为 1
   → 白底误判 1 → 增大 TRACKING_LINE_THRESHOLD
   → 黑线误判 0 → 减小 TRACKING_LINE_THRESHOLD

3. TRACKING_RUN_MODE=1
   → 正式巡线，调 PD 参数
```

### 感为 8 路采样

新扩展板把八路模拟量直接接到两个 ADC。程序每个控制周期分别启动一次序列转换：

- `ADC1` 的 MEM0..MEM2 同时负责 S1..S3。
- `ADC0` 的 MEM0..MEM4 同时负责 S4..S8。
- 每个通道启用 4 次硬件平均，降低电机噪声和地面纹理造成的瞬时波动。

因此一次采集即可得到完整八路数据，不再切换地址线，采样时序也比旧版更稳定。

### 黑白归一化

每一路都有一组黑白标定值：

```c
Tracking_CalWhite[8]
Tracking_CalBlack[8]
```

程序把原始 ADC 归一化成：

```text
0    表示黑
4096 表示白
```

白底黑线时，代码使用：

```c
black_strength = 4096 - normal;
```

当黑线强度超过 `TRACKING_LINE_THRESHOLD` 时，认为该路看到黑线。当前阈值是：

```c
#define TRACKING_LINE_THRESHOLD 1000U
```

### 误差计算

8 路传感器的权值如下：

```text
S1     S2     S3     S4    S5    S6    S7    S8
-3500 -2500 -1500 -500   500   1500  2500  3500
```

含义是：

- `Tracking_Error < 0`：黑线偏左。
- `Tracking_Error > 0`：黑线偏右。
- `Tracking_Error = 0`：黑线在中间附近。

误差不是只看某一个传感器，而是用每一路黑线强度做加权平均。这样当黑线同时压到多路传感器时，误差会更平滑。

### 普通循迹

普通有线时，状态机会调用 `NAV_LineFollowFromCurrentError()`，核心逻辑是：

```c
correction = error * Kp + derivative * Kd;
leftSpeed  = baseSpeed + correction;
rightSpeed = baseSpeed - correction;
```

当前参数在 `Tracking.h` 中：

```c
#define TRACKING_BASE_SPEED 2000
#define TRACKING_KP_NUM     40
#define TRACKING_KP_DEN     100
#define TRACKING_KD_NUM     10
#define TRACKING_KD_DEN     10
```

也就是说当前 `Kp = 0.4`，`Kd = 1.0`，属于 PD 控制。

误差越大时，代码会自动降低基础速度，避免弯道速度太快冲出线。当前最低循迹速度限制在 `NAV_TRACK_MIN_SPEED = 1200`。

### 开关与调参速查

PID/PD 调参位置在 `Tracking.h`：

| 参数 | 作用 |
| --- | --- |
| `TRACKING_BASE_SPEED` | 普通循迹基础速度 |
| `TRACKING_KP_NUM / TRACKING_KP_DEN` | P 项，越大越积极往线拉 |
| `TRACKING_KD_NUM / TRACKING_KD_DEN` | D 项，抑制快速摆动，当前为 1.0 |
| `TRACKING_LINE_THRESHOLD` | 黑线识别阈值 |

导航功能开关位置在 `InertialNav.c`：

| 开关 | 当前值 | 作用 |
| --- | --- | --- |
| `NAV_LOST_SEARCH_ENABLE` | `0U` | 离线追线：丢线后按最后外侧 S1/S8 方向低速追回 |
| `NAV_OUTER_GUARD_ENABLE` | `0U` | 左右固定转：S1/S8 单独识别到线时给固定差速修正 |
| `NAV_ENABLE_LINE_TURN` | `0U` | 脱线导航转：有线到无线后按计数奇偶触发陀螺仪 45 度转向 |

`0U` 表示关闭，`1U` 表示打开。建议顺序是先调普通 P/PD 循迹，再打开左右固定转，最后再测试离线追线或脱线导航转。

### 外侧保护

当前 `NAV_OUTER_GUARD_ENABLE = 0U`，外侧保护是关闭的。

逻辑是：

- 只有 S1 看到线时，给一个固定左侧修正。
- 只有 S8 看到线时，给一个固定右侧修正。
- 这不是进入强制转向状态，只是当前控制周期给一次较强差速。

这个逻辑用于弯道或边缘压线时快速把车拉回来。

### 丢线追回

当前 `NAV_LOST_SEARCH_ENABLE = 0U`，丢线追回是关闭的。

逻辑是：

- 如果最后一次看到线的是左外侧 S1，之后丢线，就往左侧追回。
- 如果最后一次看到线的是右外侧 S8，之后丢线，就往右侧追回。
- 如果没有记录到外侧方向，丢线时就按白地直行处理。

追回时使用较小前进速度叠加转向差速：

```c
#define NAV_LOST_SEARCH_FORWARD_SPEED 200
#define NAV_LOST_SEARCH_TURN_SPEED    900
```

### 45 度转弯

当前 `NAV_ENABLE_LINE_TURN = 0U`，所以“有线到没线后，奇数左转 45 度、偶数右转 45 度”的功能是关闭的。

不过稳定丢线计数仍然保留，OLED 上的计数可以用来观察小车经过黑线的次数。

## 调试入口

### 黑白标定值采集

1. `empty.c` 中设 `TRACKING_RUN_MODE = 0U`
2. `Display.h` 中设 `TRACKING_DISPLAY_MODE = 1U`
3. 编译烧录，小车静止，OLED 显示八路原始 ADC
4. 传感器放白色地面 → 目视记录 8 个值 → 写入 `Tracking.c` 顶部 `Tracking_CalWhite[8]`
5. 传感器放黑色线上 → 目视记录 8 个值 → 写入 `Tracking.c` 顶部 `Tracking_CalBlack[8]`
6. `Display.h` 中改回 `TRACKING_DISPLAY_MODE = 0U`
7. 重新编译烧录，进入 01 检查模式验证黑白区分是否正常

### 阈值验证

1. `TRACKING_RUN_MODE = 0U`，`TRACKING_DISPLAY_MODE = 0U`
2. OLED 第 3 行 `S:` 显示八路 0/1 状态
3. 放白底上 → 8 位应全为 0
4. 放黑线上 → 对应位应为 1
5. 如果白底误判为 1 → 增大 `Tracking.h` 中的 `TRACKING_LINE_THRESHOLD`
6. 如果黑线误判为 0 → 减小 `Tracking.h` 中的 `TRACKING_LINE_THRESHOLD`

### 正式巡线

1. `empty.c` 中设 `TRACKING_RUN_MODE = 1U`
2. 编译烧录，小车正常运行导航状态机
3. 在 `Tracking.h` 中调 PD 参数

感为黑白校准值在 `Tracking.c` 顶部：

```c
Tracking_CalWhite[8]
Tracking_CalBlack[8]
```

这些值需要按你当前传感器高度、赛道材质和供电重新测。
