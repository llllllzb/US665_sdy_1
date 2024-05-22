#ifndef BT_PREPROC_H_
#define BT_PREPROC_H_

/*
 * pack to send
 * input:cmd(key#value)
 * return:bt_data(json), NULL if pack failed
 */
char* uni_bt_preproc_pack(const char *cmd);

/*
 * unpack from receive
 * input:bt_data(json)
 * return:cmd(key#value), NULL if unpack failed
 */
char* uni_bt_preproc_unpack(const char *bt_data);

#endif
