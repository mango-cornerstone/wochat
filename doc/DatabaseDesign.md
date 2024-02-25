# WoChat的数据库设计

Wochat的客户端采用sqlite数据库作为基本的数据存储技术，同时借鉴git的设计思想，对于体积超过一定大小的消息包，譬如图片、视频文件等，存储在.wochat/objects目录中。 本文档论述了sqlite数据库的表结构设计。

## 私钥表k
```sql
CREATE TABLE k
(
	id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 序列号，从1开始，1,2,3,...
	p INTEGER,  -- 父节点，如果是0，表示原始私钥。如果是1，这表示这把私钥是1号私钥加密的
	a CHAR(1),  -- 该私钥是否已经被禁止，处于非活跃状态： Y = active
	dt INTEGER, -- 8字节的时间
	sk CHAR(64) NOT NULL UNIQUE, -- 被AES256加密过的密钥。 p=0的私钥由口令加密，p为非0的，有对应的私钥加密
	pk CHAR(66) NOT NULL UNIQUE, -- sk对应的公钥
	nm VARCHAR(128) NOT NULL UNIQUE, -- UTF8编码的用户名称
	i32 BLOB,  -- 32X32的头像
	i128 BLOB  -- 128X128的大头像
);
```

当用户登录WoChat时，需要在一个下拉别表框中选择用户名N，然后输入口令P。N就是name这一列的值。P包含所有合法的ASCII字符，长度不限。P经过SHA256哈希后得到的32个字节用于对sk进行解密。加解密的算法是AES256。对sk解密后，由sk可以计算出pk0。 然后拿pk0和pk这一列进行对比，就可以知道sk是否是被成功地解密了。

任何时刻，用户只能使用一个sk进行登录。当然用户可以开启多个WoChat进程，每一个进程有自己的sk。这样用户在一台机器上就可以使用多个账号同时聊天了。对于群聊，群内的每一个成员都拥有这个群的sk。当一个群成员被踢出该群后，这个群必须新产生一个sk。群聊的问题后面再研究。


## 发送消息表s
```sql
CREATE TABLE s
(
	id INTEGER PRIMARY KEY AUTOINCREMENT, -- 序列号，从1开始，1,2,3,...
	dt INTEGER, -- 8字节的时间
	st CHAR(1), -- 是否发送成功 Y - 成功
	h CHAR(64), -- 原始消息的SHA256哈希值
	t TEXT -- UF8编码的原始消息包，是base64编码过的，所以都是可打印字符
);
```

## 接收消息表r
```sql
CREATE TABLE r
(
	id INTEGER PRIMARY KEY AUTOINCREMENT, -- 序列号，从1开始，1,2,3,...
	dt INTEGER, -- 8字节的时间
	h CHAR(64), -- 原始消息的SHA256哈希值
	t TEXT -- UF8编码的原始消息包，是base64编码过的，所以都是可打印字符
);
```

## 无法解密的接收消息表q
```sql
CREATE TABLE q
(
	id INTEGER PRIMARY KEY AUTOINCREMENT, -- 序列号，从1开始，1,2,3,...
	dt INTEGER, -- 8字节的时间
	t TEXT -- UF8编码的原始消息包，是base64编码过的，所以都是可打印字符
);
```

## 联系人表f
```sql
CREATE TABLE f
(
	id INTEGER PRIMARY KEY AUTOINCREMENT, -- 序列号，从1开始，1,2,3,...
	dt INTEGER,  -- 8字节的时间
	a CHAR(1),   -- 该公钥钥是否已经被拉黑，处于非活跃状态： Y = active
	pk CHAR(64), -- 该用户的公钥
	nm VARCHAR(128),  -- UTF8编码的用户名称
	b INTEGER, -- 生日
	x CHAR(1), -- 性别
	i32 BLOB, -- 32X32的头像
	i128 BLOB -- 128X128的大头像
);
```
