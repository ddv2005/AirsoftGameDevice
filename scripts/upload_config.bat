mkspiffs.exe -c .\config -a -s 131072 config.img
esptool.exe -b 921600 write_flash 0x370000 config.img