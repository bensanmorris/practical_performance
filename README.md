# TLDR

A short practical guide on code performance analysis.

# CPU profiling

## Flamegraphs

Flamegraphs (created by Brendan Gregg) are very useful in helping you quickly identify code hotspots. 

1. Launch your linux docker container with SYS_ADMIN capabilities: `docker run --privileged --cap-add SYS_ADMIN -t [your_image_id] /bin/bash` . From this point forward we are inside our launched container
2. `sudo sysctl -w kernel.kptr_restrict=0` and `sudo sysctl -w kernel.perf_event_paranoid=1`
3. clone Brendan Gregg's Flamegraph repo: `git clone https://github.com/brendangregg/Flamegraph.git`
4. clone and build your code (with symbols)
5. run your code / unit tests under perf. As I use gtest, typically I list my  tests and choose one `[my_gtest_app] --gtest_list_tests`
6. next, let's run our gtest of interest under perf: `perf record -a -g ./my_gtest_app --gtest_filter=MyTestSuite.MyTest`
7. run perf script to generate trace output from perf data: `perf script > out.perf`
8. next, we'll normalise this data and fold all the call-graphs: `../../Flamegraph/stackcollapse-perf.pl out.perf > out.folded`
9. now let's generate the flamegraph: `../../Flamegraph/flamegraph.pl out.folded > out.svg`
10. you should now have an out.svg file in the current directory
11. let's copy the out.svg from our container to our host. In a separate shell (i.e. outside our container), obtain your running container's name: `docker ps` then: `docker cp your_container_name:/your/out.svg/location/out.svg .`
12. open it in an svg viewer (a web browser for instance)

## Other tools

- [Intel VTune Profiler](https://software.intel.com/content/www/us/en/develop/documentation/vtune-help/top.html) - Good for getting an overview of your app's utilisation of CPU across your app's threads (red = spinning, green = good utilisation).

# Memory profiling

## valgrind + massif

1. `valgrind --tool=massif --xtree-memory=full ./your_gtest_app --gtest_filter=your_test_suite.your_test`
2. once this completes you can view the output (massif.out.xxx) in the massif-visualizer and also view the xtmemory.kcg.xxx file in kcachegrind

## valgrind + memcheck

1. `valgrind --tool=memcheck --xml=yes --xml-file=./output_file.xml --leak-check=full ./your_gtest_app --gtest_filter=your_test_suite.your_test`
2. for a quick summary let's grab the python ValgrindCI tool: `python -m pip install ValgrindCI --user`
3. for a summary: `valgrind-ci ./output_file.xml --summary` or to use it as part of CI and abort on errors: `valgrind-ci ./output_file.xml --abort-on-errors`

## A small memory profiling utility

I've created a small [memory monitor](memory_monitor.h) that you can drop into your google tests as follows:

```
MEMORY_MONITOR_BEGIN
    // your test code
MEMORY_MONITOR_END
ASSERT_LT(meminfo.process_pmem, 1024 * 1024 * 1024 * 1.5 /* eg assert peak physical memory consumption during test was < 1.5GB */);

```

# GPU profiling

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

# OpenCV and multi-threading

- you can influence the number of internal threads used by OpenCV via the `cv::setNumThreads()`
- 

# Android

- [Roy Longbottom's extensive benchmarking of Neon](http://www.roylongbottom.org.uk/android%20neon%20benchmarks.htm)

# Performance related YouTube channels and videos

- [What's a Creel](https://www.youtube.com/user/WhatsACreel) - great channel on intrinsics and assembler

# Links

- Agner Fog's CPU optimisation manual: https://www.agner.org/optimize/optimizing_cpp.pdf
- [Profiling OpenCV Applications](https://github.com/opencv/opencv/wiki/Profiling-OpenCV-Applications)
