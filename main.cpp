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
class PremiumCustomer : public Customer {
public:
    PremiumCustomer(const string& name, const string& phone, const string& address)
        : Customer(name, phone, address) {}
    string category() const override { return "Premium"; }
    double discount() const override { return 0.10; }
};
class Room {
    int number = 0;
    string type;
    double dailyRate = 0;
    bool occupied = false;
public:
    Room() = default;
    Room(int number, const string& type, double rate) : number(number), type(type), dailyRate(rate) {}
    int getNumber() const { return number; }
    const string& getType() const { return type; }
    bool isOccupied() const { return occupied; }
    void setOccupied(bool value) { occupied = value; }
    double price(int days) const { return dailyRate * days; }
};
struct FoodItem { int id; string name; double price; };
struct FoodOrder { FoodItem item; int quantity; };
class Bill {
    double room, food, discount;
public:
    Bill(double room = 0, double food = 0, double rate = 0)
        : room(room), food(food), discount((room + food) * rate) {}
    bool readMatches(istream& in) const {
        string tag; double r, f, d;
        return (in >> tag >> r >> f >> d) && tag == "BILL" && room == r && food == f && discount == d;
    }
    void save(ostream& out) const { out << "BILL " << room << ' ' << food << ' ' << discount << '\n'; }
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
    bool active() const { return !completed; }
};
int nextId(int last) {
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
    Room rooms[floors][roomsPerFloor];
    map<int, unique_ptr<Customer>> customers;
    map<int, Booking> bookings;
    const FoodItem menu[foodCount]{{1, "Tea", 30}, {2, "Coffee", 50}, {3, "Sandwich", 100},
                          {4, "Veg Thali", 180}, {5, "Paneer Meal", 250}};
    const string types[3]{"Standard", "Deluxe", "Suite"};
    const filesystem::path file = "hotel_data.txt";
    void checkRoom(int number) const {
        if (number / 100 < 1 || number / 100 > floors || number % 100 < 1 || number % 100 > roomsPerFloor)
            throw runtime_error("Invalid room number.");
    }
    Room& roomAt(int number) { checkRoom(number); return rooms[number / 100 - 1][number % 100 - 1]; }
    const Room& roomAt(int number) const { checkRoom(number); return rooms[number / 100 - 1][number % 100 - 1]; }
    Booking& activeBooking(int id) {
        Booking& b = bookings.at(id);
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
        return Bill(roomAt(b.room).price(b.days), food, customers.at(b.customer)->discount());
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
    Hotel() {
        const double rates[]{1500, 2500, 4000};
        for (int floor = 0; floor < floors; ++floor) for (int position = 0; position < roomsPerFloor; ++position) {
            int number = (floor + 1) * 100 + position + 1;
            int type = position / 2;
            rooms[floor][position] = Room(number, types[type], rates[type]);
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
            if (b.completed) b.finalBill.save(out);
        }
        out << "END\n"; out.close();
        if (!out) throw runtime_error("Cannot write records.");
        filesystem::rename(temporary, file);
    }
    void registerCustomer() {
        string name = readLine("Name: "), phone = readLine("Phone number (10 digits): "), address = readLine("Address: ");
        int type = readNumber("Customer type (1 Regular, 2 Premium): ");
        if (type != 1 && type != 2) throw runtime_error("Choose customer type 1 or 2.");
        int id = nextId(customers.empty() ? 100 : customers.rbegin()->first);
        customers.emplace(id, makeCustomer(name, phone, address, type == 2));
        try { save(); } catch (...) { customers.erase(id); throw; }
        cout << "Customer registered. Customer ID: " << id << '\n';
    }
    void showCustomers() const { for (const auto& [id, c] : customers) c->print(id); }
    void showRooms() const {
        for (const auto& floor : rooms) for (const Room& room : floor)
            cout << room.getNumber() << " | " << room.getType() << " | Rs. " << room.price(1) << "/day | "
                 << (room.isOccupied() ? "Occupied" : "Available") << '\n';
    }
    int bookRoom(int customer, int number, int days) {
        if (!customers.count(customer)) throw runtime_error("Customer was not found.");
        Room& room = roomAt(number);
        if (room.isOccupied() || days <= 0) throw runtime_error("Choose an available room and a positive stay duration.");
        int id = nextId(bookings.empty() ? 1000 : bookings.rbegin()->first);
        bookings.emplace(id, Booking{customer, number, days, {}, {}});
        try { save(); } catch (...) { bookings.erase(id); throw; }
        room.setOccupied(true);
        return id;
    }
    int bookRoom(int customer, const string& type, int days) {
        for (const auto& floor : rooms) for (const Room& room : floor)
            if (room.getType() == type && !room.isOccupied()) return bookRoom(customer, room.getNumber(), days);
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
        Booking& b = activeBooking(readNumber("Booking ID: "));
        showFood();
        int item = readNumber("Food item ID: "), quantity = readNumber("Quantity: ");
        if (item < 1 || item > foodCount || quantity <= 0) throw runtime_error("Enter a valid food ID and positive quantity.");
        b.orders.push_back({menu[item - 1], quantity});
        try { calculateBill(b); save(); } catch (...) { b.orders.pop_back(); throw; }
    }
    void showBill(int id) const {
        const Booking& b = bookings.at(id);
        cout << (b.active() ? "Bill preview" : "Final bill") << " | Booking " << id
             << " | Room " << b.room << " (" << roomAt(b.room).getType() << ") | " << b.days << " day(s)\n";
        customers.at(b.customer)->print(b.customer);
        for (const auto& o : b.orders)
            cout << o.item.name << " x " << o.quantity << " @ Rs. " << o.item.price << " = Rs. " << o.item.price * o.quantity << '\n';
        Bill bill = b.active() ? calculateBill(b) : b.finalBill; bill.print();
    }
    void checkout(int id) {
        Booking& b = activeBooking(id);
        showBill(id);
        string answer = readLine("Confirm payment (y/n): ");
        if (answer == "n" || answer == "N") { cout << "Checkout cancelled.\n"; return; }
        if (answer != "y" && answer != "Y") throw runtime_error("Enter y or n.");
        b.finalBill = calculateBill(b);
        b.completed = true;
        try { save(); } catch (...) { b.completed = false; throw; }
        roomAt(b.room).setOccupied(false);
        cout << "Checkout successful.\n";
    }
    void billingHistory() const { for (const auto& [id, b] : bookings) if (!b.active()) showBill(id); }
};
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
int main() {
    try {
        Hotel hotel; runMenu(hotel);
    } catch (const exception& error) {
        cerr << "Startup error: " << error.what() << '\n';
        return 1;
    }
}
