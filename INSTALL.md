# Installing

## Desktop

Requirements:

* POCO libraries (`brew intall poco`, `pacman -S mingw64/mingw-w64-x86_64-poco`)
* GLFW library (`brew intall glfw`, `pacman -S mingw64/mingw-w64-x86_64-glfw`)

First build the `clip` library:

    cd clip
    mkdir build
    cd build
    cmake ..
    make
    cd ../..


Next build the main programs:

    make

## Magic Leap One

Requirements:

* The Lab (Native and Lumin SDKs, 0.23+)
* Python 3.x in path (The Lab comes with a python interpreter, you can use that)

1. Open a CLI window from The Lab's left hand pane (bottom icon)
2. `cd` to project directory.
3. Run `mabu vklBoards.package -t debug_device -s <path-to-certificate>.cert`
4. Deploy .out/vklBoards/vklBoards.mpk to the device.

# Running

The server process:

    ./board_server <port>

The GUI client:

    ./gui_client <server_uri>
