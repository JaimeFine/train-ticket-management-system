# Train Ticket Management System

## Structure

```txt
TrainTicketSystem/
│
├── src/
│   ├── login/
│   ├── ticket/
│   ├── train/
│   ├── statistics/
│   ├── database/
│   └── ui/
├── include/
├── resources/
├── docs/
├── tests/
├── database/
├── CMakeLists.txt
└── README.md
```

## **VERY IMPORTANT: DataBase Schema**

### **User**
- id  
- username  
- password  
- role  
- enabled  

### **Train**
- train_id  
- train_no  
- departure_station  
- arrival_station  
- departure_time  
- arrival_time  

### **Station**
- station_id  
- name  

### **Ticket**
- ticket_id  
- train_id  
- seat_type  
- remaining  
- price  

### **Order**
- order_id  
- user_id  
- ticket_id  
- status  
- purchase_time  
