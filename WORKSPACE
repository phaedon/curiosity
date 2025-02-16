load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "implot-0.16",
    build_file_content = """
cc_library(
    name = "implot-0.16",
    srcs = ["implot.cpp", "implot_items.cpp", "implot_demo.cpp"],
    hdrs = ["implot.h", "implot_internal.h"],
    deps = [
        "@imgui",
    ],
    #linkopts = ["-lGL"], # Required for linux
    visibility = ["//visibility:public"], 
)
""",
    strip_prefix = "implot-0.16",
    urls = ["https://github.com/epezent/implot/archive/refs/tags/v0.16.tar.gz"],
)
