## Configuration defaults

# By default, don't build a release
RELEASE ?= false
# By default, use Clang C++ compiler
CXX = clang++
# By default, run ParaOS in QEMU with 2 CPUS
QEMU_SMP ?= 2
# Additional options for QEMU
QEMU_OPTS ?=
# QEMU
QEMU ?= qemu-system-x86_64
# Extra C++ compile flags
CXX_FLAGS +=

##

depdir := .deps
depflags = -MT $@ -MMD -MP -MF $(depdir)/$@.d

uname_s := $(shell uname -s)
ifeq ($(uname_s),Darwin)
  cxx_ld ?= ld.lld
else
  cxx_ld ?= ld
endif

qemu_params = -cpu max -serial mon:stdio -machine q35 -smp $(QEMU_SMP) $(QEMU_OPTS)

ovmf = -drive if=pflash,format=raw,readonly=on,file=support/ovmf.fd \
       -drive if=pflash,format=raw,readonly=off,file=support/ovmf_VARS.fd

cxx_flags += $(CXX_FLAGS) -ffreestanding -nostdlib -nostdinc -fno-exceptions -fno-rtti \
	     -fpic -fstack-protector-all -mno-red-zone --std=c++20 \
	     -I libpara -Wall -Werror $(depflags)

pcm_cxx_flags +=  -mcmodel=kernel -Wno-unused-command-line-argument

ifeq ($(RELEASE),false)
  cxx_flags += -g -fstack-size-section
  pcm_cxx_flags += -g -fstack-size-section
endif

ifeq ($(RELEASE),true)
  cxx_flags += -DRELEASE
endif

build = build

kernel_sources = $(wildcard kernel/*.cpp kernel/*/*.cpp kernel/*/*/*.cpp kernel/*/*/*/*.cpp)
kernel_pcms = $(patsubst %.pcm,$(build)/%.pcm,$(subst /,.,$(patsubst %.cpp,%.pcm,$(kernel_sources))))
kernel_objects = $(patsubst %.o,$(build)/%.o,$(subst /,.,$(patsubst %.cpp,%.o,$(kernel_sources))))

libpara_sources = $(wildcard libpara/*.cpp libpara/*/*.cpp libpara/*/*/*.cpp libpara/*/*/*/*.cpp)
libpara_headers = $(wildcard libpara/*.hpp)
libpara_pcms = $(patsubst %.pcm,$(build)/%.pcm,$(subst /,.,$(patsubst %.cpp,%.pcm,$(libpara_sources))))
libpara_objects = $(patsubst %.o,$(build)/%.o,$(subst /,.,$(patsubst %.cpp,%.o,$(libpara_sources))))

depfiles := $(libpara_pcms:%.pcm=$(depdir)/%.pcm.d) $(kernel_pcms:%.pcm=$(depdir)/%.pcm.d)
mdepfiles := $(libpara_pcms:%.pcm=$(depdir)/%.pcm.md) $(kernel_pcms:%.pcm=$(depdir)/%.pcm.md)

.SECONDARY: $(kernel_pcms) $(libpara_pcms)

all: $(build)/paraos

$(build)/paraos: $(libpara_objects) $(kernel_objects) kernel/bootboot.ld Makefile
	$(cxx_ld) $(kernel_objects) $(libpara_objects) \
	-T kernel/bootboot.ld -o $@ -e bootboot_main -nostdlib

$(build)/bootdisk/bootboot/x86_64: $(build)/paraos
	mkdir -p $(build)/bootdisk/bootboot
	cp support/bootboot.efi $(build)/bootdisk/bootboot.efi
	echo "BOOTBOOT.EFI" > $(build)/bootdisk/startup.nsh
	cp $(build)/paraos $(build)/bootdisk/bootboot/x86_64

qemu: $(build)/bootdisk/bootboot/x86_64
	$(QEMU) $(ovmf) \
	-drive format=raw,file=fat:rw:$(build)/bootdisk $(qemu_params) -s

qemu-dbg: $(build)/bootdisk/bootboot/x86_64
	$(QEMU) $(ovmf) \
	-drive format=raw,file=fat:rw:$(build)/bootdisk $(qemu_params) -s -S

$(build)/bootdisk_test/bootboot/x86_64: $(build)/paraos
	mkdir -p $(build)/bootdisk_test/bootboot
	cp support/bootboot.efi $(build)/bootdisk_test/bootboot.efi
	echo "BOOTBOOT.EFI" > $(build)/bootdisk_test/startup.nsh
	echo "test=yes" >> $(build)/bootdisk_test/bootboot/config
	cp $(build)/paraos $(build)/bootdisk_test/bootboot/x86_64
 
test: $(build)/bootdisk_test/bootboot/x86_64
	qemu-system-x86_64 -nographic $(ovmf) \
	-drive format=raw,file=fat:rw:$(build)/bootdisk_test $(qemu_params) -no-reboot -s -device isa-debug-exit ;\
	EXIT_CODE=$$?  ;\
	exit $$(($$EXIT_CODE >> 1)) 

test-gdb: $(build)/bootdisk_test/bootboot/x86_64
	qemu-system-x86_64 -nographic $(ovmf) \
	-drive format=raw,file=fat:rw:$(build)/bootdisk_test $(qemu_params) -no-reboot -s -S -device isa-debug-exit ;\
	EXIT_CODE=$$?  ;\
	exit $$(($$EXIT_CODE >> 1)) 

clean:
	rm -rf $(build) $(depdir)

$(depdir): ; @mkdir -p $@/build
$(build): ; @mkdir -p $@

$(depfiles):
$(mdepfiles):

-include $(depfiles)
-include $(mdepfiles)

.SECONDEXPANSION:

$(build)/%.pcm: $$(subst .,/,%).cpp $(depdir)/$(build)/%.pcm.d $(depdir)/$(build)/%.pcm.md Makefile | $(build) 
	$(CXX) -target x86_64-unknown -c $< -o $@ $(cxx_flags) -Xclang -emit-module-interface -fimplicit-modules -fimplicit-module-maps -fprebuilt-module-path=$(build)

$(build)/%.o: $(build)/%.pcm Makefile | $(build)
	$(CXX) -target x86_64-unknown -c $< -o $@ $(pcm_cxx_flags)

$(depdir)/$(build)/%.pcm.md: $$(subst .,/,%).cpp Makefile | $(depdir)
	@gawk '{ if (match($$0, /import\s+([a-zA-Z0-9\._]+);/, arr)) print "$(patsubst %.pcm,$(build)/%.pcm,$(subst /,.,$(patsubst %.cpp,%.pcm,$<)))" ": $(build)/" arr[1] ".pcm"; }' $< > $@
