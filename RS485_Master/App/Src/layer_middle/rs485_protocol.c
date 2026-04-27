/**
 * @file rs485_protocol.c
 * @brief Modbus-RTU 基础协议实现（CRC）
 *
 * 此文件实现 Modbus-RTU CRC16 的基础能力，包括 CRC 计算、帧尾追加 CRC，
 * 以及完整帧 CRC 校验逻辑，供主机侧编解码和事务处理复用。
 *
 * 主要内容：
 * - CRC16 标准算法实现
 * - 帧尾 CRC 追加实现
 * - 完整帧 CRC 校验实现
 */
#include "layer_middle/rs485_protocol.h"

// Modbus-RTU 的 CRC16 算法是协议层基础能力，因此单独放在协议模块里，避免和业务状态机耦合。
uint16_t Modbus_CRC16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;
    uint8_t j;

    if (data == 0) {
        return 0U;
    }

    for (i = 0U; i < length; i++) {
        crc ^= (uint16_t)data[i];
        for (j = 0U; j < 8U; j++) {
            if ((crc & 0x0001U) != 0U) {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            } else {
                crc >>= 1U;
            }
        }
    }

    return crc;
}

// 这里直接在帧尾追加 CRC，调用方只需要保证缓冲区末尾已经预留 2 字节空间。
void Modbus_AppendCRC(uint8_t *frame, uint16_t length_without_crc)
{
    uint16_t crc;

    if (frame == 0) {
        return;
    }

    crc = Modbus_CRC16(frame, length_without_crc);
    frame[length_without_crc] = (uint8_t)(crc & 0x00FFU);
    frame[length_without_crc + 1U] = (uint8_t)((crc >> 8U) & 0x00FFU);
}

// CRC 校验也放在协议模块里，调用方只关心帧是否合法，不需要重复理解校验细节。
int Modbus_CheckCRC(const uint8_t *frame, uint16_t length_with_crc)
{
    uint16_t expected_crc;
    uint16_t recv_crc;

    if ((frame == 0) || (length_with_crc < 4U)) {
        return 0;
    }

    expected_crc = Modbus_CRC16(frame, (uint16_t)(length_with_crc - 2U));
    recv_crc = (uint16_t)frame[length_with_crc - 2U] |
               (uint16_t)((uint16_t)frame[length_with_crc - 1U] << 8U);

    if (recv_crc != expected_crc) {
        return 0;
    }

    return 1;
}