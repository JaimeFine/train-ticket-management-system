# Requirements

## Functionalities

Train schedule management, real-time ticket availability inquiry, ticket sales / refunds, order saving; historical order statistics, dynamic pricing calculation; Dijkstra/Floyd shortest path transfer recommendation, graph structure storage for routes.

## Basic Structures

Here is the structure of the Train Ticket Management System:

- **Train Ticket Management System**
    - **Administrator Login**
        - Staff Account & Permission Management
            - Create New Ticket Seller Account
            - Modify Account Password
            - Enable/Disable Ticket Seller Account
            - View System Operation Logs
        - Train Schedule & Station Management
            - Add New Train Schedule Info
            - Modify Train Schedule Time
            - Delete (Suspended) Train Schedule
            - Train Start / Stop Control
            - Seat Inventory Configuration
            - Multi-condition Train Search
                - Search by Train Number
                - Search by Departure Station
                - Search by Arrival Station
        - Ticket Data Statistics
            - Sales Statistics per Train
            - Popular Route Ranking
            - Monthly Passenger Flow Summary
            - Refund Data Statistics
    - **Ticket Seller Login**
        - Ticket Booking Operations
            - Multi-condition Ticket Search
                - Search by Travel Date
                - Search by Departure / Arrival Station
            - Book Ticket
            - Refund Ticket
            - Change Ticket
            - Real-time Surplus Ticket Monitoring (Over-ordering)
        - Personal Order Inquiry
            - Search Purchase History by Order Number
            - Search Orders by Passenger Name
        - Refresh Real-time Ticket Availability
        - Save Ticket Operation Logs
    - **Guest Access Portal**
        - Remaining Ticket Inquiry
            - Search Remaining Tickets by Date
            - Search Train by Departure / Arrival Station
    - **Exit System**
