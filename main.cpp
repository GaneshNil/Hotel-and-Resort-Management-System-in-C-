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
#include <vector>
using namespace std;

class Customer {
    string name, phone, address;
    inline static int liveCount = 0;
public:
    Customer(string name, string phone, string address) : name(name), phone(phone), address(address) {
        if (name.find_first_not_of(" \t\r\n") == string::npos || address.find_first_not_of(" \t\r\n") == string::npos)
            throw runtime_error("Enter a name and address.");
        if (phone.size() != 10 || phone.find_first_not_of("0123456789") != string::npos)
            throw runtime_error("Phone number must contain exactly 10 digits.");
        ++liveCount;
    }
    Customer(const Customer&) = delete;
    virtual ~Customer() { --liveCount; }
    virtual string category() const { return "Regular"; }
    virtual double discount() const { return 0; }
    static int count() { return liveCount; }
    void print(int id) const { cout << id << " | " << name << " (" << category() << ") | " << phone << " | " << address << '\n'; }
    void save(ostream& out) const { out << category() << ' ' << quoted(name) << ' ' << quoted(phone) << ' ' << quoted(address); }
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
    int number;
    string type;
public:
    Room(int number, string type) : number(number), type(type) {}
    virtual ~Room() = default;
    virtual double price(int days) const = 0;
};
class StandardRoom : public Room {
public:
    explicit StandardRoom(int number) : Room(number, "Standard") {}
    double price(int days) const override { return 1500.0 * days; }
};
class DeluxeRoom : public Room {
public:
    explicit DeluxeRoom(int number) : Room(number, "Deluxe") {}
    double price(int days) const override { return 2500.0 * days; }
};
class SuiteRoom : public Room {
public:
    explicit SuiteRoom(int number) : Room(number, "Suite") {}
    double price(int days) const override { return 4000.0 * days; }
};

struct FoodItem { int id; string name; double price; };
struct FoodOrder { FoodItem item; int quantity; };
class Bill {
    double room, food, discount;
public:
    Bill(double room, double food, double discount = 0) : room(room), food(food), discount(discount) {
        if (!isfinite(room + food) || room < 0 || food < 0 || !(discount >= 0 && discount <= room + food))
            throw runtime_error("Invalid bill amounts.");
    }
    Bill operator+(const Bill& other) const { return {room + other.room, food + other.food, discount + other.discount}; }
    Bill withDiscount(double rate) const { return {room, food, (room + food) * rate}; }
    bool matches(double r, double f, double d) const { return room == r && food == f && discount == d; }
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
    double foodTotal() const {
        double total = 0;
        for (const auto& order : orders) total += order.item.price * order.quantity;
        return total;
    }
};

template<class Map>
auto& lookup(Map& records, int id, const string& kind) {
    auto it = records.find(id);
    if (it == records.end()) throw runtime_error(kind + " was not found.");
    return it->second;
}

class Hotel {
    static constexpr int floors = 3, roomsPerFloor = 5;
    unique_ptr<Room> rooms[floors][roomsPerFloor];
    map<int, unique_ptr<Customer>> customers;
    map<int, Booking> bookings;
    const FoodItem menu[]{{1, "Tea", 30}, {2, "Coffee", 50}, {3, "Sandwich", 100},
                          {4, "Veg Thali", 180}, {5, "Paneer Meal", 250}};
    const filesystem::path file = "hotel_data.txt";
    Room& roomAt(int number) const {
        int floor = number / 100 - 1, position = number % 100 - 1;
        if (floor < 0 || floor >= floors || position < 0 || position >= roomsPerFloor)
            throw runtime_error("Invalid room number.");
        return *rooms[floor][position];
    }
    Booking& activeBooking(int id) {
        auto& b = lookup(bookings, id, "Booking");
        if (!b.active()) throw runtime_error("Booking is already completed.");
        return b;
    }
    const FoodItem& foodAt(int id) const {
        for (const auto& item : menu) if (item.id == id) return item;
        throw runtime_error("Invalid food item ID.");
    }
    static int nextId(int last) {
        if (last == numeric_limits<int>::max()) throw runtime_error("No more IDs are available.");
        return last + 1;
    }
    static unique_ptr<Customer> makeCustomer(const string& name, const string& phone, const string& address, bool premium) {
        if (premium) return make_unique<PremiumCustomer>(name, phone, address);
        return make_unique<Customer>(name, phone, address);
    }
    Bill calculateBill(const Booking& b) const {
        return (Bill(roomAt(b.room).price(b.days), 0) + Bill(0, b.foodTotal())).withDiscount(lookup(customers, b.customer, "Customer")->discount());
    }
    void load() {
        auto source = filesystem::exists(file) ? file : filesystem::path("data/records.txt");
        if (!filesystem::exists(source)) return;
        ifstream in(source);
        string tag;
        int version;
        if (!(in >> tag >> version) || tag != "HOTEL" || version != 1) throw runtime_error("Invalid records header.");
        while (in >> tag && tag != "END") {
            int id;
            if (!(in >> id)) throw runtime_error("Missing record ID.");
            if (tag == "CUSTOMER") {
                string category, name, phone, address;
                if (!(in >> category >> quoted(name) >> quoted(phone) >> quoted(address)) ||
                    id < 101 || customers.count(id) || (category != "Premium" && category != "Regular"))
                    throw runtime_error("Invalid customer record.");
                customers.emplace(id, makeCustomer(name, phone, address, category == "Premium"));
                continue;
            }
            Booking b{};
            string status;
            int count;
            if (tag != "BOOKING" || !(in >> b.customer >> b.room >> b.days >> status >> count) ||
                id < 1001 || bookings.count(id) || b.days <= 0 || count < 0 || (status != "Active" && status != "Completed"))
                throw runtime_error("Invalid booking record.");
            for (int i = 0; i < count; ++i) {
                FoodOrder o{};
                if (!(in >> tag >> o.item.id >> quoted(o.item.name) >> o.item.price >> o.quantity) ||
                    tag != "FOOD" || o.item.name.empty() || o.item.price <= 0 || o.quantity <= 0)
                    throw runtime_error("Invalid food order record.");
                foodAt(o.item.id);
                b.orders.push_back(o);
            }
            Bill calculated = calculateBill(b);
            if (status == "Completed") {
                double room, food, discount;
                if (!(in >> tag >> room >> food >> discount) || tag != "BILL" || !calculated.matches(room, food, discount))
                    throw runtime_error("Invalid final bill.");
                b.finalBill = calculated;
            } else {
                Room& room = roomAt(b.room);
                if (room.occupied) throw runtime_error("Room is booked twice.");
                room.occupied = true;
            }
            bookings.emplace(id, move(b));
        }
        char extra;
        if (tag != "END" || in >> extra || in.bad()) throw runtime_error("Invalid end of records.");
    }
public:
    Hotel() {
        for (int f = 0; f < floors; ++f)
            for (int p = 0; p < roomsPerFloor; ++p) {
                int number = (f + 1) * 100 + p + 1;
                if (p < 2) rooms[f][p] = make_unique<StandardRoom>(number);
                else if (p < 4) rooms[f][p] = make_unique<DeluxeRoom>(number);
                else rooms[f][p] = make_unique<SuiteRoom>(number);
            }
        load();
    }
    void save() const {
        auto temporary = file.string() + ".tmp";
        ofstream out(temporary);
        out << "HOTEL 1\n" << setprecision(numeric_limits<double>::max_digits10);
        for (const auto& [id, c] : customers) { out << "CUSTOMER " << id << ' '; c->save(out); out << '\n'; }
        for (const auto& [id, b] : bookings) {
            out << "BOOKING " << id << ' ' << b.customer << ' ' << b.room << ' ' << b.days
                << (b.active() ? " Active " : " Completed ") << b.orders.size() << '\n';
            for (const auto& o : b.orders)
                out << "FOOD " << o.item.id << ' ' << quoted(o.item.name) << ' ' << o.item.price << ' ' << o.quantity << '\n';
            if (b.finalBill) b.finalBill->save(out);
        }
        out << "END\n";
        out.close();
        if (!out) throw runtime_error("Cannot write records.");
        filesystem::rename(temporary, file);
    }
    int registerCustomer(const string& name, const string& phone, const string& address, bool premium) {
        int id = nextId(customers.empty() ? 100 : customers.rbegin()->first);
        customers.emplace(id, makeCustomer(name, phone, address, premium));
        try { save(); } catch (...) { customers.erase(id); throw; }
        return id;
    }
    void showCustomers() const {
        for (const auto& [id, c] : customers) c->print(id);
    }
    void showRooms() const {
        for (const auto& floor : rooms)
            for (const auto& room : floor)
                cout << room->number << " | " << room->type << " | Rs. " << room->price(1) << "/day | "
                     << (room->occupied ? "Occupied" : "Available") << '\n';
    }
    int bookRoom(int customer, int number, int days) {
        lookup(customers, customer, "Customer");
        Room& room = roomAt(number);
        if (room.occupied || days <= 0) throw runtime_error("Choose an available room and a positive number of days.");
        int id = nextId(bookings.empty() ? 1000 : bookings.rbegin()->first);
        bookings.emplace(id, Booking{customer, number, days, {}, {}});
        try { save(); } catch (...) { bookings.erase(id); throw; }
        room.occupied = true;
        return id;
    }
    int bookRoom(int customer, const string& type, int days) {
        for (const auto& floor : rooms)
            for (const auto& room : floor)
                if (room->type == type && !room->occupied) return bookRoom(customer, room->number, days);
        throw runtime_error("No " + type + " rooms are available.");
    }
    void showBookings(bool activeOnly) const {
        for (const auto& [id, b] : bookings)
            if (!activeOnly || b.active())
                cout << id << " | Customer " << b.customer << " | Room " << b.room << " | " << b.days
                     << " day(s) | " << (b.active() ? "Active" : "Completed") << '\n';
    }
    void showFood() const {
        for (const auto& item : menu) cout << item.id << ". " << item.name << " - Rs. " << item.price << '\n';
    }
    void orderFood(int id, int item, int quantity) {
        auto& b = activeBooking(id);
        if (quantity <= 0) throw runtime_error("Quantity must be positive.");
        b.orders.push_back({foodAt(item), quantity});
        try { calculateBill(b); save(); } catch (...) { b.orders.pop_back(); throw; }
    }
    void showBill(int id) const {
        const auto& b = lookup(bookings, id, "Booking");
        cout << (b.active() ? "Bill preview" : "Final bill") << " | Booking " << id << " | Customer " << b.customer
             << " | Room " << b.room << " (" << roomAt(b.room).type << ") | " << b.days << " day(s)\n";
        lookup(customers, b.customer, "Customer")->print(b.customer);
        for (const auto& o : b.orders)
            cout << o.item.name << " x " << o.quantity << " @ Rs. " << o.item.price << " = Rs. " << o.item.price * o.quantity << '\n';
        printBill(b.active() ? calculateBill(b) : *b.finalBill);
    }
    void checkout(int id) {
        auto& b = activeBooking(id);
        b.finalBill = calculateBill(b);
        try { save(); } catch (...) { b.finalBill.reset(); throw; }
        roomAt(b.room).occupied = false;
        cout << "Checkout successful. Room " << b.room << " is now available.\n";
    }
    void billingHistory() const {
        for (const auto& [id, b] : bookings) if (!b.active()) showBill(id);
    }
};

struct EndOfInput {};
string readLine(const char* prompt) {
    cout << prompt;
    string line;
    if (!getline(cin, line)) throw EndOfInput{};
    return line;
}
int readNumber(const char* prompt) {
    istringstream in(readLine(prompt));
    int number;
    char extra;
    if (!(in >> number) || in >> extra) throw runtime_error("Enter one whole number.");
    return number;
}

int main() {
    try {
        Hotel hotel;
        cout << fixed << setprecision(2);
        while (true) {
            cout << "\nHotel and Resort Management\n1. Register Customer\n2. View Customers\n3. View Room Availability\n"
                    "4. Book Room\n5. View Active Bookings\n6. Order Food\n7. View Food Menu\n8. Generate Bill\n"
                    "9. Checkout Customer\n10. View Booking History\n11. View Billing History\n12. Save Records\n0. Save and Exit\n";
            try {
                switch (readNumber("Enter your choice: ")) {
                case 1: {
                    auto name = readLine("Name: "), phone = readLine("Phone number (10 digits): "), address = readLine("Address: ");
                    int type = readNumber("Customer type (1 Regular, 2 Premium): ");
                    if (type != 1 && type != 2) throw runtime_error("Choose customer type 1 or 2.");
                    int id = hotel.registerCustomer(name, phone, address, type == 2);
                    cout << "Customer registered. Customer ID: " << id << '\n';
                    break;
                }
                case 2: hotel.showCustomers(); break;
                case 3: hotel.showRooms(); break;
                case 4: {
                    int customer = readNumber("Customer ID: ");
                    hotel.showRooms();
                    int room = readNumber("Room number, or -1 Standard, -2 Deluxe, -3 Suite: ");
                    int days = readNumber("Number of days: ");
                    const string types[]{"Standard", "Deluxe", "Suite"};
                    int id = room >= -3 && room <= -1 ? hotel.bookRoom(customer, types[-room - 1], days)
                                                     : hotel.bookRoom(customer, room, days);
                    cout << "Room booked. Booking ID: " << id << '\n';
                    break;
                }
                case 5: hotel.showBookings(true); break;
                case 6: {
                    int id = readNumber("Booking ID: ");
                    hotel.showFood();
                    int item = readNumber("Food item ID: "), quantity = readNumber("Quantity: ");
                    hotel.orderFood(id, item, quantity);
                    cout << "Food added.\n";
                    break;
                }
                case 7: hotel.showFood(); break;
                case 8: hotel.showBill(readNumber("Booking ID: ")); break;
                case 9: {
                    int id = readNumber("Booking ID: ");
                    hotel.showBill(id);
                    string answer = readLine("Confirm payment (y/n): ");
                    if (answer == "y" || answer == "Y") hotel.checkout(id);
                    else if (answer == "n" || answer == "N") cout << "Checkout cancelled.\n";
                    else throw runtime_error("Enter y or n.");
                    break;
                }
                case 10: hotel.showBookings(false); break;
                case 11: hotel.billingHistory(); break;
                case 12: hotel.save(); cout << "Records saved.\n"; break;
                case 0: hotel.save(); cout << "Records saved. Goodbye.\n"; return 0;
                default: cout << "Enter a listed menu option.\n";
                }
            } catch (const exception& error) { cout << "Error: " << error.what() << '\n'; }
        }
    } catch (const EndOfInput&) {
        cout << "\nEnd of input. Goodbye.\n";
    } catch (const exception& error) {
        cerr << "Startup error: " << error.what() << '\n';
        return 1;
    }
}
