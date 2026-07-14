# heyvhao - 感为八路灰度圆环 PID 循迹版

这是从 `ganv` 新建出来的 MSPM0G3507 小车工程，功能逻辑保持感为八路灰度循迹版本不变，主要工作是按新的 `pinout.md` 重新映射引脚。

## 当前状态

- 工程名：`heyvhao`
- 主控：MSPM0G3507
- 电机驱动：TB6612
- 循迹传感器：感为无 MCU 八路灰度
- PWM：TIMG0，周期 1000，占空比命令范围 0..10000
- 循迹控制：带条件积分、积分分离和限幅保护的 PID
- OLED：显示八路 0/1 线状态 / 八路原始 ADC / 陀螺仪角度 / 跟踪误差
- 编译输出：`Debug/heyvhao.out`

## 本版本备注

这一版是在感为传感器调车版基础上加入圆环 PID。OLED 用于观察传感器识别、陀螺仪
姿态和循迹误差，积分项用于补偿持续圆弧所需的固定转向偏置。

- **新增双宏开关**：`TRACKING_RUN_MODE`（调试/巡线）和 `TRACKING_DISPLAY_MODE`（01检查/ADC获取），详见下方「双宏开关」章节。
- 当前循迹参数入口在 `Tracking.h`：`TRACKING_BASE_SPEED`、`TRACKING_KP_NUM/DEN`、`TRACKING_KI_NUM/DEN`、`TRACKING_KD_NUM/DEN`。
- 普通循迹已经由 PD 升级为带条件积分和抗饱和的 PID，用 I 项补偿圆环持续曲率造成的稳态偏差。
- 当前功能开关入口在 `InertialNav.c`：离线追线、左右固定转、脱线导航转。
- 目前三个高级开关默认都是关闭：先保证普通循迹稳定，再逐个打开测试。

## 左右轮通道

当前已经还原到左右轮未互换的上一版：

| 软件命令 | 实际使用通道 |
| --- | --- |
| 左轮 | PWMA + AIN1/AIN2 + 左编码器 |
| 右轮 | PWMB + BIN1/BIN2 + 右编码器 |

也就是说，`Motor_SetSpeed_L()` 驱动 A 桥，`Motor_SetSpeed_R()` 驱动 B 桥。

## 接线表

### TB6612

| 软件侧 | TB6612 功能 | MSPM0G3507 |
| --- | --- | --- |
| 左轮 PWM | PWMA | PA12 / TIMG0_CCP0 |
| 右轮 PWM | PWMB | PA13 / TIMG0_CCP1 |
| 左轮方向 | AIN1 | PB17 |
| 左轮方向 | AIN2 | PB19 |
| 右轮方向 | BIN1 | PA16 |
| 右轮方向 | BIN2 | PB24 |
| 使能 | STBY | 硬接 +5V |

### 感为八路灰度

| 功能 | MSPM0G3507 |
| --- | --- |
| OUT | PA18 / ADC1_CH3 |
| AD0 | PB25 |
| AD1 | PB18 |
| AD2 | PB21 |
| EN | 悬空或 GND |
| VCC | 5V |
| GND | MSP GND |

注意：这里必须是 `ADC1_CH3`，不是 `ADC0_CH3`。

### 编码器

| 软件侧 | MSPM0G3507 |
| --- | --- |
| LEFT_A | PA26 |
| LEFT_B | PA27 |
| RIGHT_A | PA25 |
| RIGHT_B | PA14 |

源码里仍保留 `ENCODER_PORTB_*` 这个宏名前缀，是为了少改已有代码；实际生成端口是 GPIOA。

### MS901M / ATK-IMU901

| UART 功能 | MSPM0G3507 |
| --- | --- |
| UART0_TX | PA0 |
| UART0_RX | PA1 |

实际接线要交叉：模块 TX 接 MSP 的 RX，也就是 PA1；模块 RX 接 MSP 的 TX，也就是 PA0。

### OLED I2C

| 功能 | MSPM0G3507 |
| --- | --- |
| SCL | PA31 |
| SDA | PA28 |
| VCC | 按 OLED 模块要求 |
| GND | MSP GND |

### 其他

| 功能 | MSPM0G3507 |
| --- | --- |
| RUN_LED | PB27 |

## 灰度传感器与循迹逻辑

当前工程分成两层：

- `Tracking.c`：负责读取感为 8 路灰度、归一化 ADC、判断黑线、计算线的位置误差。
- `InertialNav.c`：负责导航状态机，根据灰度结果决定直行、循迹、丢线追回或陀螺仪转向。

主循环在 `empty.c` 中调用：

```c
while (1) {
#if TRACKING_RUN_MODE
    /* 巡线模式：读传感器 → PID循迹/直行/陀螺仪转向 */
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
   → 正式巡线，调 PID 参数
```

### 感为 8 路采样

感为无 MCU 八路灰度模块是 8 路共用一个模拟输出：

- `OUT` 输出模拟灰度电压，接 `PA18 / ADC1_CH3`。
- `AD0/AD1/AD2` 是三位地址选择线，分别接 `PB25/PB18/PB21`。
- 程序通过改变 `AD0/AD1/AD2`，依次选择 S1 到 S8，然后读取 ADC。

每一路采样时，代码会读 5 次 ADC，丢掉前 2 次，只平均后 3 次。这样做是为了等模拟通道切换后稳定下来。

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
correction = error * Kp + integral * Ki + derivative * Kd;
leftSpeed  = baseSpeed + correction;
rightSpeed = baseSpeed - correction;
```

当前参数在 `Tracking.h` 中：

```c
#define TRACKING_BASE_SPEED 2000
#define TRACKING_KP_NUM     21
#define TRACKING_KP_DEN     100
#define TRACKING_KI_NUM     3
#define TRACKING_KI_DEN     200
#define TRACKING_KD_NUM     12
#define TRACKING_KD_DEN     10
```

也就是说当前有效值为 `Kp = 0.21`、`Ki = 0.015/控制周期`、`Kd = 1.2`。I 项只在有线且
误差绝对值不超过 1200 时工作，输出限幅为 `±600`；误差过大、丢线、外侧固定修正、
离线追线或陀螺仪转向时会清零，避免积分饱和。

### 圆环条件积分

积分并不是任何时候都无条件累加：

- `|error| <= 40`：保持已有 I 项，不继续累计中心噪声。
- `40 < |error| <= 1200`：累加误差，逐渐建立圆环所需的固定差速。
- `|error| > 1200`：清零 I 项，由 P、D 负责快速追回。
- 误差进入相反方向：清除上一段圆弧留下的积分，再向新方向积分。
- 丢线、外侧固定修正、离线追线、陀螺仪转向：立即清零。

当前积分误差限幅为 `±60000`，换算后的 I 项输出再限制为 `±600`，因此 I 项不会
无限增大并压过 P、D。若圆环仍持续偏向一侧，可继续增大 `Ki`；若出圆环后回摆明显，
应减小 `Ki` 或降低 `TRACKING_I_OUTPUT_LIMIT`。

误差越大时，代码会自动降低基础速度，避免弯道速度太快冲出线。当前最低循迹速度限制在 `NAV_TRACK_MIN_SPEED = 1200`。

### 开关与调参速查

PID 调参位置在 `Tracking.h`：

| 参数 | 作用 |
| --- | --- |
| `TRACKING_BASE_SPEED` | 普通循迹基础速度 |
| `TRACKING_KP_NUM / TRACKING_KP_DEN` | P 项，越大越积极往线拉 |
| `TRACKING_KI_NUM / TRACKING_KI_DEN` | I 项，消除圆环中的持续偏差，当前为 0.015/周期 |
| `TRACKING_KD_NUM / TRACKING_KD_DEN` | D 项，抑制摆动，当前为 1.2 |
| `TRACKING_I_ACTIVE_ERROR` | 允许积分的最大误差，当前为 1200 |
| `TRACKING_I_OUTPUT_LIMIT` | I 项最大差速贡献，当前为 ±600 |
| `TRACKING_LINE_THRESHOLD` | 黑线识别阈值 |

导航功能开关位置在 `InertialNav.c`：

| 开关 | 当前值 | 作用 |
| --- | --- | --- |
| `NAV_LOST_SEARCH_ENABLE` | `0U` | 离线追线：丢线后按最后外侧 S1/S8 方向低速追回 |
| `NAV_OUTER_GUARD_ENABLE` | `0U` | 左右固定转：S1/S8 单独识别到线时给固定差速修正 |
| `NAV_ENABLE_LINE_TURN` | `0U` | 脱线导航转：有线到无线后按计数奇偶触发陀螺仪 45 度转向 |

`0U` 表示关闭，`1U` 表示打开。建议顺序是先调普通 PID 循迹，再打开左右固定转，最后再测试离线追线或脱线导航转。

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
3. 在 `Tracking.h` 中调 PID 参数

感为黑白校准值在 `Tracking.c` 顶部：

```c
Tracking_CalWhite[8]
Tracking_CalBlack[8]
```

这些值需要按你当前传感器高度、赛道材质和供电重新测。
