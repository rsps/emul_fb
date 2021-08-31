# emul_fb
A Linux userspace framebuffer emulator.

The emulator contains a kernel driver which creates a framebuffer device under `/dev/fbX`, where X is the next available frame buffer device.

It also creates a `/dev/fb_view` character device, which is used to notify the viewer (`emul_fb`) when the frame buffer content changes and needs to be redrawn.


## Dependencies
The emulator uses `SDL2` for the window showing the framebuffer content.

On Ubuntu the development dependencies can be installed with:

```shell
sudo apt-get install build-essential git make cmake autoconf automake libtool pkg-config libasound2-dev libpulse-dev libaudio-dev libjack-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxinerama-dev libxxf86vm-dev libxss-dev libgl1-mesa-dev libdbus-1-dev libudev-dev libgles2-mesa-dev libegl1-mesa-dev libibus-1.0-dev fcitx-libs-dev libsamplerate0-dev libsndio-dev libwayland-dev libxkbcommon-dev libdrm-dev libgbm-dev

sudo apt install libsdl2-2.0-0 libsdl2-dev
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
The required kernel module must be installed before emul_fb can be invoked. In a terminal simply execute:

```shell
sudo modprobe vfb2
emul_fb
```

This will create a user accessible framebuffer device located at `/dev/fbX` and a viewer notification on `/dev/fb_view`.

*Note:* The kernel module can be loaded automatically at boot time, by entering its name in `/etc/modules`.


### Test
Open another terminal and run the following command to fill the framebuffer with random pixel data, then start `emul_fb` to show the content:

```shell
cat /dev/urandom >> /dev/fbX
emul_fb
```


