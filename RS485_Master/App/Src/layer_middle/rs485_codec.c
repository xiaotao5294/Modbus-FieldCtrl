/**
 * @file rs485_codec.c
 * @brief 主机侧 Modbus 编解码实现
 *
 * 此文件实现主机侧 Modbus 请求帧构建与响应帧解析，覆盖 0x03 读保持寄存器
 * 和 0x06 写单寄存器两类事务，统一执行协议字段与 CRC 合法性校验。
 *
 * 主要内容：
 * - 读写请求帧构建实现
 * - 读响应寄存器解析实现
 * - 写响应回显校验实现
 */
#include "layer_middle/rs485_codec.h"

#include "layer_middle/rs485_protocol.h"

// 统一校验帧长度和指针，减少各入口重复防御代码。
static uint8_t rs485_codec_validate_buffer(const uint8_t *buf, uint16_t len, uint16_t need_len)
{
    if ((buf == 0) || (len < need_len)) {
        return 0U;
    }

    return 1U;
}

// 按 Modbus-RTU 格式组装 0x03 读请求，并在末尾追加 CRC。
uint8_t RS485_CodecBuildReadRequest(uint8_t slave_addr,
                                    uint16_t start_reg,
                                    uint16_t reg_count,
                                    uint8_t *tx_buf,
                                    uint16_t tx_buf_len)
{
    if ((tx_buf == 0) || (tx_buf_len < MODBUS_REQ_READ_LEN)) {
        return 0U;
    }

    tx_buf[0] = slave_addr;
    tx_buf[1] = MODBUS_FUNC_READ_HOLDING_REGS;
    tx_buf[2] = (uint8_t)((start_reg >> 8U) & 0x00FFU);
    tx_buf[3] = (uint8_t)(start_reg & 0x00FFU);
    tx_buf[4] = (uint8_t)((reg_count >> 8U) & 0x00FFU);
    tx_buf[5] = (uint8_t)(reg_count & 0x00FFU);
    Modbus_AppendCRC(tx_buf, 6U);

    return 1U;
}

// 按 Modbus-RTU 格式组装 0x06 写单寄存器请求，并在末尾追加 CRC。
uint8_t RS485_CodecBuildWriteRequest(uint8_t slave_addr,
                                     uint16_t reg_addr,
                                     uint16_t reg_val,
                                     uint8_t *tx_buf,
                                     uint16_t tx_buf_len)
{
    if ((tx_buf == 0) || (tx_buf_len < MODBUS_REQ_WRITE_LEN)) {
        return 0U;
    }

    tx_buf[0] = slave_addr;
    tx_buf[1] = MODBUS_FUNC_WRITE_SINGLE_REG;
    tx_buf[2] = (uint8_t)((reg_addr >> 8U) & 0x00FFU);
    tx_buf[3] = (uint8_t)(reg_addr & 0x00FFU);
    tx_buf[4] = (uint8_t)((reg_val >> 8U) & 0x00FFU);
    tx_buf[5] = (uint8_t)(reg_val & 0x00FFU);
    Modbus_AppendCRC(tx_buf, 6U);

    return 1U;
}

// 对 0x03 响应做完整合法性检查，并把寄存器导出给调用方缓存。
uint8_t RS485_CodecParseReadResponse(uint8_t expected_slave_addr,
                                     uint16_t expected_reg_count,
                                     const uint8_t *rx_buf,
                                     uint16_t rx_len,
                                     uint16_t *out_regs,
                                     uint16_t out_reg_count)
{
    uint16_t reg_i;

    if ((out_regs == 0) || (expected_reg_count == 0U) || (out_reg_count < expected_reg_count)) {
        return 0U;
    }

    if (rs485_codec_validate_buffer(rx_buf, rx_len, (uint16_t)(5U + expected_reg_count * 2U)) == 0U) {
        return 0U;
    }

    if (Modbus_CheckCRC(rx_buf, rx_len) == 0) {
        return 0U;
    }

    if ((rx_buf[0] != expected_slave_addr) ||
        (rx_buf[1] != MODBUS_FUNC_READ_HOLDING_REGS) ||
        (rx_buf[2] != (uint8_t)(expected_reg_count * 2U))) {
        return 0U;
    }

    // 异常响应功能码最高位会置 1，收到后直接判为失败交给上层重试。
    if ((rx_buf[1] & 0x80U) != 0U) {
        return 0U;
    }

    for (reg_i = 0U; reg_i < expected_reg_count; reg_i++) {
        uint16_t base = (uint16_t)(3U + reg_i * 2U);
        out_regs[reg_i] = (uint16_t)((uint16_t)rx_buf[base] << 8U) | rx_buf[base + 1U];
    }

    return 1U;
}

// 对 0x06 响应做地址/功能码/回显值校验，确保从机确实执行了目标写操作。
uint8_t RS485_CodecParseWriteResponse(uint8_t expected_slave_addr,
                                      uint16_t expected_reg_addr,
                                      uint16_t expected_reg_val,
                                      const uint8_t *rx_buf,
                                      uint16_t rx_len)
{
    uint16_t rsp_addr;
    uint16_t rsp_val;

    if (rs485_codec_validate_buffer(rx_buf, rx_len, MODBUS_RSP_WRITE_LEN) == 0U) {
        return 0U;
    }

    if (Modbus_CheckCRC(rx_buf, rx_len) == 0) {
        return 0U;
    }

    if ((rx_buf[0] != expected_slave_addr) || (rx_buf[1] != MODBUS_FUNC_WRITE_SINGLE_REG)) {
        return 0U;
    }

    if ((rx_buf[1] & 0x80U) != 0U) {
        return 0U;
    }

    rsp_addr = (uint16_t)((uint16_t)rx_buf[2] << 8U) | rx_buf[3];
    rsp_val = (uint16_t)((uint16_t)rx_buf[4] << 8U) | rx_buf[5];

    if ((rsp_addr != expected_reg_addr) || (rsp_val != expected_reg_val)) {
        return 0U;
    }

    return 1U;
}