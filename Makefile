# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++17

# Source files
SRCS = src/main.cpp

# Output binary
TARGET = gnome-cec-screensaver

# Use pkg-config to get compiler and linker flags
PKG_CONFIG_LIBS = libcec gio-2.0 glib-2.0
CXXFLAGS += $(shell pkg-config --cflags $(PKG_CONFIG_LIBS))
LDFLAGS += $(shell pkg-config --libs $(PKG_CONFIG_LIBS))

# Build target
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

# Clean up build artifacts
clean:
	rm -f $(TARGET)
