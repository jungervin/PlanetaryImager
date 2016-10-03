file(GLOB image_handlers_SRCS *.cpp)
if(Enable_Autoguider)
    file(GLOB autoguider_SRCS guider/*.cpp)
    message("Enabling autoguider sources: ${autoguider_SRCS}")
endif(Enable_Autoguider)

add_library(image_handlers STATIC ${image_handlers_SRCS} ${autoguider_SRCS})
add_imager_dependencies(image_handlers)
add_subdirectory(output_writers)
