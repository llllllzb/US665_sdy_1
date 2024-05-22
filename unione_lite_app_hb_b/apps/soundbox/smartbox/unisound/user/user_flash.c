#include "cpu.h"
#include "syscfg_id.h"

int user_flash_read(u16 id, void *buff, u16 len)
{
  return syscfg_read(id, buff, len);
}

int user_flash_write(u16 id, void *buff, u16 len)
{
  return syscfg_write(id, buff, len);
}
