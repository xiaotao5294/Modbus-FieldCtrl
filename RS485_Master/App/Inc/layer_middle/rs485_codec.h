/**
 * @file rs485_codec.h
 * @brief 主机侧 Modbus 编解码接口声明
 *
 * 此文件声明主机侧 Modbus 读写事务编解码接口，负责请求帧构建和响应帧校验，
 * 以便任务状态机只关注事务调度和收发推进，不直接耦合协议细节。
 *
 * 主要内容：
 * - 0x03 读请求构建接口
 * - 0x06 写请求构建接口
 * - 0x03/0x06 响应解析与校验接口
 */
#ifndef APP_LAYER_MIDDLE_RS485_CODEC_H
#define APP_LAYER_MIDDLE_RS485_CODEC_H

#include <stdint.h>

// Modbus 固定长度帧定义放在编解码模块，主状态机只关心调用结果。
#define MODBUS_REQ_READ_LEN             8U
#define MODBUS_REQ_WRITE_LEN            8U
#define MODBUS_RSP_WRITE_LEN            8U

// 构建 0x03 读保持寄存器请求帧。
uint8_t RS485_CodecBuildReadRequest(uint8_t slave_addr,
                                    uint16_t start_reg,
                                    uint16_t reg_count,
                                    uint8_t *tx_buf,
                                    uint16_t tx_buf_len);

// 构建 0x06 写单寄存器请求帧。
uint8_t RS485_CodecBuildWriteRequest(uint8_t slave_addr,
                                     uint16_t reg_addr,
                                     uint16_t reg_val,
                                     uint8_t *tx_buf,
                                     uint16_t tx_buf_len);

// 解析 0x03 响应并导出寄存器数组。
uint8_t RS485_CodecParseReadResponse(uint8_t expected_slave_addr,
                                     uint16_t expected_reg_count,
                                     const uint8_t *rx_buf,
                                     uint16_t rx_len,
                                     uint16_t *out_regs,
                                     uint16_t out_reg_count);

// 校验 0x06 响应（地址、功能码、寄存器地址和值回显）。
uint8_t RS485_CodecParseWriteResponse(uint8_t expected_slave_addr,
                                      uint16_t expected_reg_addr,
                                      uint16_t expected_reg_val,
                                      const uint8_t *rx_buf,
                                      uint16_t rx_len);

#endif
