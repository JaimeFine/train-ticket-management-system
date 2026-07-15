-- Schema V2: 基于出行日期的 Trip 数据模型

-- 用户表：登录账号与角色
CREATE TABLE IF NOT EXISTS User (
    userId INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,        -- 登录名，全局唯一
    password TEXT NOT NULL,
    role INTEGER NOT NULL,                -- 角色：0=Guest 1=Seller 2=Admin
    enabled INTEGER NOT NULL DEFAULT 1    -- 1=启用 0=禁用（软删除）
);

-- 车站表：站名字典
CREATE TABLE IF NOT EXISTS Station (
    stationId INTEGER PRIMARY KEY AUTOINCREMENT,
    stationName TEXT NOT NULL UNIQUE      -- 站名唯一
);

CREATE TABLE IF NOT EXISTS Train (
    trainId INTEGER PRIMARY KEY AUTOINCREMENT,
    trainNumber TEXT NOT NULL UNIQUE,     -- 车次号，全局唯一
    departureStationId INTEGER NOT NULL,
    arrivalStationId INTEGER NOT NULL,
    departureTime TEXT NOT NULL,
    arrivalTime TEXT NOT NULL,
    totalSeats INTEGER NOT NULL CHECK(totalSeats >= 0),
    enabled INTEGER NOT NULL DEFAULT 1,
    FOREIGN KEY (departureStationId) REFERENCES Station(stationId),
    FOREIGN KEY (arrivalStationId) REFERENCES Station(stationId)
);

CREATE TABLE IF NOT EXISTS Trip (
    tripId INTEGER PRIMARY KEY AUTOINCREMENT,
    trainId INTEGER NOT NULL,
    travelDate TEXT NOT NULL,
    departureTime TEXT NOT NULL,
    arrivalTime TEXT NOT NULL,
    totalSeats INTEGER NOT NULL CHECK(totalSeats >= 0),
    remainingSeats INTEGER NOT NULL CHECK(remainingSeats >= 0),
    basePrice REAL NOT NULL DEFAULT 0,
    enabled INTEGER NOT NULL DEFAULT 1,
    FOREIGN KEY (trainId) REFERENCES Train(trainId),
    UNIQUE(trainId, travelDate)           -- 同一车次每天最多一个班次
);

CREATE TABLE IF NOT EXISTS "Order" (
    orderId INTEGER PRIMARY KEY AUTOINCREMENT,
    userId INTEGER NOT NULL,
    tripId INTEGER NOT NULL,
    passengerName TEXT NOT NULL,
    travelDate TEXT NOT NULL,
    purchaseTime TEXT NOT NULL,
    price REAL NOT NULL DEFAULT 0,
    status INTEGER NOT NULL,
    FOREIGN KEY (userId) REFERENCES User(userId),
    FOREIGN KEY (tripId) REFERENCES Trip(tripId)
);

-- 操作日志表：记录管理/售票端关键操作，用于审计
CREATE TABLE IF NOT EXISTS OperationLog (
    logId INTEGER PRIMARY KEY AUTOINCREMENT,
    operatorUsername TEXT NOT NULL,       -- 操作人用户名
    action TEXT NOT NULL,                 -- 操作类型
    detail TEXT NOT NULL,                 -- 操作详情
    createdAt TEXT NOT NULL               -- 记录时间
);
