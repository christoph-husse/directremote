# DirectRemote

DirectRemote is a prototype project, trying to bring the XenDesktop experience of hardware accellerated screen codecs, which allow for realtime streaming of any 3D application (including games), to the consumer market. That is, DirectRemote runs on consumer hardware, like recent Intel CPUs and recent NVIDIA/ATI consumer products. It is designed to be highly modular. All functionality is encapsulated within C-API wrapped C++ interfaces and thus allow any component to be consumed or provided by any programming language that can consume or export C-ABI. I hope that this fact alone encourages people to try to experiment by integrating this prototype into existing products.

For a decent video quality, you need about 10 MBits upstream, which is not much from a data-center perspective, but currently most consumers uplinks could not sustain this datarate at all. But consumers can easily connect to PCs at work and also business presentations/meetings will usually have better uplinks.

### 10.000 Feets Architecture

Before going any deeper, you should have an overview of how the project is structured and have established a common set of things we can speak about without explaining at every turn.

So let's start from scratch... 

We have nothing, except a recent gaming PC running Windows 8.1, and want to mirror our screen we see before us onto our Linux Notebook standing next to it. The first thing we need to do is start the DirectRemote Host (DRHost), which currently is a background process that needs to be terminated manually through the TaskManager (sorry for that). Its sole purpose is to capture whatever is visible on your main desktop (screen 0, also called "Host System" or "Host"), encode it into an H.264 video stream using either your graphics card or CPU, and then send the whole thing somewhere. Also it will replay input made on DirectRemote Viewers (SDLViewer binary running on the "viewer system" or "viewer") on the host system.

The "somewhere" for now is a simple UDP proxy (UDPProxy binary), which can run anywhere... The ideal situation (with the best latency) is when the UDPProxy runs on the host system, which requires the host system to be accessible by any viewer. Firewalls make that hard, which is why the proxy will commonly run on some server in the internet. 

The SDLViewer is a thin client application that really does nothing else than decoding the H.264 stream, displays the image, collects some performance metrics and sends the metrics plus the input made on viewer back to the host.

### Supported Systems

The DRHost currently runs only under Windows 8.1... This is because Windows 8 introduced a new screen capture API called "Desktop Duplication API", which gives you a DirectX 11 texture of the screen content (isn't that just awesome?!).

The SDLViewer runs on Mac OS 10.8+ (even though it only compiles on 10.10 for now), Linux (compiles with Ubuntu 14.04 LTS but potentially runs on any linux with a 2.6+ kernel and OpenGL). and Windows (I am not sure, but once the single file cross compilation on Linux works, the resulting executable should really run on any Windows at least back to 2000 if not even 95/98). 

There is another viewer, called UnityViewer. It is currently out of sync and will not work, but I will fix it soon. It allows you to use the whole viewer, which is encapsulated in a "DRViewerLib" module, through Unity3D and thus potentially can be deployed to any target that supports FFMPEG and Unity3D...

The UdpProxy should really run on anything that has a true "kernel" and supports C++11 as well as Udp Sockets.

### Build it yourself

One of the core tasks with every project is building it. Right now, this is not trivial, but its also not terribly difficult. 
The general concept is as follows. In the project root, there is a "build.py" script. You need to invoke it with Python 2.7... It provides a lot options for customization, some of which will be covered in the following sections. This script currently invokes a CMake build that is correctly parameterized depending on settings, source & target system. The complexity comes from the many different systems, cross-compilation and flags that need to be supported at once. If you want to dig deeper, look at the script and the "CMakeLists.txt" files.

There is an additional set of build scripts located in "dependencies/build-deps/*". Those are needed to build dependencies on Linux and MacOS, while on Windows all is prebuilt already (and yes, it's not nice to get up and running without using those prebuilts). On linux, you need to call "sudo bash ./apt-get-ubuntu-14.04.sh" to install all prerequisites for your Ubuntu 14.04 LTS 64-Bit (any other system is not supported and you need to find out yourself how to build on it). Then call "bash ./configure.sh", followed by "bash ./make.sh" and your are done :). To cross compile for Windows, you also need to call "bash ./make-i686-w64-mingw32.sh" and "bash ./make-x86_64-w64-mingw32.sh", which will build SDL and FFMPEG dependencies for windows. On MacOS 10.10 its similar, but you will have a bumpy ride here, since it is more of a patchwork for now (I just wanted to get an impression, it's not a supported build yet).

##### Building on Windows 8.1

If you want to build on Windows, your should have Windows 8.1 and Visual Studio 2013 (Bamboo can build it too). If you have DirectX SDK installed, make sure to set the environment variable "DXSDK_DIR" to an empty string before running the build.

Also note that "just" Windows is not enough to fully compile this project. The main issue is FFMPEG, which is bundled into one single DLL, exporting DirectRemote's IVideoDecoder interface. While it is possible on Windows to create a working SDLViewer from scratch, it is terribly convoluted and causes you to end up with 10 DLLs you need to ship. Instead, the FFMPEG based video decoder DLL is cross-compiled on Linux and will produce a single DLL with a minimal FFMPEG distribution that just includes the features we use (on MacOS, a 7zipped SDLViewer is only 900KB in size). To make it easy for you to get started, this decoder comes prebuilt and only if you want to modify the code behind it (the VideoDecoder_ffmpeg project), you need Linux.

The DRHost can only be built & run on Windows 8.1 for now. 

So after checking out the GIT repo, go to the root folder and run 
```sh
python ./build.py --clean
```

and you will find the executables in "./bin/windows-i386" or "./bin/windows-x86_64" depending on your system. You can decide explicitly by passing "--target=windows-i386" or the other respectively.

If you want to use Visual Studio 2013, you will find a solution in "./_build", which you can open and use for debug & development. For debugging, make sure that the working directories for all executables are set to the binary output directory as stated above. Otherwise it may not find the plugins and fail strangely. Also set the executable to debug to the respective ones in the binary output folder. So for DRHost in a 32-bit build this gives:

```sh
"Working Directory"=$(ProjectDir)../../bin/windows-i386/
"Command"=$(ProjectDir)../../bin/windows-i386/DRHost.exe
```

You can set these paths in the "Debugging" tab in the respective VS2013 project properties dialog.

##### Building on Ubuntu 14.04 LTS 64-Bit

There are a few things to keep in mind on linux. Number one is that you need to build the dependencies yourself (see "make.sh" invokation above). This only needs to be done once. Number two is that the DRHost is not supported, so it must be disabled. Also, dynamic module loading is currently not implemented on Linux, so you must enable static plugins, which basically hard-codes dependencies inside the code. If you want to disable modules altogether and just get a single, standlone SDLViewer, libDRViewerLib.so and libVideoDecoder_ffmpeg.so, then just pass "--static-linkage" in addition.

```sh
python ./build.py --clean --disable-host --static-plugins
```

Successive invocations can omit "--clean" to avoid a full rebuild, but its sometimes hard to know when you need a clean, so do it at your own risk.

The above will yield executables in "./bin/linux-i686/" or "./bin/linux-x86_64" depending on your system (currently, deviating from your host bitness is not supported on Linux).

You are done! I recommend Clion or Sublime Text as IDE :).

##### Cross-compiling on Ubuntu 14.04 LTS 64-Bit

Cross-compilation is needed for creating Windows dependencies as well as later supporting Android & other targets. Currently, only MinGW is supported. To do a cross build, invoke:

```sh
python ./build.py --clean --disable-all --static-plugins --static-linkage --target=%target% --enable-viewer-lib --enable-ffmpeg
```

and replace "%target%" with either "i686-w64-mingw32" or "x86_64-w64-mingw32". Currently, only the viewer library and the FFMPEG decoder can be cross compiled, which is all we need for now since all devices that depend on UnityViewer will only need the viewer library and not the SDLViewer or DRHost.

If you want to get experimental, I will describe how to add a new cross-compilation target. 
First you need to create a CMake toolchain file in "./cmake". You will already see "Toolchain-i686-w64-mingw32.cmake" in there. Those files need to start with "Toolchain-", end with ".cmake" and have the "%target%" in between. You need to pick an existing toolchain file and customize it properly (I hope you see the pattern when comparing the both existing ones).

Further you need to add scripts to "dependencies/build-deps/*/" to build the dependencies with your compiler toolchain. Please look at the three existing "make-%target%.sh" files that are there for Linux and find out how to create your own. This will be fiddling and patchwork, since the dependencies are quite complex and you may need to first find out how to cross compile x264 and FFMPEG and maybe as well as SDL2 (even though the latter is only needed for the SDLViewer)... Please note that the "%target%" needs to match between the cmake file and the output directory to which you copy your dependencies. So for a "Toolchain-%target%,cmake" file you need to have dependencies copied into "./dependencies/%target%/" and invoke "build.py" with "--target=%target%".

On top of that you need to make sure that "./cmake/TargetArch.cmake" can detect your target platform and that the start of "./CMakeLists.txt" properly translates your target into a "platform" variable whose value matches "%target%. This is a tricky part I guess. Good luck :P.



### Okay, that was easy... How do I run it?

Now knowing how trivial cross-compiling to Sun SPARC would be, let's look at how to run it...

First of all, find a place for UDPProxy. It needs to run somewhere and will always listen on "0.0.0.0:41988"... 

Then start the DRHost (please start the host first :P, otherwise you may run into trouble). It accepts one command line parameter, which is and IPAddress-port pair and should point to the UdpProxy. Let's say you run the UdpProxy at "127.0.0.1", then you would start the host like this:

```sh
DRHost.exe 127.0.0.1:41988
```

To see that you are truly up and running, open "./website/perfMon.html" in a browser (on the same machine, DRHost is running on). It should display performance graphs of the running DRHost.

Now let's say that the machine running UdpProxy is at "123.56.34.1", then open the SDLViewer somewhere like this:

```sh
SDLViewer.exe 123.56.34.1:41988
```

And that's basically it. You can run the viewer also on "127.0.0.1:41988" for testing & debugging, in which case the input replay will be automatically disabled, yeah!

### TODO

Well, everything is todo... An incomplete list includes

* Add bandwidth adaptive encoding, so that the fixed H.264 datarate (around 10 MBits), supports a multitude of network speeds.
* Add UnitTests
* Add Integration Tests
* Add Documentation
* Add better protocol handshakes and logging to see why sometimes a connection is lost or not established
* Make use of session-ids (which are somewhat implemented) and pass them as additional console parameter to allow multiple sessions on one UdpProxy
* Handle unicode input to allow writing text instead of just pressing virtual keys individually
* Handle Gamecontrollers 
* Add a virtual Gamecontroller on the host via PPJoy, otherwise viewers with game controllers are of no use
* Support Android and other platforms
* Make Unity3D viewer work again
* Add a WebRTC viewer
* Add a RTP streamer to allow any RTP capable device to show a mirror of your screen
* Add a DRHost UI to control parameters, like which screen to record
* Improve overall UX
* Add launch flows
* Add a webpage similar to G2P, so that I can connect to a host with a "Connect" button
* Add installer for Windows
* Add ATI graphic card support (Videoengine)
* Add optional hardware decoder support for PCs (using the same SDKs that ship the encoder)
* Make MacOS viewer work faster, currently there is something wrong with the network stack as it loses tons of packets
* Make MacOS viewer work on 10.8+
* Test compiled SDLViewer on other linux systems (like Debian 4,5,6,7 and other older ones)
* Make a DRHost for Linux, MacOS, PS4, XBox One
* Support sound transmission (this is not trivial as realtime requirements are much more unforgiving than with video, also capturing is not as streamlined as "Desktop Duplication API")
* Add encryption
* Add HDX3D codec
* and tons more!! This is a huge product xD
