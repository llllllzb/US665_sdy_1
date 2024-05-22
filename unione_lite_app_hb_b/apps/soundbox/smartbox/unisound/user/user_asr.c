#include "uni_nlu.h"
#include "kws_demo.h"

int user_asr_word_enable(const char *word)
{
  return del_word_from_blacklist(word);
}

int user_asr_word_disable(const char *word)
{
  return add_word_into_blacklist(word);
}

int user_asr_timeout_disable(void)
{
  disable_asr_timeout();
  return 0;
}

int user_asr_timeout_enable(void)
{
  enable_asr_timeout();
  return 0;
}

