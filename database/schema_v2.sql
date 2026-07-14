-- Schema V2: 基于出行日期的 Trip 数据模型

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

CREATE TABLE IF NOT EXISTS Train (
    trainId INTEGER PRIMARY KEY AUTOINCREMENT,
    trainNumber TEXT NOT NULL UNIQUE,
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
    UNIQUE(trainId, travelDate)
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

CREATE TABLE IF NOT EXISTS OperationLog (
    logId INTEGER PRIMARY KEY AUTOINCREMENT,
    operatorUsername TEXT NOT NULL,
    action TEXT NOT NULL,
    detail TEXT NOT NULL,
    createdAt TEXT NOT NULL
);
