# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -std=c++17

# Source files
SRCS = src/main.cc

# Output binary
TARGET = gnome-cec-screensaver

# Use pkg-config to get compiler and linker flags
PKG_CONFIG_LIBS = libcec glibmm-2.68 giomm-2.68
CXXFLAGS += $(shell pkg-config --cflags $(PKG_CONFIG_LIBS))
LDFLAGS += $(shell pkg-config --libs $(PKG_CONFIG_LIBS))

# Build target
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

# Clean up build artefacts
clean:
	rm -f $(TARGET)

install: $(TARGET)
	cp -f gnome-cec-screensaver ~/.local/bin
	cp gnome-cec-screensaver.desktop ~/.config/autostart