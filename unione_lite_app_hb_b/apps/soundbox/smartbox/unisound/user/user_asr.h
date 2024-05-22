#ifndef USER_ASR_H_
#define USER_ASR_H_

/** @addtogroup uni_asr_interface
@{*/

/**
 * @brief 启用识别词
 * @param word 要启用的识别词
 * @retval 0 操作成功
 * @retval -1 操作失败
 */
int user_asr_word_enable(const char *word);

/**
 * @brief 禁用识别词
 * @param word 要禁用的识别词
 * @retval 0 操作成功
 * @retval -1 操作失败
 */
int user_asr_word_disable(const char *word);

/**
 * @brief 禁止超时自动退出识别模式
 * @retval 0 操作成功
 */
int user_asr_timeout_disable(void);

/**
 * @brief 使能超时自动退出识别模式
 * @retval 0 操作成功
 */
int user_asr_timeout_enable(void);
/** @}*/
#endif
