# WoChat的数据库设计

Wochat的客户端采用sqlite数据库作为基本的数据存储技术，同时借鉴git的设计思想，对于体积超过一定大小的消息包，譬如图片、视频文件等，存储在.wochat/objects目录中。 本文档论述了sqlite数据库的表结构设计。

## 私钥表keys
```sql
CREATE TABLE keys
(
	id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 序列号，1,2,3,...
	dt TIME NOT NULL,                      -- 私钥被创建的时间
	sk CHAR(64) NOT NULL UNIQUE,           -- 被AES256加密过的密钥
	pk CHAR(66) NOT NULL UNIQUE,           -- sk对应的公钥
	name VARCHAR(128) NOT NULL UNIQUE,     -- UTF8编码的用户名称
	active CHAR(1)                         -- 该私钥是否已经被禁止，处于非活跃状态： Y = active
);
```

当用户登录WoChat时，需要在一个下拉别表框中选择用户名N，然后输入口令P。N就是name这一列的值。P包含所有合法的ASCII字符，长度不限。P经过SHA256哈希后得到的32个字节用于对sk进行解密。加解密的算法是AES256。对sk解密后，由sk可以计算出pk0。 然后拿pk0和pk这一列进行对比，就可以知道sk是否是被成功地解密了。

任何时刻，用户只能使用一个sk进行登录。当然用户可以开启多个WoChat进程，每一个进程有自己的sk。这样用户在一台机器上就可以使用多个账号同时聊天了。对于群聊，群内的每一个成员都拥有这个群的sk。当一个群成员被踢出该群后，这个群必须新产生一个sk。群聊的问题后面再研究。


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
