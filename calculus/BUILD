load('/aos/externals/seasocks/gen_embedded', 'gen_embedded')
load('/aos/downloader/downloader', 'aos_downloader_dir')

gen_embedded(
  name = 'gen_embedded',
  srcs = glob(['www_defaults/**/*'], ['www/**/*']),
)

aos_downloader_dir(
  name = 'www_files',
  visibility = ['//visibility:public'],
  srcs = glob([
    'www/**/*',
  ]),
  dir = "www",
)

cc_binary(
  name = 'server',
  visibility = ['//visibility:public'],
  srcs = [
    'server.cc',
    'server.h',
  ],
  deps = [
    ':gen_embedded',
    '//aos/linux_code:init',
    '//aos/common/logging',
    '//aos/common/util:phased_loop',
    '//aos/common:time',
    '//third_party/seasocks',
  ],
)
