CC = gcc
CFLAGS = -Wall -Wextra -I./include
LDFLAGS = 
BUILD_DIR = build

.PHONY: all clean run-cli run-driver

all: clean $(BUILD_DIR)/cli $(BUILD_DIR)/driver

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/cli: $(BUILD_DIR)/cli_main.o $(BUILD_DIR)/cli.o $(BUILD_DIR)/driver_list.o $(BUILD_DIR)/common.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/driver: $(BUILD_DIR)/driver_main.o $(BUILD_DIR)/driver.o $(BUILD_DIR)/common.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/cli_main.o: src/cli/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/cli.o: src/cli/cli.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/driver_list.o: src/cli/driver_list.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/driver_main.o: src/driver/main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/driver.o: src/driver/driver.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/common.o: src/common/common.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

run-cli: $(BUILD_DIR)/cli
	./build/cli

run-driver: $(BUILD_DIR)/driver
	./build/driver