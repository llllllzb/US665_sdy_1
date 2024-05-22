#ifndef USER_FLASH_H_
#define USER_FLASH_H_

#include "cpu.h"

/** @addtogroup uni_flash_interface
@{*/

/**
 * @brief 读取Flash数据
 * @param id 数据的id，有效范围为[1, 49]
 * @param buff 数据缓冲区起始地址
 * @param len 读取的数据长度
 * @retval 实际读取的数据长度
 */
int user_flash_read(u16 id, void *buff, u16 len);

/**
 * @brief 向Flash写入数据
 * @param id 数据的id，有效范围为[1, 49]
 * @param buff 数据起始地址
 * @param len 数据的长度
 * @retval 实际写入的数据长度
 */
int user_flash_write(u16 id, void *buff, u16 len);
/** @}*/
#endif
