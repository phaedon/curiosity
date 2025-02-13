load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "imgui-1.91.8",
    build_file_content = """
cc_library(
    name = "imgui-1.91.8",
    srcs = [
        "backends/imgui_impl_glfw.cpp",
        "backends/imgui_impl_opengl3.cpp",
        "imgui.cpp",
        "imgui_demo.cpp",
        "imgui_draw.cpp",
        "imgui_tables.cpp",
        "imgui_widgets.cpp",
    ],
    hdrs = [
        "backends/imgui_impl_glfw.h",
        "backends/imgui_impl_opengl3.h",
        "backends/imgui_impl_opengl3_loader.h",
        "imconfig.h",
        "imgui.h",
        "imgui_internal.h",
        "imstb_rectpack.h",
        "imstb_textedit.h",
        "imstb_truetype.h",
    ],
    include_prefix = "imgui",
    includes = ["."],
    linkopts = ["-ldl"],
    visibility = ["//visibility:public"],
    deps = [
        "@glfw",
    ],
)
""",
    strip_prefix = "imgui-1.91.8",
    urls = ["https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8.tar.gz"],
)

http_archive(
    name = "implot-0.16",
    build_file_content = """
cc_library(
    name = "implot-0.16",
    srcs = ["implot.cpp", "implot_items.cpp", "implot_demo.cpp"],
    hdrs = ["implot.h", "implot_internal.h"],
    deps = [
        "@imgui-1.91.8",
    ],
    visibility = ["//visibility:public"], 
)
""",
    strip_prefix = "implot-0.16",
    urls = ["https://github.com/epezent/implot/archive/refs/tags/v0.16.tar.gz"],
)
