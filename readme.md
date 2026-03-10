# C++聊天web应用

1、创建文件目录，配置开发环境

2、编写CMAkeLists.txt
用简单main.cpp进行build测试成功

3、实现工具类（utils）
json解析库:用于将json文件解析成C++数据结构
config.cpp:实现配置管理，从config.json文件读取配置（数据库地址、端口等）
logger.cpp:实现日志输出功能

4、实现数据库层（database）
DAO(Aata Access Object) ：数据访问对象，将数据库的连接管理、sql语句封装在其内部
暂时实现userDAO

5、实现数据模型（model）
暂时实现user

6、实现网络层（server）
http_server:HTTP服务器，能够注册路由，处理GET/POST网络请求，返回JSON响应
websocket_server:WebSocket服务器，管理连接，接收消息并广播
session:WebSocket会话，代表一个链接，负责收发消息

7、实现业务处理器（handler）

8、编写main.cpp串联所有模块

8、编写前端（web）
前端服务器：用于托管静态文件，本质是一个http服务器，负责接收浏览器请求并返回静态资源，模拟真实环境、避免跨域问题