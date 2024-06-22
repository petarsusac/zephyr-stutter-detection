year=$(date +"%Y")
month=$(date +"%m")
day_of_month=$(date +"%d")
day_of_week=$(date +"%u")
hour=$(date +"%H")
minutes=$(date +"%M")
seconds=$(date +"%S")

west build app -b gugu_main_board -DRTC_SET_TIME=1 -DRTC_YEAR=$year -DRTC_MON=$month -DRTC_MDAY=$day_of_month -DRTC_WDAY=$day_of_week -DRTC_HOUR=$hour -DRTC_MIN=$minutes -DRTC_SEC=$seconds