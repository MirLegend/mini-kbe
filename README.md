mini_kbe
=============
The essence of KBEngine \ 轻量级的kbengine服务器实现
-------------

##### 保留了kbengine中最核心的 network、server库， 
      保持了服务器最核心组件拓扑结构(login、logger、basemgr、cellmgr、dbmgr、base、cell) basemgr、dbmgr、cellmgr唯一，
      login、base、cell可以多个负载。

##### 简化了resmgr功能
      resmgr不使用环境变量方式读取资源路径，改用直接本地配置。

##### 去掉了kbe中复杂的machine机制
      各个组件的连接使用配置简化，例如 base 要连接basemgr，不使用machine udp广播发包，
      直接读取本地配置，找到依赖组件的ip 和端口。

##### 暂时去掉了python脚本功能支持

##### 暂时去掉了数据库功能
      后续会添加mysql 和 redis的 搭配。 mysql做永久存储、redis做缓存的多级数据库方案。

##### 暂时去掉了性能统计模块。
      轻量原则，暂时去掉性能统计这块。kbe性能监视模块比较强大，后续会添加。

##### 去掉了kbengine中rpc机制， 
      网络通信数据传输方式 将2进制方式 改为传统的protobuf来传输。
      服务器使用原生protobuf c++， 客户端使用云风的lua_pbc。
      
##### 简化并优化了网络协议宏， 
      保留简化了网络消息解析 handle 宏， 保持原有的简杰、封装的特性，详见各个服务器组件消息定义文件 ***_interface.h 

## kbe 引擎最核心类结构：

      最核心的eventdispatch 主要复杂三大类功能： 定时器、任务task、网络， 服务器游戏逻辑主要在 定时器里面实现。
![kbe core class](https://github.com/MirLegend/mini-kbe/blob/master/doc/class.png)

## kbe 引擎组件结构：
![kbe compents](https://github.com/MirLegend/mini-kbe/blob/master/doc/compents.png)
