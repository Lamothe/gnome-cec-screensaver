# About

A screensaver for GNOME that will turn off and on your TV using a libcec compatible HDMI-CEC adapter.
The application hooks into GNOME's screensaver event and detects when it has been activated/deactivated.
It makes an assumption that the TV is the only thing connected and defaults to sending the signal to address 0.

# Install Prerequisites (Fedora)

```
sudo dnf install -y git make libcec-devel glibmm2.68-devel gcc-c++
```

# Download, Build and Install

```
git clone https://github.com/Lamothe/gnome-cec-screensaver.git
cd gnome-cec-screensaver
make install
```

This will put the binary in the user's bin directory so it can be run on the command line with `gnome-cec-screensaver` and copy the desktop file into the autostart directory so GNOME will run it at startup.

The application will log to the command line as well as `.gnome-cec-screensaver.log` in the user's home direcory.

I have only tested this with the [USB - CEC Adapter](https://www.pulse-eight.com/p/104/usb-hdmi-cec-adapter) from Pulse-Eight connected to my NVIDIA GPU on Fedora 40.
