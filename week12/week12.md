# Week 12

## Running tests
So far, I have relied on the nrf connect extension to get the environment going to build and run tests. Good enough for now. With a project with no build configurations, right click the project under the applications tab and click "Start New Terminal". This starts a terminal within VS code that you can run West commands from. Ensure you're in your project directory (the one with your src, proj.conf, CMakeLists.txt, etc). Run:
```
west build -b native_sim/native
```
That will create a build directory for you that's for running the tests natively on your computer. Then run:
```
<path/to/cmake> --build <path/to/build/directory>
```
For me, working on a project called test0, that's:
```
/home/adam/ncs/toolchains/b77d8c1312/usr/local/bin/cmake --build /home/adam/zephyrProjects/test0/build
```
This should give you an executable file located at the following, assuming you're still in the project directory.
```
./build/test0/zephyr/zephyr.exe
```
Running that file will run your test suite in ```./src/main.c```!

## What I got done this week
I was not in the best mental headspace this week and struggled to get anything significant done in terms of testing... Basically just a hello world sort of thing that runs natively on Linux. But I was able to drive myself enough to get BLE working through Zephyr on the nRF52840, so that's good! Like many things with Zephyr the learning curve was steep but once I figured out how to get it running it wasn't too bad and I kinda like the API.