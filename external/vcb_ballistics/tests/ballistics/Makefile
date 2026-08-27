CC ?= cc
BUILD_DIR ?= /tmp/vcb-ballistics-validator
TARGET := $(BUILD_DIR)/vcb_ballistics_validator
SOURCES := test_vcb_ballistics.c ../../source/vcb_ballistics.c

CPPFLAGS += -I../../source
CFLAGS += -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror
LDLIBS += -lm

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SOURCES) ../../source/vcb_ballistics.h
	mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) $(LDLIBS) -o $@

run: $(TARGET)
	$(TARGET) $(CURDIR)/reference_cases.csv $(CURDIR)/reference_cases_g7.csv

clean:
	rm -f $(TARGET)
