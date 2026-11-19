# Hotel and Resort Management System

A C++17 mini-project for hotel reception. It brings customer registration, room occupancy, food orders, billing and checkout into one offline console program. The objectives are to prevent double bookings, produce accurate bills and retain customer and booking history across restarts.

## Build and run

All source code is in `main.cpp`. From the project directory, run:

```bash
g++ -std=c++17 -Wall -Wextra main.cpp -o hotel
./hotel
```

## Features and algorithms

- Register Regular or Premium customers. Enter a name, a 10-digit phone number and an address. IDs start at 101.
- View 15 rooms on three floors. Positions 1–2 are Standard at Rs. 1,500 per day, 3–4 are Deluxe at Rs. 2,500 and 5 is a Suite at Rs. 4,000. Room 203 maps to array row `203 / 100 - 1 = 1` and column `203 % 100 - 1 = 2`. Hotel validates the room before accessing the array.
- Enter a room number or use `-1` for Standard, `-2` for Deluxe or `-3` for Suite to take the first available room of that type. Hotel verifies the customer, availability and stay duration before assigning an ID starting at 1001. A booking occupies its room immediately and stays active until checkout.
- Order Tea, Coffee, Sandwich, Veg Thali or Paneer Meal against an active booking. Every order keeps its quantity and unit price. The bill shows each line and adds the line totals.
- Preview a bill at any time. The `Bill` constructor adds the room and food charges. The customer’s rate is applied once to the combined subtotal. Regular customers receive 0% and Premium customers receive 10%.
- Confirm checkout with `y` or cancel with `n`. A successful checkout saves the final bill before releasing the room. Booking history includes active and completed stays. Billing history shows final bills.

## Class relationships

`PremiumCustomer` inherits from `Customer` and overrides its category and discount. A `Room` object stores its type, daily rate and occupancy. A `Booking` links a customer and a room by ID and contains its food orders and final bill.

| OOP concept | Where to find it |
| --- | --- |
| Classes and objects | `Hotel` manages customers, bookings and a 2D array of room objects. |
| Encapsulation and constructors | `Room` keeps its rate and occupancy private. Its constructor sets the room details. |
| Single inheritance and overriding | `PremiumCustomer` extends `Customer` and overrides `discount()`. |
| Runtime polymorphism | Hotel calls `discount()` through a `Customer` pointer. |
| Function overloading | `Hotel::bookRoom` accepts either a room number or a room type. |
| Composition | Each `Booking` contains food orders and a `Bill`. |

## Saved records

Run one instance per records directory. The program uses `hotel_data.txt` in its working directory and saves each successful registration, booking, food order and checkout. Menu options 12 and 0 also save. All four record types share a single snapshot so a checkout’s status and bill can be committed together. The program writes a sibling temporary file and renames it over the previous snapshot only after the write succeeds. If a save fails, the affected operation rolls back in memory.

The file starts with `HOTEL 1` and ends with `END`. Fields are separated by spaces. Text fields use double quotes and escape embedded quotes and backslashes. A booking line states how many food lines follow it. A completed booking also has a bill line.

```text
HOTEL 1
CUSTOMER 101 Premium "Rahul Sharma" "9876543210" "Pune"
BOOKING 1001 101 103 3 Completed 1
FOOD 4 "Veg Thali" 180 2
BILL 7500 360 786
END
```

If `hotel_data.txt` is missing, the program looks for the previous version’s `data/records.txt` and saves the loaded records to `hotel_data.txt` on the next save. If neither file exists, the hotel starts empty. At startup, active bookings restore room occupancy. If a saved file is unreadable or malformed, startup reports the problem and exits so staff can recover the file without overwriting it.

## Sample workflow

With an empty dataset, register Rahul Sharma as Premium, choose room 103 for three days, and order two Veg Thalis against booking 1001. Option 8 produces this breakdown.

```text
Veg Thali x 2 @ Rs. 180.00 = Rs. 360.00
Room charges: Rs. 7500.00
Food charges: Rs. 360.00
Subtotal: Rs. 7860.00
Discount (10%): Rs. 786.00
Final amount: Rs. 7074.00
```

In option 9, enter `1001` and then `y`. Room 103 becomes available and option 11 displays the final bill.

## Verify the program

To use a fresh dataset, build the program and run its absolute path from an empty temporary directory. Follow the sample workflow, then try these checks.

- Try to book room 103 twice while booking 1001 is active. The second booking should be rejected.
- At checkout, enter `n` and check that room 103 is still occupied. Enter `y` on the next attempt and check that it is available.
- Restart the program. Option 11 should still show the Rs. 7,074.00 bill, and a second checkout of booking 1001 should be rejected.
- Book a room for a Regular customer and preview the bill to see the 0% discount.

## Viva walkthrough

Use `main.cpp` to trace the demonstration after checkout.

1. Follow `main` into `runMenu` and then into a `bookRoom` overload. Trace how the room number maps to the 2D array.
2. Follow `calculateBill` to `Room::price` and then to `Customer::discount`. Explain how a Premium customer gets a 10% discount.
3. In `checkout`, trace how the booking is saved and the room is released. In `load`, see how active bookings restore occupancy.

# Hotel-and-Resort-Management-System-in-C-
# Hotel-and-Resort-Management-System-in-C-
