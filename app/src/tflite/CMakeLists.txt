# These samples use local static initialization. Since Zephyr doesn't support the
# C++ ABI for thread-safe initialization of local statics and the constructors don't
# appear to require thread safety, we turn it off in the C++ compiler.
set(NO_THREADSAFE_STATICS $<TARGET_PROPERTY:compiler-cpp,no_threadsafe_statics>)
zephyr_compile_options($<$<COMPILE_LANGUAGE:CXX>:${NO_THREADSAFE_STATICS}>)

target_include_directories(app PRIVATE inc)

target_sources(app PRIVATE 
    src/model_block.cpp
    src/model_repetition.cpp
    src/model_prolongation.cpp
    src/inference.cpp
)
