CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -Isrc
LDFLAGS ?= -lpthread

TARGET = emergency-proxy
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

SRCS = src/main.cpp \
       src/emergency_proxy.cpp \
       src/openvpn_tunnel.cpp \
       src/proxy_server.cpp \
       src/wsnet/WSNet.cpp \
       src/wsnet/emergency_connect_impl.cpp \
       src/utils/config.cpp \
       src/utils/logger.cpp \
       src/utils/process.cpp \
       src/utils/network.cpp \
       src/utils/string_utils.cpp

OBJS = $(patsubst src/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

TEST_TARGET = $(BUILD_DIR)/test_wsnet
TEST_SRCS = tests/test_wsnet.cpp \
            src/wsnet/WSNet.cpp \
            src/wsnet/emergency_connect_impl.cpp \
            src/utils/logger.cpp

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built target: $(TARGET)"

$(OBJ_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	@echo "Running tests..."
	@./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Cleaned build artifacts"

