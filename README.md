## 钉钉WIFI打卡
#### 基于ESP32-C3芯片，通过修改AP模式WIFI MAC地址，实现钉钉WIFI打卡 。
使用方法(PlatformIO)
1. 修改 platformio.ini 配置文件第13行，board = 你的开发板型号
2. 编译并上传固件。
3. 在PlatformIO拓展中找到并点击 Upload Filesystem image 上传数据文件。
4. 连接默认热点名称 WIFI MANAGER ，手机会自动弹出配置页面(192.168.4.1)，填写需要模拟的WIFI信息

使用ESP32C3 Xiao开发板测试，修改了WIFI发射功率降低发热，使用LED指示热点状态，长按左边按键清除保存的WIFI信息
![image](images/ESP32C3%20Xiao.jpg)
![image](images/ESP32%20WIFI%20MANAGER.jpg)