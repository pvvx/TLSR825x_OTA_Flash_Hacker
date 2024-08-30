# TLSR825x_OTA_Flash_Hacker
Read firmware via BLE, restore and patch the original firmware, ... 

#### Usage 

Read Full Flash and restore old firmware:

1. Using [TelinkMiFlasher](https://pvvx.github.io/ATC_MiThermometer/TelinkMiFlasher.html) or [TelinkOTA](https://pvvx.github.io/ATC_MiThermometer/TelinkOTA.html) download '[BLE-Hacker.bin](https://github.com/pvvx/TLSR825x_OTA_Flash_Hacker/raw/main/BLE-Hacker.bin)'
2. Using [TLSR825xOTA_Hacker](https://pvvx.github.io/CGPR1/TLSR825xOTA_Hacker.html) connect to the appeared device "BLE_xxxxxx"
3. Read the entire Flash into a file: "Read Full Flash" and "Save to File"
4. Restore original firmware boot: key "Change Start Boot"
5. Close "TLSR825xOTA_Hacker"

![png](https://github.com/pvvx/TLSR825x_OTA_Flash_Hacker/blob/main/work.png)

#### Building the firmware

1. Go to [wiki.telink-semi.cn](http://wiki.telink-semi.cn/wiki/IDE-and-Tools/IDE-for-TLSR8-Chips/) and get the IDE for TLSR8 Chips.
2. Clone https://github.com/Ai-Thinker-Open/Telink_825X_SDK
3. Install the IDE and import the project
4. Change 'Linked resource' and 'C/C++ Build/Build command' 
5. Compile the project

