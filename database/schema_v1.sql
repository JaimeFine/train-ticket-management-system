-- This is SQL reference

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
    remainingSeats INTEGER NOT NULL CHECK(
        remainingSeats >= 0 AND remainingSeats <= totalSeats
    ),
    enabled INTEGER NOT NULL DEFAULT 1,
    FOREIGN KEY (departureStationId) REFERENCES Station(stationId),
    FOREIGN KEY (arrivalStationId) REFERENCES Station(stationId)
);

-- Here "Order" is quoted because ORDER is a keyword
CREATE TABLE IF NOT EXISTS "Order" (
    orderId INTEGER PRIMARY KEY AUTOINCREMENT,
    userId INTEGER NOT NULL,
    trainId INTEGER NOT NULL,
    passengerName TEXT NOT NULL,
    purchaseTime TEXT NOT NULL,
    status INTEGER NOT NULL,
    FOREIGN KEY (userId) REFERENCES User(userId),
    FOREIGN KEY (trainId) REFERENCES Train(trainId)
);
