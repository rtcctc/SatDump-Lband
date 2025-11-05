# SatDump

<img src='/icon.png' width='500px' />

A generic satellite data processing software.
*Thanks Crosswalkersam for the icon!*

There now also is a [Matrix](https://matrix.to/#/#satdump:altillimity.com) room if you want to chat! Otherwise, a [Discord](https://discord.gg/ctt3M8pRYG) bridge to the Matrix is also available.

# Introduction

*Note : This is a very basic "how-to" skipping details and assuming some knowledge of what you are doing. For more details and advanced use cases, please see the [detailed documentation](https://docs.satdump.org).* 

## GUI Version

### Offline processing (recorded data)
Quick-Start :
- Choose the appropriate pipeline for what you want to process
- Select the input file (baseband, frames, soft symbols...)
- Set the appropriate input level (what your file is)
- Check settings shown below are right (such as samplerate)
- Press "Start"!

![Img](gui_example.png)

![Img](gui_example2.png)
*SatDump demodulating a DVB-S2 Baseband*

### Live processing or recording (directly from your SDR)

Quick-Start :
- Go in the "Recorder" Tab
- Select and start your SDR Device
- Choose a pipeline
- Start it, and done!
- For recording, use the recording tab instead. Both processing and recording can be used at once.

![Img](gui_recording.png)

## CLI Version

![Img](cli_example.png)

### Offline processing

```
Usage : satdump [pipeline_id] [input_level] [input_file] [output_file_or_directory] [additional options as required]
Extra options (examples. Any parameter used in modules can be used here) :
  --samplerate [baseband_samplerate] --baseband_format [cf32/cs32/cs16/cs8/cu8] --dc_block --iq_swap
Sample command :
satdump metop_ahrpt baseband /home/user/metop_baseband.cs16 metop_output_directory --samplerate 6e6 --baseband_format cs16
```

You can find a list of Satellite pipelines and their parameters [Here](https://docs.satdump.org/pipelines.html).

### Live processing

```
Usage : satdump live [pipeline_id] [output_file_or_directory] [additional options as required]
Extra options (examples. Any parameter used in modules or sources can be used here) :
  --samplerate [baseband_samplerate] --baseband_format [cf32/cs32/cs16/cs8/w8] --dc_block --iq_swap
  --source [airspy/rtlsdr/etc] --gain 20 --bias
As well as --timeout in seconds
Sample command :
satdump live metop_ahrpt metop_output_directory --source airspy --samplerate 6e6 --frequency 1701.3e6 --general_gain 18 --bias --timeout 780
```

You can find a list of all SDR Options [Here](https://docs.satdump.org/sdr_options.html). Run `satdump sdr_probe` to get a list of available SDRs and their IDs.

### Recording

```
Usage : satdump record [output_baseband (without extension!)] [additional options as required]
Extra options (examples. Any parameter used in sources can be used here) :
  --samplerate [baseband_samplerate] --baseband_format [cf32/cs32/cs16/cs8/cu8/w16] --dc_block --iq_swap
  --source [airspy/rtlsdr/etc] --gain 20 --bias
As well as --timeout in seconds
Sample command :
satdump record baseband_name --source airspy --samplerate 6e6 --frequency 1701.3e6 --general_gain 18 --bias --timeout 780 --baseband_format cf32
```

# Building / Installing

### Linux

On Linux, building from source is recommended, but builds are provided for x64-based Ubuntu distributions. Here are some build instructions for common distros.

---

<details>

<summary>Install Dependencies - Debian, Ubuntu, and other Debian-based distros</summary>

```bash
sudo apt install git build-essential cmake g++ pkgconf libfftw3-dev libpng-dev \
                 libtiff-dev libjemalloc-dev libcurl4-openssl-dev libvolk-dev libnng-dev \
                 libglfw3-dev zenity portaudio19-dev libzstd-dev libhdf5-dev librtlsdr-dev \
                 libhackrf-dev libairspy-dev libairspyhf-dev libad9361-dev libiio-dev \
                 libbladerf-dev libomp-dev ocl-icd-opencl-dev intel-opencl-icd mesa-opencl-icd
```
</details>

<details>

<summary>Install Dependencies - Raspberry PI OS</summary>

```bash
sudo apt install git build-essential cmake g++ pkgconf libfftw3-dev libpng-dev \
                 libtiff-dev libjemalloc-dev libcurl4-openssl-dev libvolk-dev libnng-dev \
                 libglfw3-dev zenity portaudio19-dev libzstd-dev libhdf5-dev librtlsdr-dev \
                 libhackrf-dev libairspy-dev libairspyhf-dev libad9361-dev libiio-dev \
                 libbladerf-dev libomp-dev ocl-icd-opencl-dev mesa-opencl-icd
```
</details>

#### Build SatDump
```
git clone https://github.com/rtcctc/SatDump-Lband.git
cd SatDump-Lband
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..

make -j`nproc` #for pc/laptop
make -j1 #for Raspberry PI

# To run without installing
ln -s ../pipelines .        # Symlink pipelines so it can run
ln -s ../resources .        # Symlink resources so it can run
ln -s ../satdump_cfg.json . # Symlink settings so it can run

# To install system-wide
sudo make install

# Run (if you want!)
./satdump-ui
```

### Docker

Building and running under docker is a nice way to separate the build environment and libraries from the host OS.
The build process is a multistage build that uses two images, one with the -dev packages and another for the runtime.
This means that the runtime image can be kept smaller, although the disk space is still needed to complete the build.

To match the system user for the shared files get the same owner, set these in `.env` before building the image.
The user inside the container will always be named `satdump`, but the uid and gid will match the system user.

```bash
# set the current uid and gid 
printf "HOST_UID: $(id -u)\nHOST_GID: $(id -g)\n" > .env

# create the shared directory
mkdir -p srv

# Build the images with compose, 8 parallel
docker compose build --build-arg CMAKE_BUILD_PARALLEL_LEVEL=8

# Launch a shell inside the container/service
docker compose run --rm -it satdump
```

The command that is started inside the container can either be specified at the end on the commandline mentioned above, 
or put in the `.env` with for example `COMMAND: satdump_sdr_server`. If you want to run the sdr server in the background
and have it start automatically on boot, simply launch the container with `docker compose up -d` and check the logs with
`docker compose logs -f`

#### X11 under docker

To use the `satdump-ui` under docker you need to make a few changes.
It is possible to run this on WSL2 as well, change the source to `/run/desktop/mnt/host/wslg/.X11-unix` instead.
In the [docker-compose.yml](docker-compose.yml) you need to uncomment a few lines and make it looks like this:
```yaml
      - type: 'bind'
        source: '/tmp/.X11-unix'
        target: '/tmp/.X11-unix'
```

If the user in the container is not authorized for X11, you will probably get this error message:
`Authorization required, ...`
This is due to the ACL controlling access to your screen.
There's several ways to solve this, a few very broad and insecure, but the following should be acceptable on a non-shared system.

On the host, run: `xhost +local:docker` , then to start the ui: `docker compose run --rm -it satdump satdump-ui`
