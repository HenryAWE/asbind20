add_library(shared_bench_lib STATIC shared_bench_lib.hpp shared_bench_lib.cpp)
target_link_libraries(shared_bench_lib PUBLIC asbind20)
target_link_libraries(shared_bench_lib PUBLIC benchmark::benchmark)

add_executable(bench_fn bench_fn.cpp)
target_link_libraries(bench_fn PRIVATE shared_bench_lib)
target_link_libraries(bench_fn PRIVATE asbind20_ext_container)

add_executable(bench_io bench_io.cpp)
target_link_libraries(bench_io PRIVATE shared_bench_lib)

add_executable(bench_invoke bench_invoke.cpp)
target_link_libraries(bench_invoke PRIVATE shared_bench_lib)
target_link_libraries(bench_invoke PRIVATE asbind20_ext_container)

