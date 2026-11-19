#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
using namespace std;
class Customer {
    string name, phone, address;
    static bool invalidText(const string& text) {
        return text.find_first_not_of(" \t") == string::npos || text.find_first_of("\r\n") != string::npos;
    }
public:
    Customer(const string& name, const string& phone, const string& address) : name(name), phone(phone), address(address) {
        if (invalidText(name) || invalidText(address)) throw runtime_error("Enter a name and address on single lines.");
        if (phone.size() != 10 || phone.find_first_not_of("0123456789") != string::npos)
            throw runtime_error("Phone number must contain exactly 10 digits.");
    }
    virtual ~Customer() = default;
    virtual string category() const { return "Regular"; }
    virtual double discount() const { return 0; }
    void print(int id) const { cout << id << " | " << name << " (" << category() << ") | " << phone << " | " << address << '\n'; }
    void save(ostream& out, int id) const {
        out << "CUSTOMER " << id << ' ' << category() << ' ' << quoted(name) << ' ' << quoted(phone) << ' ' << quoted(address) << '\n';
    }
};
class DiscountBenefit { protected: static constexpr double rate = 0.10; };
class PremiumCustomer : public Customer, public DiscountBenefit {
public:
    using Customer::Customer;
    string category() const override { return "Premium"; }
    double discount() const override { return rate; }
};
class Room {
    friend class Hotel;
    bool occupied = false;
    int number; string type;
public:
    Room(int number, const string& type) : number(number), type(type) {}
    virtual ~Room() = default;
    virtual double price(int days) const = 0;
};
template<int DailyRate>
struct PricedRoom : Room {
    using Room::Room;
    double price(int days) const override { return static_cast<double>(DailyRate) * days; }
};
using StandardRoom = PricedRoom<1500>;
using DeluxeRoom = PricedRoom<2500>;
using SuiteRoom = PricedRoom<4000>;
struct FoodItem { int id; string name; double price; };
struct FoodOrder { FoodItem item; int quantity; };
class Bill {
    double room, food, discount;
public:
    Bill(double room, double food, double discount = 0) : room(room), food(food), discount(discount) {}
    Bill operator+(const Bill& other) const { return {room + other.room, food + other.food, discount + other.discount}; }
    Bill withDiscount(double rate) const { return {room, food, (room + food) * rate}; }
    bool readMatches(istream& in) const {
        string tag; double r, f, d;
        return (in >> tag >> r >> f >> d) && tag == "BILL" && room == r && food == f && discount == d;
    }
    void save(ostream& out) const { out << "BILL " << room << ' ' << food << ' ' << discount << '\n'; }
    friend void printBill(const Bill& bill) {
        double subtotal = bill.room + bill.food;
        cout << "Room charges: Rs. " << bill.room << "\nFood charges: Rs. " << bill.food
             << "\nSubtotal: Rs. " << subtotal << "\nDiscount (" << setprecision(0)
             << (subtotal ? bill.discount / subtotal * 100 : 0) << "%): Rs. " << setprecision(2)
             << bill.discount << "\nFinal amount: Rs. " << subtotal - bill.discount << '\n';
    }
};
struct Booking {
    int customer, room, days;
    vector<FoodOrder> orders;
    optional<Bill> finalBill;
    bool active() const { return !finalBill; }
};
template<class Map>
int nextId(const Map& records, int firstMinusOne) {
    int last = records.empty() ? firstMinusOne : records.rbegin()->first;
    if (last == numeric_limits<int>::max()) throw runtime_error("No more IDs are available.");
    return last + 1;
}
string readLine(const char* prompt) {
    string line; cout << prompt;
    if (!getline(cin, line)) throw runtime_error("End of input.");
    return line;
}
int readNumber(const char* prompt) {
    istringstream in(readLine(prompt));
    int number; char extra;
    if (!(in >> number) || in >> extra) throw runtime_error("Enter one whole number.");
    return number;
}
class Hotel {
    static constexpr int floors = 3, roomsPerFloor = 5, foodCount = 5;
    unique_ptr<Room> rooms[floors][roomsPerFloor];
    map<int, unique_ptr<Customer>> customers;
    map<int, Booking> bookings;
    const FoodItem menu[foodCount]{{1, "Tea", 30}, {2, "Coffee", 50}, {3, "Sandwich", 100},
                          {4, "Veg Thali", 180}, {5, "Paneer Meal", 250}};
    const string types[3]{"Standard", "Deluxe", "Suite"};
    const filesystem::path file = "hotel_data.txt";
    Room& roomAt(int number) const {
        int floor = number / 100 - 1, position = number % 100 - 1;
        if (floor < 0 || floor >= floors || position < 0 || position >= roomsPerFloor)
            throw runtime_error("Invalid room number.");
        return *rooms[floor][position];
    }
    Booking& activeBooking(int id) {
        auto& b = bookings.at(id);
        if (!b.active()) throw runtime_error("Booking is already completed.");
        return b;
    }
    static unique_ptr<Customer> makeCustomer(const string& name, const string& phone, const string& address, bool premium) {
        if (premium) return make_unique<PremiumCustomer>(name, phone, address);
        return make_unique<Customer>(name, phone, address);
    }
    Bill calculateBill(const Booking& b) const {
        double food = 0;
        for (const auto& order : b.orders) food += order.item.price * order.quantity;
        if (!isfinite(food)) throw runtime_error("Food subtotal is too large.");
        return (Bill(roomAt(b.room).price(b.days), 0) + Bill(0, food)).withDiscount(customers.at(b.customer)->discount());
    }
    void load() {
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
            if (status == "Completed" && !calculated.readMatches(in)) throw runtime_error("Invalid final bill.");
            if (status == "Completed") b.finalBill = calculated;
            else {
                Room& room = roomAt(b.room);
                if (room.occupied) throw runtime_error("Room is already booked.");
                room.occupied = true;
            }
            bookings.emplace(id, move(b));
        }
        if (tag != "END" || in.bad() || !(in >> ws).eof()) throw runtime_error("Invalid end of records.");
    }
public:
    Hotel() {
        for (int i = 0; i < floors * roomsPerFloor; ++i) {
            int floor = i / roomsPerFloor, position = i % roomsPerFloor;
            int number = (floor + 1) * 100 + position + 1;
            auto& room = rooms[floor][position];
            if (position < 2) room = make_unique<StandardRoom>(number, types[0]);
            else if (position < 4) room = make_unique<DeluxeRoom>(number, types[1]);
            else room = make_unique<SuiteRoom>(number, types[2]);
        }
        load();
    }
    void save() const {
        auto temporary = file.string() + ".tmp"; ofstream out(temporary);
        out << "HOTEL 1\n" << setprecision(numeric_limits<double>::max_digits10);
        for (const auto& [id, c] : customers) c->save(out, id);
        for (const auto& [id, b] : bookings) {
            out << "BOOKING " << id << ' ' << b.customer << ' ' << b.room << ' ' << b.days
                << (b.active() ? " Active " : " Completed ") << b.orders.size() << '\n';
            for (const auto& o : b.orders)
                out << "FOOD " << o.item.id << ' ' << quoted(o.item.name) << ' ' << o.item.price << ' ' << o.quantity << '\n';
            if (b.finalBill) b.finalBill->save(out);
        }
        out << "END\n"; out.close();
        if (!out) throw runtime_error("Cannot write records.");
        filesystem::rename(temporary, file);
    }
    void registerCustomer() {
        string name = readLine("Name: "), phone = readLine("Phone number (10 digits): "), address = readLine("Address: ");
        int type = readNumber("Customer type (1 Regular, 2 Premium): ");
        if (type != 1 && type != 2) throw runtime_error("Choose customer type 1 or 2.");
        int id = nextId(customers, 100);
        customers.emplace(id, makeCustomer(name, phone, address, type == 2));
        try { save(); } catch (...) { customers.erase(id); throw; }
        cout << "Customer registered. Customer ID: " << id << '\n';
    }
    void showCustomers() const { for (const auto& [id, c] : customers) c->print(id); }
    void showRooms() const {
        for (const auto& floor : rooms) for (const auto& room : floor)
            cout << room->number << " | " << room->type << " | Rs. " << room->price(1) << "/day | "
                 << (room->occupied ? "Occupied" : "Available") << '\n';
    }
    int bookRoom(int customer, int number, int days) {
        if (!customers.count(customer)) throw runtime_error("Customer was not found.");
        Room& room = roomAt(number);
        if (room.occupied || days <= 0) throw runtime_error("Choose an available room and a positive stay duration.");
        int id = nextId(bookings, 1000);
        bookings.emplace(id, Booking{customer, number, days, {}, {}});
        try { save(); } catch (...) { bookings.erase(id); throw; }
        room.occupied = true;
        return id;
    }
    int bookRoom(int customer, const string& type, int days) {
        for (const auto& floor : rooms) for (const auto& room : floor)
            if (room->type == type && !room->occupied) return bookRoom(customer, room->number, days);
        throw runtime_error("No " + type + " rooms are available.");
    }
    void bookRoom() {
        int customer = readNumber("Customer ID: "), room = readNumber("Room number, or -1 Standard, -2 Deluxe, -3 Suite: ");
        int days = readNumber("Number of days: "), id = room >= -3 && room <= -1 ? bookRoom(customer, types[-room - 1], days)
                                         : bookRoom(customer, room, days);
        cout << "Room booked. Booking ID: " << id << '\n';
    }
    void showBookings(bool activeOnly) const {
        for (const auto& [id, b] : bookings) if (!activeOnly || b.active())
            cout << id << " | Customer " << b.customer << " | Room " << b.room << " | " << b.days
                 << " day(s) | " << (b.active() ? "Active" : "Completed") << '\n';
    }
    void showFood() const { for (const auto& item : menu) cout << item.id << ". " << item.name << " - Rs. " << item.price << '\n'; }
    void orderFood() {
        auto& b = activeBooking(readNumber("Booking ID: "));
        showFood();
        int item = readNumber("Food item ID: "), quantity = readNumber("Quantity: ");
        if (item < 1 || item > foodCount || quantity <= 0) throw runtime_error("Enter a valid food ID and positive quantity.");
        b.orders.push_back({menu[item - 1], quantity});
        try { calculateBill(b); save(); } catch (...) { b.orders.pop_back(); throw; }
    }
    void showBill(int id) const {
        const auto& b = bookings.at(id);
        cout << (b.active() ? "Bill preview" : "Final bill") << " | Booking " << id
             << " | Room " << b.room << " (" << roomAt(b.room).type << ") | " << b.days << " day(s)\n";
        customers.at(b.customer)->print(b.customer);
        for (const auto& o : b.orders)
            cout << o.item.name << " x " << o.quantity << " @ Rs. " << o.item.price << " = Rs. " << o.item.price * o.quantity << '\n';
        printBill(b.active() ? calculateBill(b) : *b.finalBill);
    }
    void checkout(int id) {
        auto& b = activeBooking(id);
        showBill(id);
        string answer = readLine("Confirm payment (y/n): ");
        if (answer == "n" || answer == "N") { cout << "Checkout cancelled.\n"; return; }
        if (answer != "y" && answer != "Y") throw runtime_error("Enter y or n.");
        b.finalBill = calculateBill(b);
        try { save(); } catch (...) { b.finalBill.reset(); throw; }
        roomAt(b.room).occupied = false;
        cout << "Checkout successful.\n";
    }
    void billingHistory() const { for (const auto& [id, b] : bookings) if (!b.active()) showBill(id); }
};
int main() try {
    Hotel hotel;
    cout << fixed << setprecision(2);
    while (true) try {
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
            case 0: hotel.save(); cout << "Records saved. Goodbye.\n"; return 0;
            default: cout << "Enter a listed menu option.\n";
        }
    } catch (const exception& error) {
        if (!cin) return 0;
        cout << "Error: " << error.what() << '\n';
    }
} catch (const exception& error) {
    cerr << "Startup error: " << error.what() << '\n'; return 1;
}
