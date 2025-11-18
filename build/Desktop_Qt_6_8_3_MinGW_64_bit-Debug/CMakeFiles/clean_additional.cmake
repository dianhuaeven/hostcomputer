# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\hostcomputer_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\hostcomputer_autogen.dir\\ParseCache.txt"
  "hostcomputer_autogen"
  "src\\communication\\CMakeFiles\\communication_layer_autogen.dir\\AutogenUsed.txt"
  "src\\communication\\CMakeFiles\\communication_layer_autogen.dir\\ParseCache.txt"
  "src\\communication\\communication_layer_autogen"
  "src\\controller\\CMakeFiles\\controller_layer_autogen.dir\\AutogenUsed.txt"
  "src\\controller\\CMakeFiles\\controller_layer_autogen.dir\\ParseCache.txt"
  "src\\controller\\controller_layer_autogen"
  "src\\model\\CMakeFiles\\model_layer_autogen.dir\\AutogenUsed.txt"
  "src\\model\\CMakeFiles\\model_layer_autogen.dir\\ParseCache.txt"
  "src\\model\\model_layer_autogen"
  "src\\parser\\CMakeFiles\\parser_layer_autogen.dir\\AutogenUsed.txt"
  "src\\parser\\CMakeFiles\\parser_layer_autogen.dir\\ParseCache.txt"
  "src\\parser\\parser_layer_autogen"
  )
endif()
