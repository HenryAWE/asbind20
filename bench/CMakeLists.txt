add_executable(bench_fn bench_fn.cpp)
target_link_libraries(bench_fn PRIVATE asbind20)
target_link_libraries(bench_fn PRIVATE benchmark::benchmark)
