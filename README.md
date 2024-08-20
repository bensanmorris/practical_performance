# Overview

A concise practical guide on code performance analysis.

# CPU profiling

## Cache stats (via perf)

Get a quick snapshot of your app's cache performance:

`sudo perf stat -e task-clock,cycles,instructions,cache-references,cache-misses  ./myapp`

Sample a stat further using perf record:

`sudo perf record -e cache-misses ./myapp`

Print a summary report:

`sudo perf report --stdio`

## CPU Flamegraphs

Flamegraphs (created by Brendan Gregg) are very useful in helping you quickly identify code hotspots. 

1. Launch your linux docker container with SYS_ADMIN capabilities: `docker run --privileged --cap-add SYS_ADMIN -t [your_image_id] /bin/bash` . From this point forward we are inside our launched container
2. `sudo sysctl -w kernel.kptr_restrict=0` and `sudo sysctl -w kernel.perf_event_paranoid=1`
3. clone Brendan Gregg's Flamegraph repo: `git clone https://github.com/brendangregg/Flamegraph.git`
4. clone and build your code (with symbols)
5. run your code / unit tests under perf. As I use gtest, typically I list my  tests and choose one `[my_gtest_app] --gtest_list_tests`
6. next, let's run our gtest of interest under perf: `perf record -a -g ./my_gtest_app --gtest_filter=MyTestSuite.MyTest` ([see here for perf record sampling docs](https://perf.wiki.kernel.org/index.php/Tutorial#Sampling_with_perf_record))
7. run perf script to generate trace output from perf data: `perf script > out.perf`
8. next, we'll normalise this data and fold all the call-graphs: `../../Flamegraph/stackcollapse-perf.pl out.perf > out.folded`
9. now let's generate the flamegraph: `../../Flamegraph/flamegraph.pl out.folded > out.svg`
10. you should now have an out.svg file in the current directory
11. let's copy the out.svg from our container to our host. In a separate shell (i.e. outside our container), obtain your running container's name: `docker ps` then: `docker cp your_container_name:/your/out.svg/location/out.svg .`
12. open it in an svg viewer (a web browser for instance)

The above summarised:
NB. To install perf (on Ubuntu): `sudo apt-get install linux-tools`
```
#!/bin/bash
process=$@
if [ ! -d Flamegraph ]; then
    git clone https://github.com/brendangregg/Flamegraph.git
fi
perf record -g -p $process
perf script > out.perf
Flamegraph/stackcollapse-perf.pl out.perf > out.folded
Flamegraph/flamegraph.pl out.folded > flamegraph.svg
```

## Other tools

- [Intel VTune Profiler](https://software.intel.com/content/www/us/en/develop/documentation/vtune-help/top.html) - Good for getting an overview of your app's utilisation of CPU across your app's threads (red = spinning, green = good utilisation).

# Memory profiling

## Memory Flamegraphs

[See Brendan Gregg's Memory Flamegraphs](https://www.brendangregg.com/FlameGraphs/memoryflamegraphs.html)

## Finding leaks with eBPF

Install bpf utilities:
```
sudo apt-get install bpfcc-tools linux-headers-$(uname -r)
```
Next, launch your app and grab its pid, then: ```sudo memleak-bpfcc -p123 -a``` replacing ***123*** with your app's process id. [memleak-bpfcc docs](https://manpages.debian.org/unstable/bpfcc-tools/memleak-bpfcc.8.en.html)

Links

- [BCC docs](https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md#bpf-c)
- [Linux kernel tracepoints docs](https://www.kernel.org/doc/html/v4.17/trace/tracepoints.html)
- [Linux kernel tracepoint headers - for details on tracepoint parameters](https://github.com/torvalds/linux/tree/master/include/trace/events)

## perf mem

[perf mem reference](https://www.man7.org/linux/man-pages/man1/perf-mem.1.html) and [redhat perf mem guide](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/monitoring_and_managing_system_status_and_performance/profiling-memory-accesses-with-perf-mem_monitoring-and-managing-system-status-and-performance):

```
(start your app then...)
sudo perf mem record -a sleep 30
perf mem report
```

## valgrind + massif

1. `valgrind --tool=massif --xtree-memory=full ./your_gtest_app --gtest_filter=your_test_suite.your_test`
2. once this completes you can view the output (massif.out.xxx) in the massif-visualizer and also view the xtmemory.kcg.xxx file in kcachegrind

## valgrind + memcheck

1. `valgrind --tool=memcheck --xml=yes --xml-file=./output_file.xml --leak-check=full ./your_gtest_app --gtest_filter=your_test_suite.your_test`
2. for a quick summary let's grab the python ValgrindCI tool: `python -m pip install ValgrindCI --user`
3. for a summary: `valgrind-ci ./output_file.xml --summary` or to use it as part of CI and abort on errors: `valgrind-ci ./output_file.xml --abort-on-errors`

## ctest (cmake)

1. `cd [your_cmake_build_dir]`
2. list your available tests: `ctest -N`
3. pick a test and run it under valgrind's memcheck tool: `ctest -T memcheck -R my_test_name`
4. list the generated memory checker reports: `ls -lat [your_cmake_build_dir]/Testing/Temporary/MemoryChecker*`
5. Fix the leaks - for this start with fixing the **definitely lost** using that description as a search term in your MemoryCheck*.log(s)

# GPU profiling

- [Summary of linux command line utilities for monitoring GPU utilisation](https://www.cyberciti.biz/open-source/command-line-hacks/linux-gpu-monitoring-and-diagnostic-commands/)
- [Renderdoc](https://renderdoc.org/) - great for profiling OpenGL / Vulkan apps across a range of platforms (including Android)

# Locating performance regressions

See [Differential Flamegraphs](http://www.brendangregg.com/blog/2014-11-09/differential-flame-graphs.html)

TODO - add worked example

# Locating performance regressions in your google benchmark history

[I've created a small python utility that generates a chart for each of your google benchmarks as a time series](https://github.com/bensanmorris/benchmark_monitor) (run it over your accumulated benchmark history data) but it also attempts to estimate the location of the build that introduced your slowdown using a sliding window.

## linux

- Use the gold linker: pass this to your `cmake -G etc` line to use the faster gold linker on linux: `-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=gold`

# Computer Vision Performance Profiling / Optimisation

How to profile OpenCV apps:

- set OPENCV_TRACE=1 environment variable (and optionally OPENCV_TRACE_LOCATION to the path to write the OpenCV trace logs to)
- run your app
- generate a top 10 most costly OpenCV functions report as follows: `[opencv_repo_location]/modules/ts/misc/trace_profiler.py [your_opencv_trace_dir]/OpenCVTrace.txt 10` . NB. this report includes run time cost as well as the number of threads that called this function (the "thr" column) - useful when trying to evaluate cpu usage

# OpenCV

- you can influence the number of internal threads used by OpenCV via the `cv::setNumThreads()`
- [OpenCV function call profiling](https://github.com/opencv/opencv/wiki/Profiling-OpenCV-Applications)
- [OpenCV Graph API - an effort to optimise CV pipelines](https://docs.opencv.org/master/d6/dc9/gapi_purposes.html)

# Tensorflow

- just use the prebuilt c libs and cppflow (if they match your target's cpu)

# Tensorflowlite

## Tools

- [Tensorflowlite visualize.py script - generates an HTML page listing your nodes / types and whether they are quantized or not](https://github.com/tensorflow/tensorflow/blob/master/tensorflow/lite/tools/visualize.py)
- [Netron - a GUI based NN visualizer (supports both TF and TFLite format models)](https://github.com/lutzroeder/Netron)

## Performance

- [Tensorflowlite performance best practices](https://www.tensorflow.org/lite/performance/best_practices)
- [Tensorflowlite GPU backend (gpu accelerated tflite)](https://github.com/tensorflow/tensorflow/tree/master/tensorflow/lite/delegates/gpu)
- [Tensorflowlite Neural Net API delegate](https://www.tensorflow.org/lite/performance/nnapi)
- [Tensorflowlite Model quantization (smaller models, potentially better CPU data alignment)](https://www.tensorflow.org/lite/performance/post_training_quantization)

# Android

## CMake

NB. setting `ANDROID_ARM_NEON=ON` will globally enable NEON in CMake based projects but if using NDK >= 21 then NEON is enabled by default.

## Refs

- [Android Neon support](https://developer.android.com/ndk/guides/cpu-arm-neon#cmake)
- [Roy Longbottom's extensive benchmarking of Neon](http://www.roylongbottom.org.uk/android%20neon%20benchmarks.htm)

# Ideas on how to not release slower code

You may already be using benchmarks as an early warning system with respect to performance regression detection in your game engine / software but in case you aren't, this may be useful. In my side project I programatically create a benchmark (using google benchmark) for each 3d game scene in a set of scenes to put my engine's performance through its paces. Then, as part of a CI build I run those benchmarks and feed the results to [my google benchmark charting project that generates a chart for each of your google benchmarks as a time series](https://github.com/bensanmorris/benchmark_monitor). The code (that you will need to modify) to adapt to your engine / scenes is as follows:

First, the `CMakeLists.txt` file that pulls in google benchmark (+ my engine - adapt to your own libs):

```
set(BENCHMARK_ENABLE_TESTING FALSE)
set(BENCHMARK_ENABLE_INSTALL FALSE)
include(FetchContent)
FetchContent_Declare(googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark
    GIT_TAG        "v1.5.4"
)
FetchContent_MakeAvailable(googlebenchmark)

include_directories(${FIREFLY_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIR})

add_executable(firefly_benchmarks benchmarks.cpp)

target_link_libraries(firefly_benchmarks
    benchmark::benchmark
    ${OPENGL_LIBRARIES}
    ${SDL2_LIBRARY}
    ${FIREFLY_LIBRARIES})

add_custom_command(TARGET firefly_benchmarks
    POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/benchmarks $<TARGET_FILE_DIR:firefly_benchmarks>/benchmarks)

```

Next, here's `benchmarks.cpp` (my c++ google benchmark code that prgramatically creates a benchmark for each scene defined in the `scenes` variable, again, adapt to your own engine):

```
#include <benchmark/benchmark.h>
#include <unordered_map>
#include <vector>
#include <set>
#include <firefly.h>
using namespace firefly;

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

static const int SCREEN_WIDTH  = 640;
static const int SCREEN_HEIGHT = 480;
SDL_Window*   displayWindow    = 0;
SDL_GLContext displayContext;

auto BenchmarkScene = [](benchmark::State& st, std::string sceneFilePath)
{
    size_t frames = 0;
    auto scene = SDK::GetInstance().Load(sceneFilePath);
    for(auto _ : st)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch(e.type)
            {
                case SDL_WINDOWEVENT:
                {
                    switch(e.window.event)
                    {
                        case SDL_WINDOWEVENT_RESIZED:
                        {
                            firefly::SDK::GetInstance().OnSize(e.window.data1, e.window.data2);
                            break;
                        }
                    }
                }
            }
        }
        SDK::GetInstance().Update(*scene);
        SDL_GL_SwapWindow(displayWindow);
        frames++;
    }
    scene->Release();
    firefly::SDK::GetInstance().Reset();
};

int main(int argc, char** argv)
{
    std::vector<std::string> scenes = {
        "BallOnPlatform.msf",
        "TiledGrass.msf"
    };

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, FIREFLY_GL_MAJOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, FIREFLY_GL_MINOR_VERSION);
    if(FIREFLY_GL_MAJOR_VERSION >= 3 && FIREFLY_GL_MINOR_VERSION >= 3)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    displayWindow = SDL_CreateWindow("",
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SDL_WINDOWPOS_UNDEFINED,
                                      SCREEN_WIDTH,
                                      SCREEN_HEIGHT,
                                      SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    displayContext = SDL_GL_CreateContext(displayWindow);
    SDL_GL_SetSwapInterval(0);
    firefly::SDKSettings sdkSettings;
    sdkSettings.renderer = firefly::RendererManager::OPENGL;
    sdkSettings.device   = 0;
    sdkSettings.loadProc = (firefly::IRenderer::GLADloadproc)SDL_GL_GetProcAddress;
    sdkSettings.window   = displayWindow;
    firefly::SDK::GetInstance().Initialise(sdkSettings);
    firefly::SDK::GetInstance().OnSize(SCREEN_WIDTH, SCREEN_HEIGHT);

    for(auto& scene : scenes)
    {
#if FIREFLY_PLATFORM == PLATFORM_LINUX
        auto benchmark = benchmark::RegisterBenchmark(scene.c_str(),
                                     BenchmarkScene,
                                     firefly::GetFireflyDir() + "../bin/benchmarks/" + scene);
        benchmark->Iterations(200);
        benchmark->Unit(benchmark::kMillisecond);
#endif
    }
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    firefly::SDK::GetInstance().Destroy();
	return 0;
}

```
This effectively creates a `firefly_benchmarks` executable (firefly being my internal side project's name). I then invoke this (on linux) as follows as part of my build:

```
#!/bin/bash

if [ ! -d benchmarking ]; then
    mkdir benchmarking
fi

cd benchmarking

if [ ! -d benchmark_monitor ]; then
    git clone https://github.com/bensanmorris/benchmark_monitor.git
    cd benchmark_monitor

    python3 -m venv env
    env/Scripts/activate
    pip3 install -r requirements.txt    

    cd ..
fi

counter=1
while [ $counter -le 30 ]
do
    echo $counter
    ./../bin/firefly_benchmarks --benchmark_out=benchmark_$counter.json && python3 ./benchmark_monitor/benchmark_monitor.py -d . -w 6 -a 0.01
    ((counter++))
done

rm -rf benchmark_monitor
rm benchmark_*.zip
timestamp=$(date +%s)
zip -r benchmark_$timestamp.zip .
cp ./benchmark_$timestamp.zip ../../benchmarks
```

It's not very sophisticated but the end result is a sequence of charts for each benchmarked scene that I then compare to the previous release. I have managed to not release a few performance regressions as a result of this so hope it's of use to you.

# Performance related YouTube channels and videos

- [What's a Creel](https://www.youtube.com/user/WhatsACreel) - great channel on intrinsics and assembler

# References

- [Brendan Gregg's memory flamegraphs](https://www.brendangregg.com/FlameGraphs/memoryflamegraphs.html)
- [Brendan Gregg's differential Flamegraphs](http://www.brendangregg.com/blog/2014-11-09/differential-flame-graphs.html)
- [Linux perf examples](https://www.brendangregg.com/perf.html)
- [Perf mem reference](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/monitoring_and_managing_system_status_and_performance/profiling-memory-accesses-with-perf-mem_monitoring-and-managing-system-status-and-performance)
- Agner Fogg's C++ optimisation manual: https://www.agner.org/optimize/optimizing_cpp.pdf
- [Profiling OpenCV Applications](https://github.com/opencv/opencv/wiki/Profiling-OpenCV-Applications)
- [Danila Kutenin's Making the Most Out of Your Compiler CppCon talk](https://youtu.be/tckHl8M3VXM)
