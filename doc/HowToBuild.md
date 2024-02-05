# WoChat源码编译指南

限于开发资源有限，很长时间内WoChat的开发都集中在Windows平台。所以本文档介绍如何在Windows下编译源码。

在编译之前，你需要安装Visual Studio 2022社区版（VSTS 2022）。这是免费的，只要从微软官方网站下载即可。安装的时候选择要安装DeskTop的开发选项，因为WoChat是用C++开发的百分百原生的应用。
你还需要安装CMake最新版。这个也非常容易。

在安装好VSTS 2022以后，在Windows主菜单上有"x64 Native Tools Command Promote for VSTS 2022"的选项，点击它就会打开一个黑乎乎的DOS窗口，只不过里面的Visual C++的各种环境变量都设置好了。你在这个窗口里面敲入cl，如果显示一大堆信息，说明编译器cl.exe是可以执行的。我们就具备了基本的编译条件。 假设你的WoChat源码目录在D:\mywork\wochat，你编译后的东西在c:\Build\wt下。

请在上面DOS窗口中执行如下命令：
```
C:\build>cmake -B wt -G "NMake Makefiles" d:\mywork\wochat
```
你会发现CMake帮你创建了一个目录c:\build\wt，切换到这个目录下，执行如下命令：
```
C:\build\wt>nmake
```
等了一会，你就会在C:\build\wt\win32目录下会看到一个可执行文件wochat-win64.exe。这个就是最终的编译成果。运行它即可。

WoChat把所需要的第三方库的源码都包含进来了，所以WoChat的源码树是自给自足的，不再需要从别的地方下载源码了。这大大方便了你的编译体验。

WoChat目前依赖的库列表如下：
- sqlite ： 著名的客户端SQL数据库
- secp256k1 : 来自比特币内核的椭圆曲线加密算法。这个是WoChat的基石
- mbedtls ： ARM公司的加密库，提供SHA256/AES/BASE64等加密解密和编码功能。
- mosquitto ： MQTT协议的客户端网络库，用于真正的聊天消息发送。

以上这些库的源码都在WoChat代码目录下可以拿到。在这个目录下的win32是主程序的核心代码。

有任何编译问题，发送消息给wochatdb@gmail.com

