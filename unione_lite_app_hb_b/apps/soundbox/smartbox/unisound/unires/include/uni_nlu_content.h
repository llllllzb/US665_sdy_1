/**************************************************************************
 * Copyright (C) 2012-2021  Unisound
 *
 * DO NOT MODIFY.
 *
 * This file was generated by res_build_tool.py
 *
 **************************************************************************/
#ifndef INC_UNI_NLU_CONTENT_H_
#define INC_UNI_NLU_CONTENT_H_

#include "uni_nlu_content_type.h"

const char* g_nlu_content_str[] = {
  [eCMD_wakeup_uni] = "{\"cmd\":\"wakeup_uni\",\"pcm\":\"[103, 104, 105]\"}",
  [eCMD_exitUni] = "{\"cmd\":\"exitUni\",\"pcm\":\"[106]\"}",
  [eCMD_TempSet15] = "{\"cmd\":\"TempSet15\",\"pcm\":\"[107]\"}",
};

/*TODO perf sort by hashcode O(logN), now version O(N)*/
const uni_nlu_content_mapping_t g_nlu_content_mapping[] = {
  {2947460911U/*你好魔方*/, eCMD_wakeup_uni, NULL},
  {2497873774U/*退下*/, eCMD_exitUni, NULL},
  {2389288886U/*再见*/, eCMD_exitUni, NULL},
  {4038776460U/*设置十五度*/, eCMD_TempSet15, NULL},
  {265656716U/*十五度*/, eCMD_TempSet15, NULL},
};

#endif
