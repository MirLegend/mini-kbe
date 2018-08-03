mini_kbe
=============
The essence of KBEngine \ 轻量级的kbengine服务器实现
-------------

##### 保留了kbengine中最核心的 network、server库， 
      保持了服务器最核心组件拓扑结构(login、logger、basemgr、cellmgr、dbmgr、base、cell) basemgr、dbmgr、cellmgr唯一，
      login、base、cell可以多个负载。
	  
##### 自定义了一套简洁的数据库模块
      自定义了一套简洁的数据库模块，不直接建表， 数据库表由配置生成(类似kbengine，只是 没有kbe那样强大，结合了实体定义),同时也去掉了子父表联系,数据表里面的字段有5种属性， type:定义数据类型，继承kbe基本数据类型， dblength:数据长度，0为默认，字符型的需填写长度， flags: 说明是否是唯一，自动增长等等，兼容mysql的flags， indextype: 索引类型，不写表示不设置索引， Default: 数据默认值
      服务器每次启动 都会对比配置，数据改动了会更新到数据库中，配置删除或增加了的，也会同步增删到数据库中。(kbengine里面简直是做得天衣无缝，跟实体定义文件结合非常巧妙与自然。 佩服kbe 作者)

      数据表配置 详见下图：
![kbe dbconfig](https://github.com/MirLegend/mini-kbe/blob/master/doc/dbconfig.png)


##### 简化了resmgr功能
      resmgr不使用环境变量方式读取资源路径，改用直接本地配置。

##### 去掉了kbe中复杂的machine机制
      各个组件的连接使用配置简化，例如 base 要连接basemgr，不使用machine udp广播发包，
      直接读取本地配置，找到依赖组件的ip 和端口。

##### 暂时去掉了python脚本功能支持
      kbe另外一大核心就是 模块def文件，def实体定义文件 分为 解析 和 绑定到脚本 两大部分，这是kbe里面的重点也是难点。mini-kbe去掉了这部分
 

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
