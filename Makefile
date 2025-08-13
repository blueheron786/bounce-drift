# Makefile for GBA Project

DEVKITPRO ?= C:/devkitPro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
LIBGBA ?= $(DEVKITPRO)/libgba

CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++

SRC_DIR = src
OBJ_DIR = obj
SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

 CFLAGS = -I$(LIBGBA)/include -Iinclude -mthumb -mthumb-interwork -O2
LDFLAGS = -mthumb -mthumb-interwork -specs=gba.specs -L$(LIBGBA)/lib -lgba

TARGET = hello
OUT_DIR = build/obj/output
ELF = $(OUT_DIR)/$(TARGET).elf
BIN = $(OUT_DIR)/$(TARGET).gba


all: $(TARGET).gba

$(ELF): $(OBJ) | $(OUT_DIR)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

$(BIN): $(ELF) | $(OUT_DIR)
	arm-none-eabi-objcopy -O binary $< $@
	./gbafix.exe $@

$(TARGET).gba: $(BIN)
	mv $(BIN) $(TARGET).gba

$(ELF): $(OBJ) | $(OUT_DIR)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

clean:
	rm -rf $(OBJ_DIR)/*.o $(OUT_DIR)/*.elf $(OUT_DIR)/*.gba

.PHONY: all clean