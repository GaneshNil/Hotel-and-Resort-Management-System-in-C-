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
- Book by room number or select a type to take the first available room of that type. Hotel verifies the customer, availability and stay duration before assigning an ID starting at 1001. A booking occupies its room immediately and stays active until checkout.
- Order Tea, Coffee, Sandwich, Veg Thali or Paneer Meal against an active booking. Every order keeps its quantity and unit price. The bill shows each line and adds the line totals.
- Preview a bill at any time. The room rate times the number of days and the food subtotal are combined with `Bill::operator+`. The customer’s rate is applied once to the combined subtotal. Regular customers receive 0% and Premium customers receive 10%.
- Confirm checkout with `y` or cancel with `n`. A successful checkout saves the final bill before releasing the room. Booking history includes active and completed stays. Billing history shows final bills.

## Class relationships

In this text diagram, each arrow points from a derived class to its base class.

```text
StandardRoom ──────────> Room (abstract)
DeluxeRoom ────────────> Room
SuiteRoom ─────────────> Room

PremiumCustomer ───────> Customer
PremiumCustomer ───────> DiscountBenefit

Hotel owns Room objects, Customer objects and Booking records.
Booking links a Customer and Room by ID, contains a vector of FoodOrder
values and holds a final Bill after checkout.
Each FoodOrder contains the FoodItem details used when the order was placed.
```

| OOP concept | Where to find it |
| --- | --- |
| Classes, objects, encapsulation and constructors | `Customer`, `Room`, `Hotel`, `Booking`, `FoodItem` and `Bill` model the records and operations. Constructors initialise their data. |
| Abstraction, overriding and runtime polymorphism | `Room` declares pure virtual pricing and type methods. Hotel calls their overrides through a base-class pointer or reference. |
| Single, hierarchical and multiple inheritance | `StandardRoom` extends `Room`. All three room classes share the same base. `PremiumCustomer` extends both `Customer` and `DiscountBenefit`. |
| Function and operator overloading | The two `Hotel::bookRoom` overloads select a room by number or type. `Bill::operator+` combines charge values. |
| 2D arrays, dynamic memory and virtual destructors | `Hotel::roomAvailability` mirrors the rooms’ occupancy. Hotel owns a vector of `Room*` allocated with `new` and deletes each through the virtual `Room` destructor. Customer records use `unique_ptr<Customer>` and a virtual destructor. |
| Exceptions and file handling | Hotel throws exceptions for invalid operations and uses file streams to store records. The menu catches operation errors and allows another action. |
| Function templates and STL | `displayRecords` works with both the customer map and the booking vector. The customer map is keyed by ID and `find_if` locates bookings. |
| Static member and friend function | `Customer::liveCount` tracks live instances. `displayBillSummary` is a friend of `Bill` and prints its private amounts. |

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

If `hotel_data.txt` is missing, the program looks for the previous version’s `data/records.txt` and saves the loaded records to `hotel_data.txt` on the next save. If neither file exists, the hotel starts empty. At startup, active bookings restore both occupancy representations. If a saved file is unreadable or malformed, startup reports the problem and exits so staff can recover the file without overwriting it.

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

1. Start at `main` and follow `runMenu` into either `Hotel::bookRoom` overload. Explain how Hotel validates the room number and uses it to locate the corresponding array cell.
2. Follow `Hotel::calculateBill` to `Room::calculateRoomPrice`. Explain why the call uses the Deluxe override for room 103. Then show how `Bill::operator+` combines the room and food charges before `withDiscount` applies the Premium rate.
3. In `Hotel::checkout`, show the final bill being saved before the room is released. In `Hotel::loadRecords`, explain how active bookings rebuild room occupancy after a restart.
4. Find `Hotel::~Hotel` and `Room::~Room`. Explain why destruction through `Room*` works and why Hotel disables copying.
# Hotel-and-Resort-Management-System-in-C-
# Hotel-and-Resort-Management-System-in-C-
