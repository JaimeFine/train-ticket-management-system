-- Schema V2: 基于出行日期的 DailyTrain / Trip 数据模型

CREATE TABLE IF NOT EXISTS User (
    userId INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password TEXT NOT NULL,
    role INTEGER NOT NULL,
    enabled INTEGER NOT NULL DEFAULT 1
);

CREATE TABLE IF NOT EXISTS Station (
    stationId INTEGER PRIMARY KEY AUTOINCREMENT,
    stationName TEXT NOT NULL UNIQUE
);

-- V2: Train 作为车次模板，去掉 remainingSeats
CREATE TABLE IF NOT EXISTS Train (
    trainId INTEGER PRIMARY KEY AUTOINCREMENT,
    trainNumber TEXT NOT NULL UNIQUE,
    departureStationId INTEGER NOT NULL,
    arrivalStationId INTEGER NOT NULL,
    departureTime TEXT NOT NULL,          -- 默认发车时间
    arrivalTime TEXT NOT NULL,            -- 默认到达时间
    totalSeats INTEGER NOT NULL CHECK(totalSeats >= 0),  -- 默认总座位数
    enabled INTEGER NOT NULL DEFAULT 1,
    FOREIGN KEY (departureStationId) REFERENCES Station(stationId),
    FOREIGN KEY (arrivalStationId) REFERENCES Station(stationId)
);

-- V2 新增: 每日班次 = 车次 + 出行日期，可独立设置时间和座位
CREATE TABLE IF NOT EXISTS Trip (
    tripId INTEGER PRIMARY KEY AUTOINCREMENT,
    trainId INTEGER NOT NULL,
    travelDate TEXT NOT NULL,
    departureTime TEXT NOT NULL,          -- 当日发车时间
    arrivalTime TEXT NOT NULL,            -- 当日到达时间
    totalSeats INTEGER NOT NULL CHECK(totalSeats >= 0),
    remainingSeats INTEGER NOT NULL CHECK(remainingSeats >= 0),
    basePrice REAL NOT NULL DEFAULT 0,       -- 当日基础票价
    enabled INTEGER NOT NULL DEFAULT 1,
    FOREIGN KEY (trainId) REFERENCES Train(trainId),
    UNIQUE(trainId, travelDate)
);

-- V2: Order 通过 tripId 关联 Trip，直接存 travelDate 便于查询
CREATE TABLE IF NOT EXISTS "Order" (
    orderId INTEGER PRIMARY KEY AUTOINCREMENT,
    userId INTEGER NOT NULL,
    tripId INTEGER NOT NULL,
    passengerName TEXT NOT NULL,
    travelDate TEXT NOT NULL,             -- 冗余列，避免 JOIN
    purchaseTime TEXT NOT NULL,
    price REAL NOT NULL DEFAULT 0,         -- 实际成交票价
    status INTEGER NOT NULL,              -- 0=已预订 1=已退票 2=已改签 3=已过期
    FOREIGN KEY (userId) REFERENCES User(userId),
    FOREIGN KEY (tripId) REFERENCES Trip(tripId)
);

CREATE TABLE IF NOT EXISTS OperationLog (
    logId INTEGER PRIMARY KEY AUTOINCREMENT,
    operatorUsername TEXT NOT NULL,
    action TEXT NOT NULL,
    detail TEXT NOT NULL,
    createdAt TEXT NOT NULL
);
