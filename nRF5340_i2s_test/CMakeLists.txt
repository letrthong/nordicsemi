#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Relies on project structure being
# Firmware
# |___ Applications
#       |__ <This project dir>
# |___ Third_Party
# |___ boards
#       |___ arm
#		|___ <some board>
#		|___ <another board>
# |___ Scripts
set(BOARD nrf5340_hhr_cpuapp)
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../)

# Set up special config for build type
set(OVERLAY_CONFIG "app_${PROJ_BUILD_TYPE}.conf" CACHE INTERNAL "Overlay configs")

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(i2s_test)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END
