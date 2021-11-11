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

NB. The beauty of `memleak-bpfcc` is it shows you memory that was allocated but hasn't been reclaimed which may not be a leak as such. For instance, like me you may be filling an `std::vector` with values (which results in allocations) and then you clear it with `clear()` (which doesn't reclaim the memory) as you intend to re-use it later in your app. This may be problematic as you have allocated memory (in your long living `vector`) but are not reclaiming it but isn't a hard and fast rule, you'll need to judge for yourself. For me it was a problem so instead I replaced my periodic `clear()` with swapping an empty vector's contents into my long life vector to force it to reclaim which resulted in quite a large memory saving. This is mentioned in `std::vector::clear()` docs.

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

# Performance related YouTube channels and videos

- [What's a Creel](https://www.youtube.com/user/WhatsACreel) - great channel on intrinsics and assembler

# References

- [Brendan Gregg's memory flamegraphs](https://www.brendangregg.com/FlameGraphs/memoryflamegraphs.html)
- [Brendan Gregg's differential Flamegraphs](http://www.brendangregg.com/blog/2014-11-09/differential-flame-graphs.html)
- [Linux perf examples](https://www.brendangregg.com/perf.html)
- [Perf mem reference](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/monitoring_and_managing_system_status_and_performance/profiling-memory-accesses-with-perf-mem_monitoring-and-managing-system-status-and-performance)
- Agner Fogg's C++ optimisation manual: https://www.agner.org/optimize/optimizing_cpp.pdf
- [Profiling OpenCV Applications](https://github.com/opencv/opencv/wiki/Profiling-OpenCV-Applications)
