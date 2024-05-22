##!/bin/sh

OBJDUMP=/opt/toolchains/hbb/pi32/bin/objdump
OBJCOPY=/opt/toolchains/hbb/pi32/bin/objcopy
OBJSIZEDUMP=/opt/toolchains/hbb/pi32/bin/objsizedump
REMOVE_TAIL=./remove_tailing_zeros

cd $(dirname $0)

${OBJDUMP} -D -address-mask=0x1ffffff -print-dbg $1.elf > $1.lst
${OBJCOPY} -O binary -j .text $1.elf text.bin
${OBJCOPY} -O binary -j .data $1.elf data.bin
${OBJCOPY} -O binary -j .data_code $1.elf data_code.bin

${OBJCOPY} -O binary -j .overlay_aec $1.elf aeco.bin
${OBJCOPY} -O binary -j .overlay_wav $1.elf wav.bin
${OBJCOPY} -O binary -j .overlay_ape $1.elf ape.bin
${OBJCOPY} -O binary -j .overlay_flac $1.elf flac.bin
${OBJCOPY} -O binary -j .overlay_m4a $1.elf m4a.bin
${OBJCOPY} -O binary -j .overlay_amr $1.elf amr.bin
${OBJCOPY} -O binary -j .overlay_dts $1.elf dts.bin
${OBJCOPY} -O binary -j .overlay_fm $1.elf fmo.bin
${OBJCOPY} -O binary -j .overlay_mp3 $1.elf mp3o.bin
${OBJCOPY} -O binary -j .overlay_wma $1.elf wmao.bin



${REMOVE_TAIL} -i aeco.bin -o aec.bin -mark ff
${REMOVE_TAIL} -i fmo.bin -o fm.bin -mark ff
${REMOVE_TAIL} -i mp3o.bin -o mp3.bin -mark ff
${REMOVE_TAIL} -i wmao.bin -o wma.bin -mark ff


${OBJDUMP} -section-headers -address-mask=0x1ffffff $1.elf
${OBJSIZEDUMP} -lite -skip-zero -enable-dbg-info $1.elf | sort -k 1 > symbol_tbl.txt


cat text.bin data.bin data_code.bin aec.bin wav.bin ape.bin flac.bin m4a.bin amr.bin dts.bin fm.bin mp3.bin wma.bin > app.bin
export LD_LIBRARY_PATH=./lib
./isd_download -tonorflash -dev br23 -boot 0x12000 -div8 -wait 300 -uboot uboot.boot -app app.bin cfg_tool.bin -res tone.cfg -mkey unisound.mkey -key1 unisound.key1 -format all
echo $?
