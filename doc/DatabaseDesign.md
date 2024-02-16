# WoChat的数据库设计

Wochat的客户端采用sqlite数据库作为基本的数据存储技术，同时借鉴git的设计思想，对于体积超过一定大小的消息包，譬如图片、视频文件等，存储在.wochat/objects目录中。 本文档论述了sqlite数据库的表结构设计。

## 私钥表privatekey
```sql
CREATE TABLE privatekey
(
	id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 序列号，1,2,3,...
	dt TIME NOT NULL,                      -- 私钥被创建的时间
	sk CHAR(64) NOT NULL UNIQUE,           -- 被AES256加密过的密钥
	name VARCHAR(128) NOT NULL UNIQUE,     -- UTF8编码的用户名称
	active CHAR(1)                         -- 该私钥是否已经被禁止，处于非活跃状态： Y = active
);
```

## 消息表message
```sql
CREATE TABLE message
(
	id BIGINT PRIMARY KEY AUTOINCREMENT,   -- 序列号，1,2,3,...
	dt TIME NOT NULL,                      -- 消息被存储的时间
	topic CHAR(66) NOT NULL                -- 该消息接受者的公钥，也是MQTT协议的topic
	pk CHAR(66) NOT NULL                   -- 该消息发送者的公钥
	hash CHAR(64) NOT NULL UNIQUE,         -- 不加密的消息的SHA256的值
	size INT NOT NULL,                     -- 消息包的体积，单位是字节
	type CHAR(1) NOT NULL,                 -- 消息包的数据类型： T - 文本, G - Gif图片，M - MP4视频文件 等等，另有文档规定
	stat CHAR(1) NOT NULL,                 -- 状态: N - 没有被发送, S - 已经发送, C - 已经收到对方的确认
	val  BLOB NOT NULL                     -- 消息的本体，加密形式保存
);
```

## 联系人表friend
```sql
CREATE TABLE friend
(
	id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 序列号，1,2,3,...
	dt TIME NOT NULL,                      -- 该记录被创建的时间
	pk CHAR(66) NOT NULL UNIQUE            -- 该联系人的公钥
	name VARCHAR(128) NOT NULL             -- 联系人的姓名
	birth TIME,                            -- 生日
	sex  CHAR(1),                          -- 性别
	stat CHAR(1) NOT NULL                  -- 状态，是否拉黑等
);
```

## 聊天表talk
```sql
CREATE TABLE talk
(
	id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 序列号，1,2,3,...
	pk CHAR(66) NOT NULL UNIQUE            -- 该联系人的公钥
);
```
