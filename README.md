# TLDR

A short practical guide on code performance analysis.

# CPU profiling

## Flamegraphs

Flamegraphs (created by Brendan Gregg) are very useful in helping you quickly identify code hotspots. 

1. Launch your docker container with SYS_ADMIN capabilities: `docker run --privileged --cap-add SYS_ADMIN -t [your_image_id] /bin/bash` . From this point forward we are inside our launched container
2. `sudo sysctl -w kernel.kptr_restrict=0` and `sudo sysctl -w kernel.perf_event_paranoid=1`
3. clone Brendan Gregg's Flamegraph repo: `git clone https://github.com/brendangregg/Flamegraph.git`
4. clone and build your code (with symbols)
5. run your code / unit tests under perf. As I use gtest, typically I list my  tests and choose one `[my_gtest_app] --gtest_list_tests`
6. next, let's run our gtest of interest under perf: `perf record -a -g ./my_gtest_app --gtest_filter=MyTestSuite.MyTest`
7. run perf script to generate trace output from perf data: `perf script > out.perf`
8. next, we'll normalise this data and fold all the call-graphs: `../../Flamegraph/stackcollapse-perf.pl out.perf > out.folded`
9. now let's generate the flamegraph: `../../Flamegraph/flamegraph.pl out.folded > out.svg`
10. you should now have an out.svg file in the current directory
11. let's copy the out.svg from our container to our host, (in a separate shell), obtain your container's id: `docker ps` then: `docker cp your_container_name:/your/out.svg/location/out.svg .`
12. open it in an svg viewer (a web browser for instance)

# Memory profiling

## valgrind + massif

1. `valgrind --tool=massif --xtree-memory=full ./your_gtest_app --gtest_filter=your_test_suite.your_test`
2. once this completes you can view the output (massif.out.xxx) in the massif-visualizer and also view the xtmemory.kcg.xxx file in kcachegrind
