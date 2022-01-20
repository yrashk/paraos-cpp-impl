# By default, don't build a release
RELEASE ?= false

CXX = clang++

uname_s := $(shell uname -s)
ifeq ($(uname_s),Darwin)
  CXX_LD ?= ld.lld
else
  CXX_LD ?= ld
endif

QEMU_PARAMS = -cpu max -serial stdio -smp 2

CXX_FLAGS += -ffreestanding -nostdlib -nostdinc -fno-exceptions -fno-rtti \
	     -fpic -fno-stack-protector -mno-red-zone --std=c++20 \
	     -I libpara -Wall -Werror

ifeq ($(RELEASE),false)
  CXX_FLAGS += -g
endif

ifeq ($(RELEASE),true)
  CXX_FLAGS += -DRELEASE
endif

build = build

kernel_sources = kernel/bootboot.cpp kernel/cxx.cpp
kernel_objects = $(patsubst %.cpp,$(build)/%.o,$(kernel_sources))

libkernel_sources = kernel/devices/serial.cpp \
	kernel/pmm.cpp \
	kernel/platform.cpp \
	kernel/platform/x86_64/gdt.cpp kernel/platform/x86_64/idt.cpp \
	kernel/platform/x86_64/port.cpp \
	kernel/platform/x86_64/serial.cpp \
	kernel/platform/x86_64/panic.cpp \
        kernel/platform/x86_64/init.cpp \
	kernel/platform/x86_64.cpp kernel/testing.cpp kernel/main.cpp
libkernel_pcms = $(patsubst %.pcm,$(build)/%.pcm,$(subst /,.,$(patsubst %.cpp,%.pcm,$(libkernel_sources))))
libkernel_objects = $(patsubst %.o,$(build)/%.o,$(subst /,.,$(patsubst %.cpp,%.o,$(libkernel_sources))))

libpara_sources = libpara/basic_types.cpp libpara/concepts.cpp libpara/formatting.cpp libpara/testing.cpp libpara/loop.cpp \
		  libpara/xxh64.cpp libpara/err.cpp libpara/sync.cpp
libpara_headers = $(wildcard libpara/*.hpp)
libpara_pcms = $(patsubst %.pcm,$(build)/%.pcm,$(subst /,.,$(patsubst %.cpp,%.pcm,$(libpara_sources))))
libpara_objects = $(patsubst %.o,$(build)/%.o,$(subst /,.,$(patsubst %.cpp,%.o,$(libpara_sources))))

.SECONDARY: $(libkernel_pcms) $(libpara_pcms)

all: $(build)/boot.img

check:
	@echo $(libkernel_pcms)

$(build)/paraos: $(kernel_objects) $(kernel_sources) $(libkernel_objects) $(libpara_objects) kernel/bootboot.ld Makefile
	$(CXX_LD) $(kernel_objects) $(libkernel_objects) $(libpara_objects) \
	-T kernel/bootboot.ld -o $@ -e bootboot_main -nostdlib

$(build)/%.o: %.cpp Makefile $(libpara_pcms) $(libkernel_pcms) $(libpara_headers)
	@mkdir -p $(dir $@)
	$(CXX) -target x86_64-unknown -c $< -o $@ $(CXX_FLAGS) \
	-fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=$(build)

$(build)/boot.img: $(build)/paraos tools/mkbootimg/mkbootimg boot.config bootimage.json
	tools/mkbootimg/mkbootimg check $<
	tools/mkbootimg/mkbootimg bootimage.json $@

$(build)/boot_test.img: $(build)/paraos tools/mkbootimg/mkbootimg boot_test.config bootimage_test.json
	tools/mkbootimg/mkbootimg check $<
	tools/mkbootimg/mkbootimg bootimage_test.json $@

tools/mkbootimg/mkbootimg: $(filter-out tools/mkbootimg/mkbootimg,$(wildcard tools/mkbootimg/*.*))
	$(MAKE) -C tools/mkbootimg
	rm -f tools/mkbootimg*.zip	

qemu: $(build)/boot.img
	qemu-system-x86_64 -drive format=raw,file=$< $(QEMU_PARAMS)
 
qemu-gdb: $(build)/boot.img
	qemu-system-x86_64 --drive format=raw,file=$< $(QEMU_PARAMS) -S -s

test: $(build)/boot_test.img
	qemu-system-x86_64 -drive format=raw,file=$< $(QEMU_PARAMS) -no-reboot -device isa-debug-exit ;\
	EXIT_CODE=$$?  ;\
	exit $$(($$EXIT_CODE >> 1)) 

test-gdb: $(build)/boot_test.img
	qemu-system-x86_64 -drive format=raw,file=$< $(QEMU_PARAMS) -no-reboot -device isa-debug-exit -S -s ;\
	EXIT_CODE=$$?  ;\
	exit $$(($$EXIT_CODE >> 1)) 

clean:
	rm -rf $(build)

deepclean: clean
	$(MAKE) -C tools/mkbootimg clean

.SECONDEXPANSION:

$(build)/kernel.%.pcm: kernel/$$(subst .,/,%).cpp Makefile $(libpara_pcms) $(libpara_sources) $(libpara_headers)
	@mkdir -p $(dir $@)
	$(CXX) -target x86_64-unknown -c $< -o $@ $(CXX_FLAGS) -Xclang -emit-module-interface \
	-fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=$(build)

$(build)/kernel.%.o: $(build)/kernel.%.pcm Makefile $(libpara_pcms) $(libpara_sources) $(libpara_headers)
	@mkdir -p $(dir $@)
	$(CXX) -target x86_64-unknown -c $< -o $@

$(build)/libpara.%.pcm: libpara/$$(subst .,/,%).cpp Makefile $(libpara_headers)
	@mkdir -p $(dir $@)
	$(CXX) -target x86_64-unknown -c $< -o $@ $(CXX_FLAGS) -Xclang -emit-module-interface \
	-fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=$(build)

$(build)/libpara.%.o: $(build)/libpara.%.pcm Makefile $(libpara_headers)
	@mkdir -p $(dir $@)
	$(CXX) -target x86_64-unknown -c $< -o $@

.ONESHELL:
