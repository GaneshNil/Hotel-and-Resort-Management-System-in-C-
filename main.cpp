#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
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

// Classes

class CustomerNotFound : public std::runtime_error { using std::runtime_error::runtime_error; };

class InvalidRoom : public std::runtime_error { using std::runtime_error::runtime_error; };

class RoomUnavailable : public std::runtime_error { using std::runtime_error::runtime_error; };

class InvalidBooking : public std::runtime_error { using std::runtime_error::runtime_error; };

class BookingNotFound : public std::runtime_error { using std::runtime_error::runtime_error; };

class Customer {
public:
    Customer(int id, std::string name, std::string phone, std::string address);
    virtual ~Customer() { --liveCount; }
    Customer(const Customer&) = delete;
    Customer& operator=(const Customer&) = delete;

    int getCustomerId() const { return customerId; }
    const std::string& getName() const { return name; }
    const std::string& getPhone() const { return phone; }
    const std::string& getAddress() const { return address; }
    virtual std::string getCategory() const { return "Regular"; }
    virtual double getDiscountRate() const { return 0.0; }
    void displayCustomer(std::ostream& output) const;
    static int getLiveCount() { return liveCount; }

private:
    int customerId;
    std::string name;
    std::string phone;
    std::string address;
    inline static int liveCount = 0;
};

class DiscountBenefit {
public:
    double getDiscountPercentage() const { return discountRate * 100; }

protected:
    explicit DiscountBenefit(double rate);
    double discountRate;
};

class PremiumCustomer : public Customer, public DiscountBenefit {
public:
    PremiumCustomer(int id, std::string name, std::string phone, std::string address);
    std::string getCategory() const override { return "Premium"; }
    double getDiscountRate() const override { return discountRate; }
};

class Room {
public:
    explicit Room(int number) : roomNumber(number) {}
    virtual ~Room() = default;
    virtual double calculateRoomPrice(int days) const = 0;
    virtual std::string getRoomType() const = 0;

    int getRoomNumber() const { return roomNumber; }
    bool isOccupied() const { return occupied; }
    void setOccupied(bool status) { occupied = status; }

protected:
    static double priceForStay(double dailyRate, int days);
    int roomNumber;
    bool occupied = false;
};

class StandardRoom : public Room {
public:
    using Room::Room;
    double calculateRoomPrice(int days) const override;
    std::string getRoomType() const override { return "Standard"; }
};

class DeluxeRoom : public Room {
public:
    using Room::Room;
    double calculateRoomPrice(int days) const override;
    std::string getRoomType() const override { return "Deluxe"; }
};

class SuiteRoom : public Room {
public:
    using Room::Room;
    double calculateRoomPrice(int days) const override;
    std::string getRoomType() const override { return "Suite"; }
};

class FoodItem {
public:
    FoodItem(int id, std::string name, double price);

    int getItemId() const { return itemId; }
    const std::string& getItemName() const { return itemName; }
    double getPrice() const { return price; }

private:
    int itemId;
    std::string itemName;
    double price;
};

class FoodOrder {
public:
    FoodOrder(const FoodItem& item, int quantity);

    const FoodItem& getItem() const { return item; }
    int getQuantity() const { return quantity; }
    double getTotal() const { return item.getPrice() * quantity; }

private:
    FoodItem item;
    int quantity;
};

class Bill {
public:
    Bill(double roomCharge, double foodCharge, double discountAmount = 0.0);
    Bill operator+(const Bill& other) const;
    Bill withDiscount(double rate) const;

    double getRoomCharge() const { return roomCharge; }
    double getFoodCharge() const { return foodCharge; }
    double getDiscountAmount() const { return discountAmount; }
    double getSubtotal() const { return roomCharge + foodCharge; }
    double getTotal() const { return getSubtotal() - discountAmount; }

    friend void displayBillSummary(std::ostream& output, const Bill& bill);

private:
    double roomCharge;
    double foodCharge;
    double discountAmount;
};

class Booking {
public:
    Booking(int id, int customerId, int roomNumber, int days);

    int getBookingId() const { return bookingId; }
    int getCustomerId() const { return customerId; }
    int getRoomNumber() const { return roomNumber; }
    int getNumberOfDays() const { return numberOfDays; }
    bool isActive() const { return !finalBill.has_value(); }
    const std::vector<FoodOrder>& getFoodOrders() const { return foodOrders; }
    double getFoodSubtotal() const;
    const Bill& getFinalBill() const;

private:
    friend class Hotel;

    int bookingId;
    int customerId;
    int roomNumber;
    int numberOfDays;
    std::vector<FoodOrder> foodOrders;
    std::optional<Bill> finalBill;
};

class Hotel {
public:
    explicit Hotel(std::filesystem::path recordsFile = "hotel_data.txt");
    ~Hotel();
    Hotel(const Hotel&) = delete;
    Hotel& operator=(const Hotel&) = delete;

    int registerCustomer(const std::string& name, const std::string& phone,
                         const std::string& address, bool premium);
    const Customer& getCustomer(int customerId) const;
    void displayCustomers(std::ostream& output) const;

    void displayRoomAvailability(std::ostream& output, bool availableOnly = false) const;
    bool isRoomAvailable(int roomNumber) const;
    void requireAvailableRoom(int roomNumber) const;
    int findAvailableRoom(const std::string& roomType) const;
    const Room& getRoom(int roomNumber) const;

    int bookRoom(int customerId, int roomNumber, int days);
    int bookRoom(int customerId, const std::string& roomType, int days);
    const Booking& getBooking(int bookingId) const;
    const Booking& getActiveBooking(int bookingId) const;
    void displayBookings(std::ostream& output, bool activeOnly) const;

    void displayFoodMenu(std::ostream& output) const;
    double orderFood(int bookingId, int itemId, int quantity);
    Bill generateBill(int bookingId) const;
    void displayBill(int bookingId, std::ostream& output) const;
    void checkout(int bookingId);
    void displayBillingHistory(std::ostream& output) const;

    void saveRecords() const;

private:
    static constexpr int floorCount = 3;
    static constexpr int roomsPerFloor = 5;
    int roomAvailability[floorCount][roomsPerFloor]{};
    std::vector<Room*> rooms;
    std::map<int, std::unique_ptr<Customer>> customers;
    std::vector<Booking> bookings;
    const std::vector<FoodItem> foodMenu{
        {1, "Tea", 30}, {2, "Coffee", 50}, {3, "Sandwich", 100},
        {4, "Veg Thali", 180}, {5, "Paneer Meal", 250}
    };
    std::filesystem::path recordsFile;

    Room* roomAt(int roomNumber) const;
    void setRoomOccupancy(int roomNumber, bool occupied);
    std::size_t bookingIndex(int bookingId) const;
    Bill calculateBill(const Booking& booking) const;
    const FoodItem& getFoodItem(int itemId) const;
    void loadRecords();
    void writeRecords(std::ostream& output) const;
    void deleteRooms();
};

// Constructors and member functions

Customer::Customer(int id, std::string name, std::string phone, std::string address)
    : customerId(id), name(std::move(name)), phone(std::move(phone)),
      address(std::move(address)) {
    if (this->name.find_first_not_of(" \t\r\n") == std::string::npos ||
        this->address.find_first_not_of(" \t\r\n") == std::string::npos ||
        this->name.find_first_of("\r\n") != std::string::npos ||
        this->address.find_first_of("\r\n") != std::string::npos) {
        throw std::runtime_error("Enter a name and address, each on a single line.");
    }
    if (this->phone.size() != 10 ||
        !std::all_of(this->phone.begin(), this->phone.end(), [](char digit) {
            return digit >= '0' && digit <= '9';
        })) {
        throw std::runtime_error("Phone number must contain exactly 10 digits.");
    }
    ++liveCount;
}

void Customer::displayCustomer(std::ostream& output) const {
    output << customerId << " | " << name << " | " << getCategory()
           << " | " << phone << " | " << address << '\n';
}

DiscountBenefit::DiscountBenefit(double rate) : discountRate(rate) {
    if (!(rate >= 0.0 && rate <= 1.0)) {
        throw std::runtime_error("Discount rate must be between 0% and 100%.");
    }
}

PremiumCustomer::PremiumCustomer(int id, std::string name, std::string phone,
                                 std::string address)
    : Customer(id, std::move(name), std::move(phone), std::move(address)),
      DiscountBenefit(0.10) {}

double Room::priceForStay(double dailyRate, int days) {
    if (days <= 0) {
        throw InvalidBooking("Number of days must be positive.");
    }
    return dailyRate * days;
}

double StandardRoom::calculateRoomPrice(int days) const {
    return priceForStay(1500.0, days);
}

double DeluxeRoom::calculateRoomPrice(int days) const {
    return priceForStay(2500.0, days);
}

double SuiteRoom::calculateRoomPrice(int days) const {
    return priceForStay(4000.0, days);
}

FoodItem::FoodItem(int id, std::string name, double price)
    : itemId(id), itemName(std::move(name)), price(price) {
    if (!std::isfinite(price) || price <= 0.0) {
        throw std::runtime_error("Food price must be positive and finite.");
    }
}

FoodOrder::FoodOrder(const FoodItem& item, int quantity)
    : item(item), quantity(quantity) {
    if (quantity <= 0) {
        throw std::runtime_error("Quantity must be positive.");
    }
    if (!std::isfinite(getTotal())) {
        throw std::runtime_error("Food order total is too large.");
    }
}

Bill::Bill(double roomCharge, double foodCharge, double discountAmount)
    : roomCharge(roomCharge), foodCharge(foodCharge), discountAmount(discountAmount) {
    if (!std::isfinite(getSubtotal()) || roomCharge < 0.0 || foodCharge < 0.0 ||
        !(discountAmount >= 0.0 && discountAmount <= getSubtotal())) {
        throw std::runtime_error("Invalid bill amounts.");
    }
}

Bill Bill::operator+(const Bill& other) const {
    return Bill(roomCharge + other.roomCharge, foodCharge + other.foodCharge,
                discountAmount + other.discountAmount);
}

Bill Bill::withDiscount(double rate) const {
    if (!(rate >= 0.0 && rate <= 1.0)) {
        throw std::runtime_error("Discount rate must be between 0% and 100%.");
    }
    return Bill(roomCharge, foodCharge, getSubtotal() * rate);
}

void displayBillSummary(std::ostream& output, const Bill& bill) {
    double percent = 0.0;
    if (bill.getSubtotal() > 0.0) {
        percent = (bill.discountAmount / bill.getSubtotal()) * 100.0;
    }
    output << std::fixed << std::setprecision(2)
           << "Room charges: Rs. " << bill.roomCharge
           << "\nFood charges: Rs. " << bill.foodCharge
           << "\nSubtotal: Rs. " << bill.getSubtotal()
           << "\nDiscount (" << std::setprecision(0) << percent << "%): Rs. "
           << std::setprecision(2) << bill.discountAmount
           << "\nFinal amount: Rs. " << bill.getTotal() << '\n';
}

Booking::Booking(int id, int customerId, int roomNumber, int days)
    : bookingId(id), customerId(customerId), roomNumber(roomNumber), numberOfDays(days) {
    if (days <= 0) {
        throw InvalidBooking("Number of days must be positive.");
    }
}

double Booking::getFoodSubtotal() const {
    double subtotal = 0.0;
    for (const FoodOrder& order : foodOrders) {
        subtotal += order.getTotal();
    }
    return subtotal;
}

const Bill& Booking::getFinalBill() const {
    if (isActive()) {
        throw InvalidBooking("This booking has no final bill yet.");
    }
    return *finalBill;
}

// Hotel operations

namespace {
template <typename Records, typename Display>
void displayRecords(const Records& records, std::ostream& output, Display display) {
    bool found = false;
    for (const auto& record : records) {
        if (display(record)) {
            found = true;
        }
    }
    if (!found) {
        output << "No records to display.\n";
    }
}

std::unique_ptr<Customer> makeCustomer(int id, const std::string& name,
                                       const std::string& phone,
                                       const std::string& address, bool premium) {
    if (premium) {
        return std::make_unique<PremiumCustomer>(id, name, phone, address);
    }
    return std::make_unique<Customer>(id, name, phone, address);
}

int nextId(int lastId) {
    if (lastId == std::numeric_limits<int>::max()) {
        throw std::runtime_error("No more IDs are available.");
    }
    return lastId + 1;
}

void requireActiveBooking(const Booking& booking) {
    if (!booking.isActive()) {
        throw InvalidBooking("Booking " + std::to_string(booking.getBookingId()) +
                             " is already completed.");
    }
}
}

Hotel::Hotel(std::filesystem::path recordsFile) : recordsFile(std::move(recordsFile)) {
    rooms.reserve(floorCount * roomsPerFloor);
    try {
        for (int floor = 1; floor <= floorCount; ++floor) {
            for (int position = 1; position <= roomsPerFloor; ++position) {
                const int roomNumber = floor * 100 + position;
                if (position <= 2) {
                    rooms.push_back(new StandardRoom(roomNumber));
                } else if (position <= 4) {
                    rooms.push_back(new DeluxeRoom(roomNumber));
                } else {
                    rooms.push_back(new SuiteRoom(roomNumber));
                }
            }
        }
        loadRecords();
    } catch (...) {
        deleteRooms();
        throw;
    }
}

Hotel::~Hotel() {
    deleteRooms();
}

void Hotel::deleteRooms() {
    for (Room* room : rooms) {
        delete room;
    }
}

int Hotel::registerCustomer(const std::string& name, const std::string& phone,
                            const std::string& address, bool premium) {
    const int id = nextId(customers.empty() ? 100 : customers.rbegin()->first);
    customers.emplace(id, makeCustomer(id, name, phone, address, premium));
    try {
        saveRecords();
    } catch (...) {
        customers.erase(id);
        throw;
    }
    return id;
}

const Customer& Hotel::getCustomer(int customerId) const {
    auto customer = customers.find(customerId);
    if (customer == customers.end()) {
        throw CustomerNotFound("Customer " + std::to_string(customerId) + " was not found.");
    }
    return *customer->second;
}

void Hotel::displayCustomers(std::ostream& output) const {
    displayRecords(customers, output, [&output](const auto& entry) {
        entry.second->displayCustomer(output);
        return true;
    });
}

Room* Hotel::roomAt(int roomNumber) const {
    const int floorIndex = roomNumber / 100 - 1;
    const int roomIndex = roomNumber % 100 - 1;
    if (floorIndex < 0 || floorIndex >= floorCount ||
        roomIndex < 0 || roomIndex >= roomsPerFloor) {
        throw InvalidRoom("Invalid room number " + std::to_string(roomNumber) + ".");
    }
    return rooms[floorIndex * roomsPerFloor + roomIndex];
}

const Room& Hotel::getRoom(int roomNumber) const {
    return *roomAt(roomNumber);
}

bool Hotel::isRoomAvailable(int roomNumber) const {
    roomAt(roomNumber);
    return roomAvailability[roomNumber / 100 - 1][roomNumber % 100 - 1] == 0;
}

void Hotel::setRoomOccupancy(int roomNumber, bool occupied) {
    Room* room = roomAt(roomNumber);
    room->setOccupied(occupied);
    roomAvailability[roomNumber / 100 - 1][roomNumber % 100 - 1] = occupied ? 1 : 0;
}

void Hotel::displayRoomAvailability(std::ostream& output, bool availableOnly) const {
    for (int floor = 0; floor < floorCount; ++floor) {
        output << "Floor " << floor + 1 << "\n";
        bool shown = false;
        for (int position = 0; position < roomsPerFloor; ++position) {
            const Room* room = rooms[floor * roomsPerFloor + position];
            const bool available = roomAvailability[floor][position] == 0;
            if (!availableOnly || available) {
                output << room->getRoomNumber() << " "
                       << (available ? "Available" : "Occupied") << " - "
                       << room->getRoomType() << " (Rs. " << std::fixed
                       << std::setprecision(2) << room->calculateRoomPrice(1) << "/day)\n";
                shown = true;
            }
        }
        if (!shown) {
            output << "No available rooms on this floor.\n";
        }
    }
}

void Hotel::requireAvailableRoom(int roomNumber) const {
    if (!isRoomAvailable(roomNumber)) {
        throw RoomUnavailable("Room " + std::to_string(roomNumber) + " is already booked.");
    }
}

int Hotel::findAvailableRoom(const std::string& roomType) const {
    if (roomType != "Standard" && roomType != "Deluxe" && roomType != "Suite") {
        throw InvalidRoom("Choose Standard, Deluxe, or Suite.");
    }
    for (const Room* room : rooms) {
        if (room->getRoomType() == roomType && isRoomAvailable(room->getRoomNumber())) {
            return room->getRoomNumber();
        }
    }
    throw RoomUnavailable("No " + roomType + " rooms are available.");
}

int Hotel::bookRoom(int customerId, int roomNumber, int days) {
    getCustomer(customerId);
    requireAvailableRoom(roomNumber);
    getRoom(roomNumber).calculateRoomPrice(days);
    const int id = nextId(bookings.empty() ? 1000 : bookings.back().getBookingId());
    bookings.emplace_back(id, customerId, roomNumber, days);
    try {
        saveRecords();
    } catch (...) {
        bookings.pop_back();
        throw;
    }
    setRoomOccupancy(roomNumber, true);
    return id;
}

int Hotel::bookRoom(int customerId, const std::string& roomType, int days) {
    getCustomer(customerId);
    return bookRoom(customerId, findAvailableRoom(roomType), days);
}

std::size_t Hotel::bookingIndex(int bookingId) const {
    auto booking = std::find_if(bookings.begin(), bookings.end(), [bookingId](const Booking& entry) {
        return entry.getBookingId() == bookingId;
    });
    if (booking == bookings.end()) {
        throw BookingNotFound("Booking " + std::to_string(bookingId) + " was not found.");
    }
    return static_cast<std::size_t>(booking - bookings.begin());
}

const Booking& Hotel::getBooking(int bookingId) const {
    return bookings[bookingIndex(bookingId)];
}

const Booking& Hotel::getActiveBooking(int bookingId) const {
    const Booking& booking = getBooking(bookingId);
    requireActiveBooking(booking);
    return booking;
}

void Hotel::displayBookings(std::ostream& output, bool activeOnly) const {
    displayRecords(bookings, output, [&](const Booking& booking) {
        if (activeOnly && !booking.isActive()) {
            return false;
        }
        const Customer& customer = getCustomer(booking.getCustomerId());
        output << booking.getBookingId() << " | " << customer.getName()
               << " (" << customer.getCategory() << ", ID " << customer.getCustomerId()
               << ") | Room " << booking.getRoomNumber() << " | "
               << booking.getNumberOfDays() << " day(s) | "
               << (booking.isActive() ? "Active" : "Completed") << '\n';
        return true;
    });
}

void Hotel::displayFoodMenu(std::ostream& output) const {
    output << std::fixed << std::setprecision(2);
    for (const FoodItem& item : foodMenu) {
        output << item.getItemId() << ". " << item.getItemName()
               << " - Rs. " << item.getPrice() << '\n';
    }
}

const FoodItem& Hotel::getFoodItem(int itemId) const {
    for (const FoodItem& item : foodMenu) {
        if (item.getItemId() == itemId) {
            return item;
        }
    }
    throw std::runtime_error("Invalid food item ID.");
}

double Hotel::orderFood(int bookingId, int itemId, int quantity) {
    Booking& booking = bookings[bookingIndex(bookingId)];
    requireActiveBooking(booking);
    booking.foodOrders.emplace_back(getFoodItem(itemId), quantity);
    try {
        const double subtotal = booking.getFoodSubtotal();
        if (!std::isfinite(subtotal)) {
            throw std::runtime_error("Food subtotal is too large.");
        }
        saveRecords();
        return subtotal;
    } catch (...) {
        booking.foodOrders.pop_back();
        throw;
    }
}

Bill Hotel::generateBill(int bookingId) const {
    const Booking& booking = getBooking(bookingId);
    if (!booking.isActive()) {
        return booking.getFinalBill();
    }
    return calculateBill(booking);
}

Bill Hotel::calculateBill(const Booking& booking) const {
    const Room& room = getRoom(booking.getRoomNumber());
    const Bill roomCharge(room.calculateRoomPrice(booking.getNumberOfDays()), 0.0);
    const Bill foodCharge(0.0, booking.getFoodSubtotal());
    const double rate = getCustomer(booking.getCustomerId()).getDiscountRate();
    return (roomCharge + foodCharge).withDiscount(rate);
}

void Hotel::displayBill(int bookingId, std::ostream& output) const {
    const Booking& booking = getBooking(bookingId);
    const Customer& customer = getCustomer(booking.getCustomerId());
    output << "\n" << (booking.isActive() ? "Bill preview" : "Final bill")
           << "\nCustomer: " << customer.getName() << " (" << customer.getCategory() << ")"
           << "\nCustomer ID: " << customer.getCustomerId()
           << "\nPhone: " << customer.getPhone() << "\nAddress: " << customer.getAddress()
           << "\nBooking ID: " << bookingId
           << "\nRoom: " << booking.getRoomNumber() << " - "
           << getRoom(booking.getRoomNumber()).getRoomType()
           << "\nDays: " << booking.getNumberOfDays()
           << "\nFood orders:\n" << std::fixed << std::setprecision(2);
    if (booking.getFoodOrders().empty()) {
        output << "  No food orders.\n";
    }
    for (const FoodOrder& order : booking.getFoodOrders()) {
        output << "  " << order.getItem().getItemName() << " x " << order.getQuantity()
               << " @ Rs. " << order.getItem().getPrice()
               << " = Rs. " << order.getTotal() << '\n';
    }
    displayBillSummary(output, generateBill(bookingId));
}

void Hotel::checkout(int bookingId) {
    Booking& booking = bookings[bookingIndex(bookingId)];
    requireActiveBooking(booking);
    booking.finalBill = calculateBill(booking);
    try {
        saveRecords();
    } catch (...) {
        booking.finalBill.reset();
        throw;
    }
    setRoomOccupancy(booking.getRoomNumber(), false);
}

void Hotel::displayBillingHistory(std::ostream& output) const {
    displayRecords(bookings, output, [&](const Booking& booking) {
        if (booking.isActive()) {
            return false;
        }
        displayBill(booking.getBookingId(), output);
        return true;
    });
}

// File handling

void requireRecordEnd(std::istream& record) {
    record >> std::ws;
    if (!record.eof()) {
        throw std::runtime_error("Unexpected fields at the end of a record.");
    }
}

void Hotel::writeRecords(std::ostream& output) const {
    output << "HOTEL 1\n" << std::setprecision(std::numeric_limits<double>::max_digits10);
    for (const auto& entry : customers) {
        const Customer& customer = *entry.second;
        output << "CUSTOMER " << entry.first << ' ' << customer.getCategory()
               << ' ' << std::quoted(customer.getName()) << ' ' << std::quoted(customer.getPhone())
               << ' ' << std::quoted(customer.getAddress()) << '\n';
    }
    for (const Booking& booking : bookings) {
        output << "BOOKING " << booking.getBookingId() << ' ' << booking.getCustomerId()
               << ' ' << booking.getRoomNumber() << ' ' << booking.getNumberOfDays()
               << ' ' << (booking.isActive() ? "Active" : "Completed")
               << ' ' << booking.getFoodOrders().size() << '\n';
        for (const FoodOrder& order : booking.getFoodOrders()) {
            const FoodItem& item = order.getItem();
            output << "FOOD " << item.getItemId() << ' ' << std::quoted(item.getItemName())
                   << ' ' << item.getPrice() << ' ' << order.getQuantity() << '\n';
        }
        if (!booking.isActive()) {
            const Bill& bill = booking.getFinalBill();
            output << "BILL " << bill.getRoomCharge() << ' ' << bill.getFoodCharge()
                   << ' ' << bill.getDiscountAmount() << '\n';
        }
    }
    output << "END\n";
}

void Hotel::saveRecords() const {
    if (recordsFile.has_parent_path()) {
        std::filesystem::create_directories(recordsFile.parent_path());
    }
    std::filesystem::path temporaryFile = recordsFile;
    temporaryFile += ".tmp";
    std::ofstream output(temporaryFile);
    if (!output) {
        throw std::runtime_error("Cannot open the temporary records file.");
    }
    try {
        writeRecords(output);
        output.close();
        if (!output) {
            throw std::runtime_error("Cannot write the records file.");
        }
        std::filesystem::rename(temporaryFile, recordsFile);
    } catch (...) {
        if (output.is_open()) {
            output.close();
        }
        std::error_code ignored;
        std::filesystem::remove(temporaryFile, ignored);
        throw;
    }
}

void Hotel::loadRecords() {
    std::filesystem::path source = recordsFile;
    // Older builds used the same format at this path.
    if (source == "hotel_data.txt" && !std::filesystem::exists(source)) {
        source = "data/records.txt";
    }
    const auto fileStatus = std::filesystem::status(source);
    if (!std::filesystem::exists(fileStatus)) {
        return;
    }
    if (!std::filesystem::is_regular_file(fileStatus)) {
        throw std::runtime_error("The records path must point to a regular file.");
    }
    std::ifstream input(source);
    if (!input) {
        throw std::runtime_error("Cannot read " + source.string() + ".");
    }

    int lineNumber = 0;
    auto readLine = [&]() {
        ++lineNumber;
        std::string line;
        if (!std::getline(input, line)) {
            throw std::runtime_error("Could not read the expected record.");
        }
        return std::istringstream(line);
    };

    try {
        auto header = readLine();
        std::string tag;
        int version;
        if (!(header >> tag >> version) || tag != "HOTEL" || version != 1) {
            throw std::runtime_error("Invalid file header.");
        }
        requireRecordEnd(header);

        while (true) {
            auto record = readLine();
            record >> tag;
            if (tag == "END" && record) {
                requireRecordEnd(record);
                input >> std::ws;
                if (!input.eof() || input.bad()) {
                    ++lineNumber;
                    throw std::runtime_error("Unexpected data after END.");
                }
                break;
            }
            if (tag == "CUSTOMER" && record) {
                int id;
                std::string category, name, phone, address;
                if (!(record >> id >> category >> std::quoted(name) >> std::quoted(phone)
                             >> std::quoted(address)) ||
                    id < 101 || (category != "Regular" && category != "Premium")) {
                    throw std::runtime_error("Invalid customer record.");
                }
                requireRecordEnd(record);
                if (customers.count(id) != 0) {
                    throw std::runtime_error("Duplicate customer ID.");
                }
                customers.emplace(id, makeCustomer(id, name, phone, address, category == "Premium"));
                continue;
            }

            int id, customerId, roomNumber, days, orderCount;
            std::string status;
            if (tag != "BOOKING" ||
                !(record >> id >> customerId >> roomNumber >> days >> status >> orderCount) ||
                id < 1001 || orderCount < 0 || (status != "Active" && status != "Completed")) {
                throw std::runtime_error("Invalid booking record.");
            }
            requireRecordEnd(record);
            if (!bookings.empty() && id <= bookings.back().getBookingId()) {
                throw std::runtime_error("Booking IDs must be unique and in increasing order.");
            }
            Booking booking(id, customerId, roomNumber, days);

            for (int index = 0; index < orderCount; ++index) {
                auto foodRecord = readLine();
                int itemId, quantity;
                std::string name;
                double price;
                if (!(foodRecord >> tag >> itemId >> std::quoted(name) >> price >> quantity) ||
                    tag != "FOOD" || name.empty()) {
                    throw std::runtime_error("Invalid food order record.");
                }
                requireRecordEnd(foodRecord);
                getFoodItem(itemId);
                booking.foodOrders.emplace_back(FoodItem(itemId, name, price), quantity);
            }
            const Bill expectedBill = calculateBill(booking);
            if (status == "Completed") {
                auto billRecord = readLine();
                double savedRoomCharge, savedFoodCharge, savedDiscount;
                if (!(billRecord >> tag >> savedRoomCharge >> savedFoodCharge >> savedDiscount) ||
                    tag != "BILL") {
                    throw std::runtime_error("Missing or invalid final bill.");
                }
                requireRecordEnd(billRecord);
                if (savedRoomCharge != expectedBill.getRoomCharge() ||
                    savedFoodCharge != expectedBill.getFoodCharge() ||
                    savedDiscount != expectedBill.getDiscountAmount()) {
                    throw std::runtime_error("Final bill does not match its booking.");
                }
                booking.finalBill.emplace(savedRoomCharge, savedFoodCharge, savedDiscount);
            } else {
                requireAvailableRoom(roomNumber);
                setRoomOccupancy(roomNumber, true);
            }
            bookings.push_back(std::move(booking));
        }
    } catch (const std::exception& error) {
        throw std::runtime_error(source.string() + ", line " + std::to_string(lineNumber) +
                                 ": " + error.what());
    }
}

// Console menu

namespace {
struct EndOfInput {};

std::string readLine(const char* prompt) {
    std::cout << prompt;
    std::string line;
    if (!std::getline(std::cin, line)) {
        if (std::cin.eof()) {
            throw EndOfInput{};
        }
        throw std::runtime_error("Could not read console input.");
    }
    return line;
}

int readNumber(const char* prompt) {
    std::istringstream input(readLine(prompt));
    int number;
    char extra;
    if (!(input >> number) || input >> extra) {
        throw std::runtime_error("Enter one whole number.");
    }
    return number;
}

void printMenu() {
    std::cout << "\nHotel and Resort Management\n"
              << "1. Register Customer\n"
              << "2. View Customers\n"
              << "3. View Room Availability\n"
              << "4. Book Room\n"
              << "5. View Active Bookings\n"
              << "6. Order Food\n"
              << "7. View Food Menu\n"
              << "8. Generate Bill\n"
              << "9. Checkout Customer\n"
              << "10. View Booking History\n"
              << "11. View Billing History\n"
              << "12. Save Records\n"
              << "0. Save and Exit\n";
}

void registerCustomer(Hotel& hotel) {
    const std::string name = readLine("Customer name: ");
    const std::string phone = readLine("Phone number (10 digits): ");
    const std::string address = readLine("Address: ");
    const int category = readNumber("Customer type (1 Regular, 2 Premium): ");
    if (category != 1 && category != 2) {
        throw std::runtime_error("Choose Regular (1) or Premium (2).");
    }
    const int id = hotel.registerCustomer(name, phone, address, category == 2);
    std::cout << "Customer registered. Customer ID: " << id << '\n';
}

void bookRoom(Hotel& hotel) {
    const int customerId = readNumber("Customer ID: ");
    hotel.getCustomer(customerId);
    hotel.displayRoomAvailability(std::cout, true);

    const int method = readNumber("Book by room number (1) or by type (2): ");
    int bookingId;
    if (method == 1) {
        const int roomNumber = readNumber("Room number: ");
        hotel.requireAvailableRoom(roomNumber);
        const int days = readNumber("Number of days: ");
        bookingId = hotel.bookRoom(customerId, roomNumber, days);
    } else if (method == 2) {
        const int type = readNumber("Room type (1 Standard, 2 Deluxe, 3 Suite): ");
        if (type < 1 || type > 3) {
            throw InvalidRoom("Choose Standard, Deluxe, or Suite.");
        }
        const std::string types[]{"Standard", "Deluxe", "Suite"};
        const std::string& roomType = types[type - 1];
        hotel.findAvailableRoom(roomType);
        const int days = readNumber("Number of days: ");
        bookingId = hotel.bookRoom(customerId, roomType, days);
    } else {
        throw std::runtime_error("Choose booking method 1 or 2.");
    }

    const Booking& booking = hotel.getBooking(bookingId);
    const Room& room = hotel.getRoom(booking.getRoomNumber());
    std::cout << "Room booked. Booking ID: " << bookingId
              << "\nRoom: " << room.getRoomNumber() << " - " << room.getRoomType()
              << "\nRoom charges: Rs. " << std::fixed << std::setprecision(2)
              << room.calculateRoomPrice(booking.getNumberOfDays()) << '\n';
}

void orderFood(Hotel& hotel) {
    const int bookingId = readNumber("Booking ID: ");
    hotel.getActiveBooking(bookingId);
    hotel.displayFoodMenu(std::cout);
    const int itemId = readNumber("Food item ID: ");
    const int quantity = readNumber("Quantity: ");
    const double subtotal = hotel.orderFood(bookingId, itemId, quantity);
    std::cout << "Food added. Food charges: Rs. " << std::fixed << std::setprecision(2)
              << subtotal << '\n';
}

void checkout(Hotel& hotel) {
    const int bookingId = readNumber("Booking ID: ");
    const int roomNumber = hotel.getActiveBooking(bookingId).getRoomNumber();
    hotel.displayBill(bookingId, std::cout);

    std::istringstream answer(readLine("Confirm payment (y/n): "));
    char choice, extra;
    if (!(answer >> choice) || answer >> extra ||
        (choice != 'y' && choice != 'Y' && choice != 'n' && choice != 'N')) {
        throw std::runtime_error("Enter y or n to confirm payment.");
    }
    if (choice == 'n' || choice == 'N') {
        std::cout << "Checkout cancelled.\n";
        return;
    }
    hotel.checkout(bookingId);
    std::cout << "Checkout successful. Room " << roomNumber
              << " is now available. Booking status: Completed.\n";
}

void runMenu(Hotel& hotel) {
    while (true) {
        printMenu();
        try {
            switch (readNumber("Enter your choice: ")) {
                case 1: registerCustomer(hotel); break;
                case 2: hotel.displayCustomers(std::cout); break;
                case 3: hotel.displayRoomAvailability(std::cout); break;
                case 4: bookRoom(hotel); break;
                case 5: hotel.displayBookings(std::cout, true); break;
                case 6: orderFood(hotel); break;
                case 7: hotel.displayFoodMenu(std::cout); break;
                case 8: hotel.displayBill(readNumber("Booking ID: "), std::cout); break;
                case 9: checkout(hotel); break;
                case 10: hotel.displayBookings(std::cout, false); break;
                case 11: hotel.displayBillingHistory(std::cout); break;
                case 12:
                    hotel.saveRecords();
                    std::cout << "Records saved.\n";
                    break;
                case 0:
                    hotel.saveRecords();
                    std::cout << "Records saved. Goodbye.\n";
                    return;
                default: std::cout << "Enter a listed menu option.\n";
            }
        } catch (const EndOfInput&) {
            std::cout << "\nEnd of input. Goodbye.\n";
            return;
        } catch (const std::exception& error) {
            std::cout << "Error: " << error.what() << '\n';
            if (!std::cin) {
                return;
            }
        }
    }
}
}

int main() {
    try {
        Hotel hotel;
        runMenu(hotel);
    } catch (const std::exception& error) {
        std::cerr << "Startup error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
