# About

A screensaver for GNOME that will turn off and on your TV using HDMI CEC.
There's nothing extra to set up in GNOME/GDM/X, the application hooks into GNOME's screensaver event and detects when it has been activated/deactivated.
The application makes an assumption that the TV is the only thing connected and defaults to sending the signal to address 0.
I believe that the GNOME screensaver project is unstable at the moment so its interface might change in the future.
I have only tested this with the USB CEC adapter from Pulse Eight connected to my NVIDIA GPU.

# Install pre-requisites

`sudo dnf install libcec-devel glib2-devel gcc-c++`

# Build

`make`

# Run

`./gnome-cec-screensaver`
