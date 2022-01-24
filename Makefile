depdir := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(depdir)/$@.d

# By default, don't build a release
RELEASE ?= false

CXX = clang++

uname_s := $(shell uname -s)
ifeq ($(uname_s),Darwin)
  CXX_LD ?= ld.lld
else
  CXX_LD ?= ld
endif

QEMU_SMP ?= 2
QEMU_PARAMS = -cpu max -serial mon:stdio -machine q35 -smp $(QEMU_SMP)

OVMF = -drive if=pflash,format=raw,readonly=on,file=support/OVMF.fd \
       -drive if=pflash,format=raw,readonly=off,file=support/OVMF_VARS.fd

CXX_FLAGS += -ffreestanding -nostdlib -nostdinc -fno-exceptions -fno-rtti \
	     -fpic -fstack-protector-all -mno-red-zone --std=c++20 \
	     -I libpara -Wall -Werror $(DEPFLAGS)

ifeq ($(RELEASE),false)
  CXX_FLAGS += -g -fstack-size-section
  PCM_CXX_FLAGS += -g -fstack-size-section -mcmodel=kernel
endif

ifeq ($(RELEASE),true)
  CXX_FLAGS += -DRELEASE
endif

build = build

kernel_sources = kernel/devices/serial.cpp \
	kernel/pmm.cpp \
	kernel/platform.cpp \
	kernel/platform/x86_64/gdt.cpp kernel/platform/x86_64/idt.cpp \
	kernel/platform/x86_64/port.cpp \
	kernel/platform/x86_64/serial.cpp \
	kernel/platform/x86_64/panic.cpp \
        kernel/platform/x86_64/init.cpp \
	kernel/platform/x86_64.cpp kernel/testing.cpp kernel/main.cpp \
	kernel/cxx.cpp kernel/ssp.cpp kernel/bootboot.cpp

kernel_pcms = $(patsubst %.pcm,$(build)/%.pcm,$(subst /,.,$(patsubst %.cpp,%.pcm,$(kernel_sources))))
kernel_objects = $(patsubst %.o,$(build)/%.o,$(subst /,.,$(patsubst %.cpp,%.o,$(kernel_sources))))

libpara_sources = libpara/basic_types.cpp libpara/concepts.cpp libpara/formatting.cpp libpara/testing.cpp libpara/loop.cpp \
		  libpara/xxh64.cpp libpara/err.cpp libpara/sync.cpp
libpara_headers = $(wildcard libpara/*.hpp)
libpara_pcms = $(patsubst %.pcm,$(build)/%.pcm,$(subst /,.,$(patsubst %.cpp,%.pcm,$(libpara_sources))))
libpara_objects = $(patsubst %.o,$(build)/%.o,$(subst /,.,$(patsubst %.cpp,%.o,$(libpara_sources))))

depfiles := $(libpara_pcms:%.pcm=$(depdir)/%.pcm.d) $(kernel_pcms:%.pcm=$(depdir)/%.pcm.d)
mdepfiles := $(libpara_pcms:%.pcm=$(depdir)/%.pcm.md) $(kernel_pcms:%.pcm=$(depdir)/%.pcm.md)

.SECONDARY: $(kernel_pcms) $(libpara_pcms)

all: $(build)/paraos

$(build)/paraos: $(libpara_objects) $(kernel_objects) kernel/bootboot.ld Makefile
	$(CXX_LD) $(kernel_objects) $(libpara_objects) \
	-T kernel/bootboot.ld -o $@ -e bootboot_main -nostdlib

$(build)/bootdisk/bootboot/x86_64: $(build)/paraos
	mkdir -p $(build)/bootdisk/bootboot
	cp support/bootboot.efi $(build)/bootdisk/bootboot.efi
	echo "BOOTBOOT.EFI" > $(build)/bootdisk/startup.nsh
	cp $(build)/paraos $(build)/bootdisk/bootboot/x86_64

qemu: $(build)/bootdisk/bootboot/x86_64
	qemu-system-x86_64 $(OVMF) \
	-drive format=raw,file=fat:rw:$(build)/bootdisk $(QEMU_PARAMS) -s

qemu-dbg: $(build)/bootdisk/bootboot/x86_64
	qemu-system-x86_64 $(OVMF) \
	-drive format=raw,file=fat:rw:$(build)/bootdisk $(QEMU_PARAMS) -s -S

$(build)/bootdisk_test/bootboot/x86_64: $(build)/paraos
	mkdir -p $(build)/bootdisk_test/bootboot
	cp support/bootboot.efi $(build)/bootdisk_test/bootboot.efi
	echo "BOOTBOOT.EFI" > $(build)/bootdisk_test/startup.nsh
	echo "test=yes" >> $(build)/bootdisk_test/bootboot/config
	cp $(build)/paraos $(build)/bootdisk_test/bootboot/x86_64
 
test: $(build)/bootdisk_test/bootboot/x86_64
	qemu-system-x86_64 -nographic $(OVMF) \
	-drive format=raw,file=fat:rw:$(build)/bootdisk_test $(QEMU_PARAMS) -no-reboot -s -device isa-debug-exit ;\
	EXIT_CODE=$$?  ;\
	exit $$(($$EXIT_CODE >> 1)) 

test-gdb: $(build)/bootdisk_test/bootboot/x86_64
	qemu-system-x86_64 -nographic $(OVMF) \
	-drive format=raw,file=fat:rw:$(build)/bootdisk_test $(QEMU_PARAMS) -no-reboot -s -S -device isa-debug-exit ;\
	EXIT_CODE=$$?  ;\
	exit $$(($$EXIT_CODE >> 1)) 

clean:
	rm -rf $(build) $(depdir)

.SECONDEXPANSION:

$(build)/kernel.%.pcm: kernel/$$(subst .,/,%).cpp $(depdir)/$(build)/kernel.%.pcm.d $(depdir)/$(build)/kernel.%.pcm.md Makefile | $(depdir)
	@mkdir -p $(dir $@)
	$(CXX) -target x86_64-unknown -c $< -o $@ $(CXX_FLAGS) -Xclang -emit-module-interface -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=$(build)

$(build)/kernel.%.o: $(build)/kernel.%.pcm Makefile 
	@mkdir -p $(dir $@)
	$(CXX) -target x86_64-unknown -c $< -o $@ $(PCM_CXX_FLAGS)

$(build)/libpara.%.pcm: libpara/$$(subst .,/,%).cpp $(depdir)/$(build)/libpara.%.pcm.d $(depdir)/$(build)/libpara.%.pcm.md Makefile | $(depdir)
	@mkdir -p $(dir $@)
	$(CXX) -target x86_64-unknown -c $< -o $@ $(CXX_FLAGS) -Xclang -emit-module-interface -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=$(build)

$(build)/libpara.%.o: $(build)/libpara.%.pcm Makefile
	@mkdir -p $(dir $@)
	$(CXX) -target x86_64-unknown -c $< -o $@ $(PCM_CXX_FLAGS)

$(depdir): ; @mkdir -p $@/build

$(depfiles):

$(mdepfiles):

$(depdir)/$(build)/kernel.%.pcm.md: kernel/$$(subst .,/,%).cpp Makefile | $(depdir)
	@gawk '{ if (match($$0, /import\s+([a-zA-Z0-9\._]+);/, arr)) print "$(patsubst %.pcm,$(build)/%.pcm,$(subst /,.,$(patsubst %.cpp,%.pcm,$<)))" ": $(build)/" arr[1] ".pcm"; }' $< > $@

$(depdir)/$(build)/libpara.%.pcm.md: libpara/$$(subst .,/,%).cpp Makefile | $(depdir)
	@gawk '{ if (match($$0, /import\s+([a-zA-Z0-9\._]+);/, arr)) print "$(patsubst %.pcm,$(build)/%.pcm,$(subst /,.,$(patsubst %.cpp,%.pcm,$<)))" ": $(build)/" arr[1] ".pcm"; }' $< > $@
 

include $(wildcard $(depfiles))
include $(wildcard $(mdepfiles))

.ONESHELL:
