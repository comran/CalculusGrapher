package(default_visibility = ['//visibility:public'])

cc_library(
  name = 'empty_main',
  srcs = [ 'empty_main.c' ],
)

cc_library(
  name = 'malloc',
  deps = select({
    '//tools:has_asan': [],
    '//tools:has_tsan': [],
    '//conditions:default': ['//third_party/gperftools:tcmalloc'],
  }),
)

cc_library(
  name = 'stl',
)

filegroup(
  name = 'empty',
  srcs = [],
)

# This is the entry point for --crosstool_top.  Toolchains are found
# by lopping off the name of --crosstool_top and searching for
# 'cc-compiler-${CPU}' in this BUILD file, where CPU is the target CPU
# specified in --cpu.
#
# This file group should include
#   * all cc_toolchain targets supported
#   * all file groups that said cc_toolchain might refer to,
# including the default_grte_top setting in the CROSSTOOL
# protobuf.
filegroup(
  name = 'toolchain',
  srcs = [
    ':cc-compiler-k8',
    ':flags_compiler_inputs',
  ],
)

# Compiler inputs given by --copt etc in //tools:bazel.rc.
filegroup(
  name = 'flags_compiler_inputs',
  srcs = select({
    '//tools:has_asan': [
      'asan-blacklist',
    ],
    '//tools:has_ubsan': [
      'ubsan-blacklist',
    ],
    '//conditions:default': [],
  }),
)

cc_toolchain(
  name = 'cc-compiler-k8',
  all_files = ':flags_compiler_inputs',
  compiler_files = ':flags_compiler_inputs',
  cpu = 'local',
  dwp_files = ':empty',
  dynamic_runtime_libs = [':empty'],
  linker_files = ':empty',
  objcopy_files = ':empty',
  static_runtime_libs = [':empty'],
  strip_files = ':empty',
  supports_param_files = 1,
)
