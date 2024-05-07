target_include_directories(app PRIVATE inc)

target_sources(app PRIVATE 
    src/audio_proc.c
    src/audio_acq.c
    src/bt_ncp.c
)