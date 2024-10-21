# About

A screensaver for GNOME that will turn off and on your TV using HDMI-CEC.
There's nothing extra to set up in GNOME/GDM/X, the application hooks into GNOME's screensaver event and detects when it has been activated/deactivated.
The application makes an assumption that the TV is the only thing connected and defaults to sending the signal to address 0.

I have only tested this with the [USB - CEC Adapter](https://www.pulse-eight.com/p/104/usb-hdmi-cec-adapter) from Pulse-Eight connected to my NVIDIA GPU on Fedora 40.

# Install Prerequisites

```
sudo dnf install git make libcec-devel glib2-devel gcc-c++
```

# Download and Build

```
git clone https://github.com/Lamothe/gnome-cec-screensaver.git
cd gnome-cec-screensaver
make
```

# Run

```
./gnome-cec-screensaver
```
