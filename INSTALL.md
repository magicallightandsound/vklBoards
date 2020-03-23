# Installing

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

# Running

The server process:

    ./board_server <port>

The GUI client:

    ./gui_client <server_uri>
