Circular Buffer with SDL Graphics
This repository contains a C program that demonstrates the use of a circular buffer along with SDL graphics to visualize data from /dev/urandom in a graphical window.

Overview
The program consists of two threads: one reads random data from /dev/urandom and pushes it into a circular buffer, and the other thread renders the data as colorful blocks on an SDL window.

Features
Circular buffer implementation for multi-threaded data management.
Use of SDL2 library for graphics rendering.
Random data read from /dev/urandom.
Colorful blocks drawn on the screen based on the random data.
Graceful handling of program termination with signal handling.

How to Use
Clone this repository to your local machine:
git clone <https://github.com/yourusername/circular-buffer-sdl.git>
cd circular-buffer-sdl

Compile the program using GCC:
make

Run the compiled executable:
./circular_buffer_plot
The program will open a window displaying colorful blocks based on the random data read from /dev/urandom. You can close the window to terminate the program.

Requirements
GCC (GNU Compiler Collection)
SDL2 library (libsdl2-dev package on Ubuntu/Debian)
Linux environment (tested on Ubuntu 20.04)
License
This project is licensed under the MIT License - see the LICENSE file for details.

Acknowledgments
The implementation is based on the concept of circular buffers and multi-threading.
