# vklBoards

Whiteboard client/server app for desktop and Magic Leap.

## Description

vklBoards is a project that contains 3 things:

1. A server application, intended to be run from the command line as a daemon, for example, on an AWS instance. The server holds the whiteboard content for all whiteboards being hosted.
2. A desktop GUI client application that connects to the server, which provides paint-like access to the whiteboards on the server one-at-a-time.
3. A Magic Leap One app, which connects to the server with the help of the GUI client, which provides simultaneous access to multiple spatially placed whiteboards.

## Intended user experience

vklBoards is a very simple set of programs, written in a quick-and-dirty fashion, without the expectation of stability or security. The assumption is that the networked programs are all run on a single virtualized local area network so that only trusted parties have access to the network data. Furthermore, there is currently no user tracking, chat, or audio/video conferencing facilities. It is assumed that users will also be signed into a teleconferencing solution (e.g. Google Hangouts) while simultaneously using vklBoards.

From a user's standpoint, the usage is to first join a teleconferencing session with collaborators.
Next, the GUI desktop client is used to connect to a running server, by specifying the server IP address and port.
Next, the Magic Leap app is started and connected using the QR code displayed by the GUI desktop client.

Collaboration occurs primarily through the teleconferencing session and the Magic Leap app, while the occasional content may be thrown onto a board using the desktop client, for marking up in the virtual environment. It should be emphasized that vklBoards is not a co-presence experience; each user may place the same whiteboard in different locations, even though they may share the same physical environment. For a truly co-presence experience, try the [Spatial](https://spatial.io/) app.

## Features

* Real-time collaborative MS Paint-like drawing capabilities on 2048 x 1024 pixel canvases.
* Multiple boards may be displayed and placed in the virtual environment.
* Image import from PNG files (via drag-and-drop into the GUI desktop client).
* Clipboard image import (Ctrl-V in GUI desktop client).
* Whiteboard content export (through GUI desktop client).

## Architecture

The server is a simple TCP socket server (using the POCO framework) that handles the simple messaging protocol for relaying drawing updates between clients.
The desktop uses GLFW to provide windowing support, and ImGui to provide a basic user interface.
All drawing is done onto a texture on the GPU.
The Magic Leap app similarly renders to a texture, but supports multiple simultaneously displayed whiteboards.
