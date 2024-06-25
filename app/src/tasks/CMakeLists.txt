target_include_directories(app PRIVATE inc)

target_sources(app PRIVATE 
    src/audio_proc.c
    src/audio_acq.c
    src/bt_ncp.c
)

if(RTC_SET_TIME)
    target_compile_definitions(app PRIVATE
        RTC_SET_TIME=${RTC_SET_TIME}
        RTC_YEAR=${RTC_YEAR}
        RTC_MON=${RTC_MON}
        RTC_MDAY=${RTC_MDAY}
        RTC_WDAY=${RTC_WDAY}
        RTC_HOUR=${RTC_HOUR}
        RTC_MIN=${RTC_MIN}
        RTC_SEC=${RTC_SEC})
endif(RTC_SET_TIME)
