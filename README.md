# emul_fb
A Linux userspace framebuffer emulator.

The emulator creates a framebuffer device under `/dev/fb_emulator`


## Dependencies
The emulator uses `libfuse3` to create a userspace device driver and `SDL2` for the window showing the framebuffer content.

On Ubuntu the development dependencies can be installed with:

```shell
sudo apt-get install build-essential git make cmake autoconf automake libtool pkg-config libasound2-dev libpulse-dev libaudio-dev libjack-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxinerama-dev libxxf86vm-dev libxss-dev libgl1-mesa-dev libdbus-1-dev libudev-dev libgles2-mesa-dev libegl1-mesa-dev libibus-1.0-dev fcitx-libs-dev libsamplerate0-dev libsndio-dev libwayland-dev libxkbcommon-dev libdrm-dev libgbm-dev

sudo apt install libsdl2-2.0-0 libsdl2-dev libfuse3-dev
```

## Build
The project is build using `cmake` version 3.16 or higher. Kitware has an [apt repository](https://apt.kitware.com/) if you Ubuntu version has an older version per default.

Best practice is to make an out of library build with cmake. Open a terminal and navigate into the source directory, then do:

```shell
mkdir build
cd build
cmake .. && make
sudo make install
```

## Operation
Because of the required FUSE privileges, the program is installed with the SUID bit set, so it can be invoked by any user. In a terminal simply execute:

```shell
emul_fb
```
This will create a user accessible framebuffer device located at `/dev/fb_emulator`.

### Test
Open another terminal and run the following command to fill the framebuffer with random pixel data:

```shell
cat /dev/urandom >> /dev/fb_emulator
```


