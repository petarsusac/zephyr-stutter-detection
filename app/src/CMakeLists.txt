target_sources(app PRIVATE main.c)

add_subdirectory(microphone)
add_subdirectory(tflite)
add_subdirectory(feature_extraction)
