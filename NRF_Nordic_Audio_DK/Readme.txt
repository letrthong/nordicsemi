
install 
 https://docs.zephyrproject.org/3.2.0/develop/getting_started/index.html
 
 https://cmake.org/download/

https://www.nordicsemi.com/Products/Development-hardware/nRF5340-Audio-DK?utm_source=google%20&utm_medium=cpc&utm_campaign=nrf5340%20audio%20dk&utm_term=audio%20development%20kit&utm_campaign=Product+%7C+nRF5340+Audio+DK+%7C+Global&utm_source=adwords&utm_medium=ppc&hsa_tgt=kwd-56312428618&hsa_grp=138063455513&hsa_src=g&hsa_net=adwords&hsa_mt=b&hsa_ver=3&hsa_ad=597567008334&hsa_acc=1116845495&hsa_kw=audio%20development%20kit&hsa_cam=17238270673&gclid=Cj0KCQiA-oqdBhDfARIsAO0TrGEeIte_jPqZ20Z4UQ3WYoAoIIKpyimRlOxfdHCl9klhn31s8IeMzKQaAtUbEALw_wcB


https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/applications/nrf5340_audio/README.html


nrf5340_audio

Build target
	nrf5340_audio_dk_nrf5340_cpuapp
	
	
	
nRF5340 Audio DK


Audio 
	CS47L63
	
https://community.element14.com/products/roadtest/rt/roadtests/586/bluetooth_le_audio_d#pifragment-4106=9&pifragment-4100=4
https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/applications/nrf5340_audio/README.html
https://webinars.nordicsemi.com/future-proofing-iot-development-5

C:\Users\HP\zephyrproject\zephyr
   west build -p -b nrf5340_audio_dk_nrf5340_cpuapp samples/hello_world
   nrfjprog --program build/zephyr/zephyr.hex --coprocessor CP_APPLICATION --chiperase -r
   
   west build -p always -b nrf5340_audio_dk_nrf5340_cpuapp samples\basic\blinky
   
   west flash
   
   
   https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/releases/release-notes-2.1.1.html'
   
   
   https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/gs_installing.html
