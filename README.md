# About

A screensaver for GNOME that will turn on and off your HDMI-CEC enabled devices using a libcec compatible HDMI-CEC adapter.
The application hooks into GNOME's screensaver event and detects when it has been activated/deactivated.
Note, all HDMI-CEC signals sent are broadcasted and are not device specific.

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

Also, make sure that you're a member of the dialout group to give you access to the USB HDMI-CEC device, usually `/dev/ttyACM0`.

```
sudo usermod -aG dialout $USER
```

This will put the binary in the user's bin directory so it can be run on the command line with `gnome-cec-screensaver` and copy the desktop file into the autostart directory so GNOME will run it at startup.

The application will log to the command line as well as `.gnome-cec-screensaver.log` in the user's home direcory.

I have only tested this with the [USB - CEC Adapter](https://www.pulse-eight.com/p/104/usb-hdmi-cec-adapter) from Pulse-Eight connected to my NVIDIA GPU on Fedora 40/41/42.
