/**
 * @file rs485_protocol.h
 * @brief Modbus-RTU 基础协议定义与 CRC 接口声明
 *
 * 此文件定义 Modbus-RTU 基础协议常量、通用帧结构以及 CRC16 相关函数接口，
 * 作为上层编解码模块和任务状态机共享的协议基础头文件。
 *
 * 主要内容：
 * - Modbus 功能码与基础常量定义
 * - 通用 RS485 帧结构体定义
 * - CRC16 计算、追加、校验接口
 */
#ifndef APP_LAYER_MIDDLE_RS485_PROTOCOL_H
#define APP_LAYER_MIDDLE_RS485_PROTOCOL_H

#include <stdint.h>

// Modbus 功能码和通用帧长度属于协议层定义，放到单独目录后更容易和业务逻辑区分。
#define MODBUS_FUNC_READ_HOLDING_REGS   0x03U
#define MODBUS_FUNC_WRITE_SINGLE_REG    0x06U
#define MODBUS_MAX_DATA_LEN             252U

typedef struct {
    uint8_t address;
    uint8_t function;
    uint8_t data[MODBUS_MAX_DATA_LEN];
    uint16_t crc16;
} RS485_Frame_t;

uint16_t Modbus_CRC16(const uint8_t *data, uint16_t length);
void Modbus_AppendCRC(uint8_t *frame, uint16_t length_without_crc);
int Modbus_CheckCRC(const uint8_t *frame, uint16_t length_with_crc);

#endif
