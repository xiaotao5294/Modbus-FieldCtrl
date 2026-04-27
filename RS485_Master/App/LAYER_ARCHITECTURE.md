# RS485_Master 主机分层架构（可直接用于 FreeRTOS 迁移）

## 1. 分层目标

本分层将主机工程按职责拆分为四层，确保后续 FreeRTOS 迁移时：

1. 硬件耦合点最小化。
2. 协议与调度逻辑可独立复用。
3. 任务逻辑与应用入口边界清晰。
4. 模块依赖方向单向、可审计、可测试。

## 2. 目录分层结果（已落地）

### 底层驱动层（Driver Layer）
路径：`App/Src/layer_driver`

文件：
- `app_preflight.c`：中断优先级统一、资源锁基础能力。
- `rs485_lock.c`：共享状态临界区抽象（当前裸机，后续可映射到 FreeRTOS mutex）。

职责：
- 提供平台相关最小基础设施。
- 不承载业务协议和流程控制。

### 中间层（Middle Layer）
路径：`App/Src/layer_middle`

文件：
- `rs485_protocol.c`：Modbus CRC 与协议基础。
- `rs485_codec.c`：Modbus 请求/响应编解码。
- `rs485_scheduler.c`：超时、重试、在线离线调度。

职责：
- 只处理协议和算法，不直接控制应用流程。
- 不依赖控制台命令、业务动作决策。

### 任务逻辑层（Task Logic Layer）
路径：`App/Src/layer_task`

文件：
- `task_rs485.c`：主机通信状态机、事务推进、诊断快照。
- `task_app.c`：FreeRTOS 任务编排、任务创建、周期调度。

职责：
- 作为通信核心任务逻辑。
- 对上提供稳定 API，对下调用中间层和驱动抽象。
- 作为 FreeRTOS 任务组织层，统一收口任务生命周期与节拍。

### 应用层（Application Layer）
路径：`App/Src/layer_app`

文件：
- `rs485_console.c`：串口控制台输入输出、命令解析与请求下发。

职责：
- 面向用户交互/调试入口。
- 不直接操作底层寄存器，不绕过任务层状态机。

## 3. 头文件分层结果（已落地）

为保证“职责对应层次”与“兼容旧 include”同时成立，头文件采用双层结构：

1. 分层头文件（真实定义）
- `App/Inc/layer_driver`: `app_preflight.h`、`rs485_lock.h`
- `App/Inc/layer_middle`: `rs485_protocol.h`、`rs485_codec.h`、`rs485_scheduler.h`
- `App/Inc/layer_task`: `app_config.h`、`task_rs485.h`
- `App/Inc/layer_app`: `rs485_console.h`

2. 兼容转发头（稳定外部接口）
- `App/Inc` 下保留同名头文件，仅做 `#include "layer_xxx/xxx.h"` 转发。

这样既保证了分层物理归属，又避免一次性修改所有外部引用。

## 4. 分层依赖规则（必须遵守）

允许依赖方向：
- 应用层 -> 任务逻辑层 -> 中间层 -> 底层驱动层

禁止：
- 下层反向依赖上层。
- 应用层直接访问协议底层寄存器。
- 中间层直接包含控制台或应用命令模块。

## 5. FreeRTOS 迁移映射建议

1. `task_rs485.c` 负责通信状态机本体，保持纯逻辑服务函数。
2. `task_app.c` 负责创建 RTOS 任务、绑定周期节拍和任务优先级。
3. `rs485_console.c` 继续作为交互输入输出模块，由任务层周期驱动。
4. `rs485_lock.c` 接口替换为 FreeRTOS mutex6. `Core/Inc/FreeRTOSConfig.h` 作为内核配置入口，统一优先级与内存策略。
7. `lib/FreeRTOSKernel` 作为内核源码本地化依赖，方便版本控制和审计。5. `app_preflight.c` 继续作为系统初始化与中断策略入口。

这样迁移时只需替换并发原语与任务启动方式，不需要重写协议与调度核心。
