#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
using namespace std;
class Customer {
    string name, phone, address;
    // Checks for blank or multiline text.
    static bool invalidText(const string& text) {
        return text.find_first_not_of(" \t") == string::npos || text.find_first_of("\r\n") != string::npos;
    }
public:
    // Creates a customer and validates their contact details.
    Customer(const string& name, const string& phone, const string& address) : name(name), phone(phone), address(address) {
        if (invalidText(name) || invalidText(address)) throw runtime_error("Enter a name and address on single lines.");
        if (phone.size() != 10 || phone.find_first_not_of("0123456789") != string::npos)
            throw runtime_error("Phone number must contain exactly 10 digits.");
    }
    // Allows safe destruction through a Customer pointer.
    virtual ~Customer() = default;
    // Returns the regular customer category.
    virtual string category() const { return "Regular"; }
    // Returns the regular customer discount rate.
    virtual double discount() const { return 0; }
    // Displays the customer's ID and contact details.
    void print(int id) const { cout << id << " | " << name << " (" << category() << ") | " << phone << " | " << address << '\n'; }
    // Writes a customer record with quoted contact fields.
    void save(ostream& out, int id) const {
        out << "CUSTOMER " << id << ' ' << category() << ' ' << quoted(name) << ' ' << quoted(phone) << ' ' << quoted(address) << '\n';
    }
};
class PremiumCustomer : public Customer {
public:
    // Creates a premium customer using the same contact validation.
    PremiumCustomer(const string& name, const string& phone, const string& address)
        : Customer(name, phone, address) {}
    // Returns the premium customer category.
    string category() const override { return "Premium"; }
    // Returns the 10% premium discount rate.
    double discount() const override { return 0.10; }
};
class Room {
    int number = 0;
    string type;
    double dailyRate = 0;
    bool occupied = false;
public:
    // Creates an empty room for the room grid.
    Room() = default;
    // Creates a room with its number, type, and daily rate.
    Room(int number, const string& type, double rate) : number(number), type(type), dailyRate(rate) {}
    // Returns the room number.
    int getNumber() const { return number; }
    // Returns the room type.
    const string& getType() const { return type; }
    // Checks whether the room is booked.
    bool isOccupied() const { return occupied; }
    // Updates the room's occupancy.
    void setOccupied(bool value) { occupied = value; }
    // Calculates the room charge for the given stay length.
    double price(int days) const { return dailyRate * days; }
};
struct FoodItem { int id; string name; double price; };
struct FoodOrder { FoodItem item; int quantity; };
class Bill {
    double room, food, discount;
public:
    // Stores the room and food charges and calculates the discount.
    Bill(double room = 0, double food = 0, double rate = 0)
        : room(room), food(food), discount((room + food) * rate) {}
    // Checks whether a saved bill matches the calculated amounts.
    bool readMatches(istream& in) const {
        string tag; double r, f, d;
        return (in >> tag >> r >> f >> d) && tag == "BILL" && room == r && food == f && discount == d;
    }
    // Writes the bill amounts to the records file.
    void save(ostream& out) const { out << "BILL " << room << ' ' << food << ' ' << discount << '\n'; }
    // Displays the charges, discount, and final amount.
    void print() const {
        double subtotal = room + food;
        cout << "Room charges: Rs. " << room << "\nFood charges: Rs. " << food
             << "\nSubtotal: Rs. " << subtotal << "\nDiscount (" << setprecision(0)
             << (subtotal ? discount / subtotal * 100 : 0) << "%): Rs. " << setprecision(2)
             << discount << "\nFinal amount: Rs. " << subtotal - discount << '\n';
    }
};
struct Booking {
    int customer, room, days;
    vector<FoodOrder> orders;
    Bill finalBill;
    bool completed = false;
    // Checks whether checkout is still pending.
    bool active() const { return !completed; }
};
// Returns the next ID or reports that the limit was reached.
int nextId(int last) {
    if (last == numeric_limits<int>::max()) throw runtime_error("No more IDs are available.");
    return last + 1;
}
// Prompts for and reads one line of input.
string readLine(const char* prompt) {
    string line; cout << prompt;
    if (!getline(cin, line)) throw runtime_error("End of input.");
    return line;
}
// Reads a single whole number from the prompt.
int readNumber(const char* prompt) {
    // Reject input with anything after the integer except whitespace.
    istringstream in(readLine(prompt));
    int number; char extra;
    if (!(in >> number) || in >> extra) throw runtime_error("Enter one whole number.");
    return number;
}
class Hotel {
    static constexpr int floors = 3, roomsPerFloor = 5, foodCount = 5;
    Room rooms[floors][roomsPerFloor];
    map<int, unique_ptr<Customer>> customers;
    map<int, Booking> bookings;
    const FoodItem menu[foodCount]{{1, "Tea", 30}, {2, "Coffee", 50}, {3, "Sandwich", 100},
                          {4, "Veg Thali", 180}, {5, "Paneer Meal", 250}};
    const string types[3]{"Standard", "Deluxe", "Suite"};
    const filesystem::path file = "hotel_data.txt";
    // Checks that the floor and room position are valid.
    // The hundreds digit is the floor, and the last two digits are the room position.
    void checkRoom(int number) const {
        if (number / 100 < 1 || number / 100 > floors || number % 100 < 1 || number % 100 > roomsPerFloor)
            throw runtime_error("Invalid room number.");
    }
    // Looks up a room so its occupancy can be updated.
    Room& roomAt(int number) { checkRoom(number); return rooms[number / 100 - 1][number % 100 - 1]; }
    // Looks up a room for read-only access.
    const Room& roomAt(int number) const { checkRoom(number); return rooms[number / 100 - 1][number % 100 - 1]; }
    // Finds a booking that is still active.
    Booking& activeBooking(int id) {
        Booking& b = bookings.at(id);
        if (!b.active()) throw runtime_error("Booking is already completed.");
        return b;
    }
    // Creates a regular or premium customer.
    static unique_ptr<Customer> makeCustomer(const string& name, const string& phone, const string& address, bool premium) {
        if (premium) return make_unique<PremiumCustomer>(name, phone, address);
        return make_unique<Customer>(name, phone, address);
    }
    // Totals room and food charges and applies the customer discount.
    Bill calculateBill(const Booking& b) const {
        double food = 0;
        for (const auto& order : b.orders) food += order.item.price * order.quantity;
        if (!isfinite(food)) throw runtime_error("Food subtotal is too large.");
        return Bill(roomAt(b.room).price(b.days), food, customers.at(b.customer)->discount());
    }
    // Loads and validates saved hotel records.
    void load() {
        // Fall back to the older records path if the current file is missing.
        auto source = filesystem::exists(file) ? file : filesystem::path("data/records.txt");
        if (!filesystem::exists(source)) return;
        ifstream in(source);
        string tag; int version, id;
        if (!(in >> tag >> version) || tag != "HOTEL" || version != 1) throw runtime_error("Invalid records header.");
        while (in >> tag && tag != "END") {
            if (!(in >> id)) throw runtime_error("Missing record ID.");
            if (tag == "CUSTOMER") {
                string category, name, phone, address;
                if (!(in >> category >> quoted(name) >> quoted(phone) >> quoted(address)) ||
                    id < 101 || customers.count(id) || (category != "Premium" && category != "Regular"))
                    throw runtime_error("Invalid customer record.");
                customers.emplace(id, makeCustomer(name, phone, address, category == "Premium"));
                continue;
            }
            Booking b{}; string status; int count;
            if (tag != "BOOKING" || !(in >> b.customer >> b.room >> b.days >> status >> count) ||
                id < 1001 || bookings.count(id) || b.days <= 0 || count < 0 || (status != "Active" && status != "Completed"))
                throw runtime_error("Invalid booking record.");
            for (int i = 0; i < count; ++i) {
                FoodOrder o{};
                if (!(in >> tag >> o.item.id >> quoted(o.item.name) >> o.item.price >> o.quantity) ||
                    tag != "FOOD" || o.item.id < 1 || o.item.id > foodCount || o.item.name.empty() || o.item.price <= 0 || o.quantity <= 0)
                    throw runtime_error("Invalid food order record.");
                b.orders.push_back(o);
            }
            Bill calculated = calculateBill(b);
            // Verify completed bills and rebuild room occupancy from active bookings.
            if (status == "Completed" && !calculated.readMatches(in)) throw runtime_error("Invalid final bill.");
            if (status == "Completed") { b.finalBill = calculated; b.completed = true; }
            else {
                Room& room = roomAt(b.room);
                if (room.isOccupied()) throw runtime_error("Room is already booked.");
                room.setOccupied(true);
            }
            bookings.emplace(id, move(b));
        }
        if (tag != "END" || in.bad() || !(in >> ws).eof()) throw runtime_error("Invalid end of records.");
    }
public:
    // Sets up the room grid and loads saved records.
    Hotel() {
        const double rates[]{1500, 2500, 4000};
        for (int floor = 0; floor < floors; ++floor) for (int position = 0; position < roomsPerFloor; ++position) {
            int number = (floor + 1) * 100 + position + 1;
            int type = position / 2;
            rooms[floor][position] = Room(number, types[type], rates[type]);
        }
        load();
    }
    // Saves all customer and booking records.
    void save() const {
        // Write a complete temporary file before replacing the saved records.
        auto temporary = file.string() + ".tmp"; ofstream out(temporary);
        out << "HOTEL 1\n" << setprecision(numeric_limits<double>::max_digits10);
        for (const auto& [id, c] : customers) c->save(out, id);
        for (const auto& [id, b] : bookings) {
            out << "BOOKING " << id << ' ' << b.customer << ' ' << b.room << ' ' << b.days
                << (b.active() ? " Active " : " Completed ") << b.orders.size() << '\n';
            for (const auto& o : b.orders)
                out << "FOOD " << o.item.id << ' ' << quoted(o.item.name) << ' ' << o.item.price << ' ' << o.quantity << '\n';
            if (b.completed) b.finalBill.save(out);
        }
        out << "END\n"; out.close();
        if (!out) throw runtime_error("Cannot write records.");
        filesystem::rename(temporary, file);
    }
    // Prompts for customer details and saves the new customer.
    void registerCustomer() {
        string name = readLine("Name: "), phone = readLine("Phone number (10 digits): "), address = readLine("Address: ");
        int type = readNumber("Customer type (1 Regular, 2 Premium): ");
        if (type != 1 && type != 2) throw runtime_error("Choose customer type 1 or 2.");
        int id = nextId(customers.empty() ? 100 : customers.rbegin()->first);
        customers.emplace(id, makeCustomer(name, phone, address, type == 2));
        try { save(); } catch (...) { customers.erase(id); throw; }
        cout << "Customer registered. Customer ID: " << id << '\n';
    }
    // Lists all registered customers.
    void showCustomers() const { for (const auto& [id, c] : customers) c->print(id); }
    // Lists room rates and availability.
    void showRooms() const {
        for (const auto& floor : rooms) for (const Room& room : floor)
            cout << room.getNumber() << " | " << room.getType() << " | Rs. " << room.price(1) << "/day | "
                 << (room.isOccupied() ? "Occupied" : "Available") << '\n';
    }
    // Books a specific available room for a customer.
    int bookRoom(int customer, int number, int days) {
        if (!customers.count(customer)) throw runtime_error("Customer was not found.");
        Room& room = roomAt(number);
        if (room.isOccupied() || days <= 0) throw runtime_error("Choose an available room and a positive stay duration.");
        int id = nextId(bookings.empty() ? 1000 : bookings.rbegin()->first);
        bookings.emplace(id, Booking{customer, number, days, {}, {}});
        // Roll back the booking if saving fails and mark the room occupied only after success.
        try { save(); } catch (...) { bookings.erase(id); throw; }
        room.setOccupied(true);
        return id;
    }
    // Books the first available room of the requested type.
    int bookRoom(int customer, const string& type, int days) {
        for (const auto& floor : rooms) for (const Room& room : floor)
            if (room.getType() == type && !room.isOccupied()) return bookRoom(customer, room.getNumber(), days);
        throw runtime_error("No " + type + " rooms are available.");
    }
    // Prompts for booking details and books by room number or type.
    void bookRoom() {
        int customer = readNumber("Customer ID: "), room = readNumber("Room number, or -1 Standard, -2 Deluxe, -3 Suite: ");
        int days = readNumber("Number of days: "), id = room >= -3 && room <= -1 ? bookRoom(customer, types[-room - 1], days)
                                         : bookRoom(customer, room, days);
        cout << "Room booked. Booking ID: " << id << '\n';
    }
    // Lists active bookings or the full booking history.
    void showBookings(bool activeOnly) const {
        for (const auto& [id, b] : bookings) if (!activeOnly || b.active())
            cout << id << " | Customer " << b.customer << " | Room " << b.room << " | " << b.days
                 << " day(s) | " << (b.active() ? "Active" : "Completed") << '\n';
    }
    // Displays the food menu and prices.
    void showFood() const { for (const auto& item : menu) cout << item.id << ". " << item.name << " - Rs. " << item.price << '\n'; }
    // Adds a food order to an active booking and saves it.
    void orderFood() {
        Booking& b = activeBooking(readNumber("Booking ID: "));
        showFood();
        int item = readNumber("Food item ID: "), quantity = readNumber("Quantity: ");
        if (item < 1 || item > foodCount || quantity <= 0) throw runtime_error("Enter a valid food ID and positive quantity.");
        b.orders.push_back({menu[item - 1], quantity});
        // Remove the new order if billing validation or saving fails.
        try { calculateBill(b); save(); } catch (...) { b.orders.pop_back(); throw; }
    }
    // Displays a bill preview or the saved final bill.
    void showBill(int id) const {
        const Booking& b = bookings.at(id);
        cout << (b.active() ? "Bill preview" : "Final bill") << " | Booking " << id
             << " | Room " << b.room << " (" << roomAt(b.room).getType() << ") | " << b.days << " day(s)\n";
        customers.at(b.customer)->print(b.customer);
        for (const auto& o : b.orders)
            cout << o.item.name << " x " << o.quantity << " @ Rs. " << o.item.price << " = Rs. " << o.item.price * o.quantity << '\n';
        Bill bill = b.active() ? calculateBill(b) : b.finalBill; bill.print();
    }
    // Confirms payment, completes the booking, and frees the room.
    void checkout(int id) {
        Booking& b = activeBooking(id);
        showBill(id);
        string answer = readLine("Confirm payment (y/n): ");
        if (answer == "n" || answer == "N") { cout << "Checkout cancelled.\n"; return; }
        if (answer != "y" && answer != "Y") throw runtime_error("Enter y or n.");
        b.finalBill = calculateBill(b);
        b.completed = true;
        // Keep the booking active if saving fails and release the room only after success.
        try { save(); } catch (...) { b.completed = false; throw; }
        roomAt(b.room).setOccupied(false);
        cout << "Checkout successful.\n";
    }
    // Displays final bills for completed bookings.
    void billingHistory() const { for (const auto& [id, b] : bookings) if (!b.active()) showBill(id); }
};
// Runs the menu and handles input errors.
void runMenu(Hotel& hotel) {
    cout << fixed << setprecision(2);
    while (true) {
        try {
            cout << "\nHotel and Resort Management\n1. Register Customer\n2. View Customers\n3. View Room Availability\n"
                    "4. Book Room\n5. View Active Bookings\n6. Order Food\n7. View Food Menu\n8. Generate Bill\n"
                    "9. Checkout Customer\n10. View Booking History\n11. View Billing History\n12. Save Records\n0. Save and Exit\n";
            switch (readNumber("Enter your choice: ")) {
                case 1: hotel.registerCustomer(); break;
                case 2: hotel.showCustomers(); break;
                case 3: hotel.showRooms(); break;
                case 4: hotel.bookRoom(); break;
                case 5: hotel.showBookings(true); break;
                case 6: hotel.orderFood(); cout << "Food added.\n"; break;
                case 7: hotel.showFood(); break;
                case 8: hotel.showBill(readNumber("Booking ID: ")); break;
                case 9: hotel.checkout(readNumber("Booking ID: ")); break;
                case 10: hotel.showBookings(false); break;
                case 11: hotel.billingHistory(); break;
                case 12: hotel.save(); cout << "Records saved.\n"; break;
                case 0: hotel.save(); cout << "Records saved. Goodbye.\n"; return;
                default: cout << "Enter a listed menu option.\n";
            }
        } catch (const exception& error) {
            if (!cin) return;
            cout << "Error: " << error.what() << '\n';
        }
    }
}
// Starts the hotel program and reports startup errors.
int main() {
    try {
        Hotel hotel; runMenu(hotel);
    } catch (const exception& error) {
        cerr << "Startup error: " << error.what() << '\n';
        return 1;
    }
}
