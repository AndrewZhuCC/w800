CPRE := @
ifeq ($(V),1)
CPRE :=
VERB := --verbose
endif

MK_GENERATED_IMGS_PATH:=generated
SOLUTOIN_NAME:=$(shell basename `pwd`)


.PHONY:startup
startup: all

all:
	@echo "Build Solution by $(BOARD) "
	$(CPRE) scons $(VERB) --board=$(BOARD) -j4
	@echo YoC SDK Done

	@echo [INFO] Create bin files
	$(CPRE) product image $(MK_GENERATED_IMGS_PATH)/images.zip -i $(MK_GENERATED_IMGS_PATH)/data -l -v "1.0.0" -p
	$(CPRE) product image $(MK_GENERATED_IMGS_PATH)/images.zip -e $(MK_GENERATED_IMGS_PATH) -x
	$(CPRE) product image $(MK_GENERATED_IMGS_PATH)/images.zip -e $(MK_GENERATED_IMGS_PATH)/data

	@echo [INFO] Create uni bin files
	$(CPRE) csky-elfabiv2-objcopy -O binary ./out/$(SOLUTOIN_NAME)/yoc.elf ./out/$(SOLUTOIN_NAME)/yoc.bin
	$(CPRE) gcc ../../components/chip_us615/unisdk/tools/us615/uni_tool.c -Wall -lpthread -O2 -o ./out/$(SOLUTOIN_NAME)/uni_tool
	$(CPRE) ./out/$(SOLUTOIN_NAME)/uni_tool -b ./out/$(SOLUTOIN_NAME)/yoc.bin -fc 0 -it 1 -ih 20047400 -ra 8012000 -ua 81C9000 -nh 0 -un 0 -vs yoc-7.3.0
	$(CPRE) ./out/$(SOLUTOIN_NAME)/uni_tool -b ./out/$(SOLUTOIN_NAME)/yoc.img -fc 1 -it 1 -ih 20047400 -ra 8012000 -ua 81C9000 -nh 0 -un 0 -vs yoc-7.3.0
	$(CPRE) mv ./out/$(SOLUTOIN_NAME)/yoc_gz.img ./out/$(SOLUTOIN_NAME)/yoc_ota.bin
	@echo [INFO] Create fls file
	$(CPRE) cp $(MK_GENERATED_IMGS_PATH)/data/boot boot.bin
	$(CPRE) cp $(MK_GENERATED_IMGS_PATH)/data/prim prim.bin
	$(CPRE) cp $(MK_GENERATED_IMGS_PATH)/data/imtb imtb.bin
	$(CPRE) ./out/$(SOLUTOIN_NAME)/uni_tool -b boot.bin -fc 0 -it 512 -ih 8002000 -ra 8002400 -ua 81C9000 -nh 20047400 -un 0
	$(CPRE) ./out/$(SOLUTOIN_NAME)/uni_tool -b prim.bin -fc 0 -it 1 -ih 20047400 -ra 8012000 -ua 81C9000 -nh 0 -un 0
	$(CPRE) ./out/$(SOLUTOIN_NAME)/uni_tool -b imtb.bin -fc 0 -it 1 -ih 20047400 -ra 8010000 -ua 0 -nh 0 -un 0
	$(CPRE) cat boot.img prim.img imtb.img > ./out/$(SOLUTOIN_NAME)/uni_app.fls
	$(CPRE) rm -f boot.bin prim.bin imtb.bin boot.img prim.img imtb.img

.PHONY:fls
fls:
	@echo [INFO] Create fls file
	$(CPRE) cp $(MK_GENERATED_IMGS_PATH)/data/boot boot.bin
	$(CPRE) cp $(MK_GENERATED_IMGS_PATH)/data/prim prim.bin
	$(CPRE) cp $(MK_GENERATED_IMGS_PATH)/data/imtb imtb.bin
	$(CPRE) ./out/$(SOLUTOIN_NAME)/uni_tool -b boot.bin -fc 0 -it 512 -ih 8002000 -ra 8002400 -ua 81C9000 -nh 20047400 -un 0
	$(CPRE) ./out/$(SOLUTOIN_NAME)/uni_tool -b prim.bin -fc 0 -it 1 -ih 20047400 -ra 8012000 -ua 81C9000 -nh 0 -un 0
	$(CPRE) ./out/$(SOLUTOIN_NAME)/uni_tool -b imtb.bin -fc 0 -it 1 -ih 20047400 -ra 8010000 -ua 0 -nh 0 -un 0
	$(CPRE) cat boot.img prim.img imtb.img > ./out/$(SOLUTOIN_NAME)/uni_app.fls
	$(CPRE) rm -f boot.bin prim.bin imtb.bin boot.img prim.img imtb.img

sdk:
	$(CPRE) yoc sdk

.PHONY:clean
clean:
	$(CPRE) scons -c
	$(CPRE) find . -name "*.[od]" -delete
	$(CPRE) rm yoc_sdk yoc.* generated out -rf
