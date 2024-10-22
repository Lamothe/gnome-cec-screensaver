# About

A screensaver for GNOME that will turn off and on your TV using a libcec compatible HDMI-CEC adapter.
The application hooks into GNOME's screensaver event and detects when it has been activated/deactivated.
It makes an assumption that the TV is the only thing connected and defaults to sending the signal to address 0.

# Install Prerequisites (Fedora)

```
sudo dnf install -y git make libcec-devel glibmm2.68-devel gcc-c++
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

If you want to run this at startup, make sure that the binary is in the path and put the gnome-cec-screensaver.desktop file in the ~/.config/autostart directory.

```
ln -s $PWD/gnome-cec-screensaver ~/.local/bin
cp gnome-cec-screensaver.desktop ~/.config/autostart
```

or use `make install`.

I have only tested this with the [USB - CEC Adapter](https://www.pulse-eight.com/p/104/usb-hdmi-cec-adapter) from Pulse-Eight connected to my NVIDIA GPU on Fedora 40.
