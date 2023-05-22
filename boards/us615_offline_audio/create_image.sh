#!/usr/bin/env bash

if [ "$1" = "make" ];then
echo "create us615_offline_audio img"
     product image ./$2/images.zip -i ./$2/data -l -p
     product image ./$2/images.zip -e ./$2 -x

echo "Create unii bin files"
    csky-elfabiv2-objcopy -O binary ./out/$3/yoc.elf ./out/$3/yoc.bin
    gcc ../../components/chip_us615/unisdk/tools/us615/uni_tool.c -Wall -lpthread -O2 -o ./out/$3/uni_tool
    ./out/$3/uni_tool -b ./out/$3/yoc.bin -fc 0 -it 1 -ih 80D0000 -ra 80D0400 -ua 8010000 -nh 0 -un 0
    cat ../../components/chip_us615/unisdk/tools/us615/us615_secboot.img ./out/$3/yoc.img > ./out/$3/yoc.fls
else
echo "flash us615_offline_audio"
    product flash ./$2/images.zip -w prim -f us615_flash.elf
fi
