TARGET?=$(ARCH)-linux-gnu

CC=clang
LD=ld.lld

NM=nm
AS=$(CC)
OBJ_DIR=obj-$(TARGET)

CFLAGS=\
	-nostdinc -Wall -fno-common -fno-PIC -Wno-format-security \
	-Wno-bitwise-op-parentheses -Wno-shift-op-parentheses \
	-std=gnu11 -fno-jump-tables -Wno-frame-address -fno-stack-protector \
	-fno-unwind-tables -fno-omit-frame-pointer -fno-asynchronous-unwind-tables \
	-fplt -O0 \
	-I sysroot-$(TARGET)/include \
	-I .

LDFLAGS=

ifeq (clang, $(CC))
CFLAGS += --target=$(TARGET) -fno-addrsig
endif

ifeq (x86_64, $(ARCH))
# CFLAGS += -mretpoline -mretpoline-external-thunk
CFLAGS += -mcmodel=kernel
endif

ifeq (aarch64, $(ARCH))
CFLAGS += -mcmodel=tiny
LDFLAGS += --fix-cortex-a53-843419
endif

all: $(OBJ_DIR)/module.ko

sysroot-$(TARGET)/lib/libc.a:
	sh build-runtime.sh "$(TARGET)" "$(CC)" "$(CFLAGS)"

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o: %.c sysroot-$(TARGET)/lib/libc.a $(OBJ_DIR)
	$(CC) ${CFLAGS} -c -o $@ $<

$(OBJ_DIR)/module.stage0.o: $(OBJ_DIR)/module.o $(OBJ_DIR)/kagent.o
	$(LD) $(LDFLAGS) -o $@ $^ sysroot-$(TARGET)/lib/libc.a sysroot-$(TARGET)/lib/librt.a -r -z noexecstack --build-id=sha1

$(OBJ_DIR)/module.stage1.o: $(OBJ_DIR)/module.stage0.o
	$(NM) -u $< >$(OBJ_DIR)/module.stage1.lst
	python3 resolve.py -o $(OBJ_DIR)/module.stage1.c $(OBJ_DIR)/module.stage1.lst
	$(CC) $(CFLAGS) -c -o $(OBJ_DIR)/module.stage1.o $(OBJ_DIR)/module.stage1.c

$(OBJ_DIR)/module.ko: $(OBJ_DIR)/module.stage1.o
	$(LD) $(LDFLAGS) -o $@ $(OBJ_DIR)/module.stage0.o $(OBJ_DIR)/module.stage1.o -r -z noexecstack --build-id=sha1

clean:
	rm -rf build-$(TARGET)
