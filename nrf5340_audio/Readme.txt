west build -b nrf5340_audio_dk_nrf5340_cpuapp --pristine -- -DCONFIG_AUDIO_DEV=1 -DCONF_FILE=prj_release.conf
west build -b nrf5340_audio_dk_nrf5340_cpuapp --pristine -- -DCONFIG_AUDIO_DEV=1 -DCONF_FILE=prj.conf


nrfjprog --program build/zephyr/zephyr.hex --coprocessor CP_APPLICATION --chiperase -r