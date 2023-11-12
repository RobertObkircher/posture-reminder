# posture-reminder

Uses face recognition to remind you to sit up straight

## Usage

**WARNING**: This project is still a work in progress. For now it only works on my machine. I've never tested it on windows.

![Webcam filming person sitting in front of computer](usage.png)

Run `./posture_reminder` and sit still until the desired position of your face appears in green.

From now on the application will check it regularly and play a sound if your posture sucks.

| Key                            | Description               |
|--------------------------------|---------------------------|
| <kbd>esc</kbd> or <kbd>q</kbd> | quit w                    |
| <kbd>space</kbd>               | Set the desired position. |
| <kbd>p</kbd>                   | pause                     |

## Installation

1. Make sure you have the following software:
    - opencv: https://opencv.org/releases/
    - a C++ compiler

2. Clone this repository or download it as a zip file

2. Build:

| Tool          | Description                  | OS      |
|---------------|------------------------------|---------|
| CLion         | Can simply open this project | any     |
| Visual Studio | Can simply open this project | Windows |
| cmake         | see below                    | any     |
| g++           | `sh compile.sh`              | Linux   |

### cmake

Figure out if you have a single or multi configuration generator:

```shell
cmake --help
cmake --help-property GENERATOR_IS_MULTI_CONFIG
```

For single-config-generators such as `Unix Makefiles` and `Ninja`:

```shell
cmake -S "." -B "build/Release" -D CMAKE_BUILD_TYPE=Release
cmake --build "build/Release"
```

For multi-config-generators such as `Ninja Multi-Config` and `Visual Studio`:

```shell
cmake -S "." -B "build"
cmake --build "build" --config Release
```
