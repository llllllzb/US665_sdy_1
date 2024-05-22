#ifndef USER_TIMER_H_
#define USER_TIMER_H_

typedef void (*user_timer_cb)(void *);

/** @addtogroup uni_timer_interface
@{*/
/**
 * @brief 创建定时动作
 * @param msec 定时长度/周期
 * @param single_shot 0：连续模式；1：单次模式，超时之后定时器停止工作
 * @param cb 回调函数，当每次超时发生时被执行
 * @param priv 私有数据,供回调函数使用
 * @retval 所创建的定时器id
 */
uint16_t user_timer_create(uint32_t msec, bool single_shot, user_timer_cb cb, void *priv);
/**
 * @brief 删除定时动作
 * @param id 定时器id，user_timer_create()的返回值
 * @param single_shot 定时器的工作模式
 * @retval 0 操作成功
 */
int user_timer_delete(uint16_t id, bool single_shot);
/** @}*/
#endif
