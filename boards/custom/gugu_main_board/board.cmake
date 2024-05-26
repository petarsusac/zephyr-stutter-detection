board_runner_args(jlink "--device=STM32F746VG" "--speed=4000" "--reset")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
