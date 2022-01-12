CXX ?= c++

uname_s := $(shell uname -s)
ifeq ($(uname_s),Darwin)
  CXX_LD ?= ld.lld
else
  CXX_LD ?= ld
endif

QEMU_PARAMS = -cpu max -serial stdio -smp 2

CXX_FLAGS += -ffreestanding -nostdlib -nostdinc -fno-exceptions -fno-rtti \
	     -fpic -fno-stack-protector -mno-red-zone --std=c++20

build = build
kernel_sources = $(wildcard kernel/*.cpp)
kernel_objects = $(patsubst %.cpp,$(build)/%.o,$(kernel_sources))

all: $(build)/boot.img

$(build)/paraos: $(kernel_objects) $(kernel_sources) kernel/bootboot.ld Makefile
	$(CXX_LD) $< -T kernel/bootboot.ld -o $@ -e bootboot_main -nostdlib 

$(build)/%.o: %.cpp $(kernel_sources) Makefile
	@mkdir -p $(dir $@)
	$(CXX) -target x86_64-unknown -c $< -o $@ $(CXX_FLAGS)

$(build)/boot.img: $(build)/paraos tools/mkbootimg/mkbootimg boot.config bootimage.json
	tools/mkbootimg/mkbootimg check $<
	tools/mkbootimg/mkbootimg bootimage.json $@

tools/mkbootimg/mkbootimg: $(filter-out tools/mkbootimg/mkbootimg,$(wildcard tools/mkbootimg/*.*))
	$(MAKE) -C tools/mkbootimg
	rm -f tools/mkbootimg*.zip	

qemu: $(build)/boot.img
	qemu-system-x86_64 -drive format=raw,file=$< $(QEMU_PARAMS)
 
qemu-gdb: $(build)/boot.img
	qemu-system-x86_64 --drive format=raw,file=$< $(QEMU_PARAMS) -S -s

clean:
	rm -rf $(build)
	$(MAKE) -C tools/mkbootimg clean
