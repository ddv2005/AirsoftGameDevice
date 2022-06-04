esptool.exe -b 921600 read_flash 0x370000 131072 config.img
mkspiffs.exe -u config config.img 