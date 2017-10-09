ips
===

*ips* is a test bed environment to design and test various image processing
algorithms.

## Prerequisites

### Software

* *CMake* `2.8.0`

*Visual Studio 2015* `Update 3` on Windows or *Xcode* `7.3.1` on macOS or any
compatible compiler and a Make utility on Unix/Unix-like systems including macOS
and Linux.

## Usage

Create a `build` directory and set it as a current working directory.

Configure the project to generate IDE or Make files

```bash
# on Windows for Visual Studio
cmake .. -G "Visual Studio 14 Win64"

# on MacOS for Xcode
cmake .. -G "Xcode"

# on MacOS or Linux to generate Make files
cmake .. -G "Unix Makefiles"
```

Build the project

```bash
cmake --build .
```

Run the program from the `build/ips/` directory

```bash
./ips [path to a png image]
```

On Windows you can also drag and drop an image file to manipulate into the
program's window.

## Credits

ips was created by [Dmitrii Toksaitov](https://github.com/toksaitov).

