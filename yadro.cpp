#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <stdexcept>

using namespace std;

class ComputerClub {
public:
    struct Time {
        int hours;
        int minutes;
        
        Time() : hours(0), minutes(0) {}
        Time(int h, int m) : hours(h), minutes(m) {}
        
        bool operator<(const Time& other) const {
            return hours < other.hours || (hours == other.hours && minutes < other.minutes);
        }
        
        bool operator<=(const Time& other) const {
            return *this < other || *this == other;
        }
        
        bool operator>(const Time& other) const {
            return !(*this <= other);
        }
        
        bool operator>=(const Time& other) const {
            return !(*this < other);
        }
        
        bool operator==(const Time& other) const {
            return hours == other.hours && minutes == other.minutes;
        }
        
        Time operator+(const Time& other) const {
            int totalMinutes = toMinutes() + other.toMinutes();
            return fromMinutes(totalMinutes);
        }
        
        Time operator-(const Time& other) const {
            int totalMinutes = toMinutes() - other.toMinutes();
            return fromMinutes(totalMinutes);
        }
        
        int toMinutes() const {
            return hours * 60 + minutes;
        }
        
        static Time fromMinutes(int minutes) {
            minutes = max(0, minutes);
            return Time(minutes / 60, minutes % 60);
        }
        
        string toString() const {
            stringstream ss;
            ss << setw(2) << setfill('0') << hours << ":" 
               << setw(2) << setfill('0') << minutes;
            return ss.str();
        }
        
        static Time fromString(const string& timeStr) {
            size_t colonPos = timeStr.find(':');
            if (colonPos == string::npos || colonPos != 2 || timeStr.length() != 5) {
                throw invalid_argument("Invalid time format");
            }
            
            int h = stoi(timeStr.substr(0, 2));
            int m = stoi(timeStr.substr(3, 2));
            
            if (h < 0 || h > 23 || m < 0 || m > 59) {
                throw invalid_argument("Invalid time value");
            }
            
            return Time(h, m);
        }
    };
    
    struct Event {
        Time time;
        int id;
        string clientName;
        int tableNumber;
        string errorMessage;
        
        Event(const Time& t, int i, const string& name = "", int table = 0, const string& error = "")
            : time(t), id(i), clientName(name), tableNumber(table), errorMessage(error) {}
        
        string toString() const {
            stringstream ss;
            ss << time.toString() << " " << id;
            
            if (!clientName.empty()) {
                ss << " " << clientName;
            }
            
            if (tableNumber > 0) {
                ss << " " << tableNumber;
            }
            
            if (!errorMessage.empty()) {
                ss << " " << errorMessage;
            }
            
            return ss.str();
        }
    };
    
    struct TableInfo {
        int revenue = 0;
        Time busyTime;
        string currentClient;
        Time startTime;
    };

private:
    int tablesCount;
    Time openTime;
    Time closeTime;
    int hourCost;
    
    map<int, TableInfo> tables;
    set<string> clients;
    map<string, int> clientToTable;
    queue<string> waitingQueue;
    
    vector<Event> events;
    
    bool isClientInClub(const string& clientName) const {
        return clients.find(clientName) != clients.end();
    }
    
    bool isTableOccupied(int tableNumber) const {
        return tables.at(tableNumber).currentClient != "";
    }
    
    int countFreeTables() const {
        int count = 0;
        for (const auto& [num, info] : tables) {
            if (info.currentClient.empty()) {
                count++;
            }
        }
        return count;
    }
    
    void processSitEvent(const Time& time, const string& clientName, int tableNumber) {
        if (!isClientInClub(clientName)) {
            addErrorEvent(time, "ClientUnknown");
            return;
        }
        
        if (tableNumber < 1 || tableNumber > tablesCount) {
            addErrorEvent(time, "PlaceIsBusy");
            return;
        }
        
        if (isTableOccupied(tableNumber)) {
            addErrorEvent(time, "PlaceIsBusy");
            return;
        }
        
        if (clientToTable.count(clientName)) {
            int oldTable = clientToTable[clientName];
            tables[oldTable].currentClient = "";
        }
        
        tables[tableNumber].currentClient = clientName;
        clientToTable[clientName] = tableNumber;
        tables[tableNumber].startTime = time;
    }
    
    void processWaitingEvent(const Time& time, const string& clientName) {
        if (!isClientInClub(clientName)) {
            addErrorEvent(time, "ClientUnknown");
            return;
        }
        
        if (countFreeTables() > 0) {
            addErrorEvent(time, "ICanWaitNoLonger!");
            return;
        }
        
        if (waitingQueue.size() >= tablesCount) {
            clients.erase(clientName);
            addOutgoingEvent(Event(time, 11, clientName));
            return;
        }
        
        waitingQueue.push(clientName);
    }
    
    void processLeaveEvent(const Time& time, const string& clientName) {
        if (!isClientInClub(clientName)) {
            addErrorEvent(time, "ClientUnknown");
            return;
        }
        
        if (clientToTable.count(clientName)) {
            int tableNumber = clientToTable[clientName];
            Time startTime = tables[tableNumber].startTime;
            Time duration = time - startTime;
            
            int hours = duration.toMinutes() / 60;
            if (duration.toMinutes() % 60 != 0) {
                hours++;
            }
            
            tables[tableNumber].revenue += hours * hourCost;
            tables[tableNumber].busyTime = tables[tableNumber].busyTime + duration;
            
            clientToTable.erase(clientName);
            tables[tableNumber].currentClient = "";
            
            processNextClientFromQueue(time, tableNumber);
        }
        
        clients.erase(clientName);
    }
    
    void addErrorEvent(const Time& time, const string& errorMessage) {
        events.emplace_back(time, 13, "", 0, errorMessage);
    }
    
    void addOutgoingEvent(const Event& event) {
        events.push_back(event);
    }
    
    void processNextClientFromQueue(const Time& time, int freedTable) {
        if (!waitingQueue.empty()) {
            string nextClient = waitingQueue.front();
            waitingQueue.pop();
            
            tables[freedTable].currentClient = nextClient;
            clientToTable[nextClient] = freedTable;
            tables[freedTable].startTime = time;
            
            addOutgoingEvent(Event(time, 12, nextClient, freedTable));
        }
    }

public:
    ComputerClub(int tables, const Time& open, const Time& close, int cost)
        : tablesCount(tables), openTime(open), closeTime(close), hourCost(cost) {
        for (int i = 1; i <= tablesCount; ++i) {
            this->tables[i] = TableInfo();
        }
    }
    
    void processEvent(const Event& event) {
        events.push_back(event);
        
        switch (event.id) {
            case 1:
                if (event.time < openTime || event.time >= closeTime) {
                    addErrorEvent(event.time, "NotOpenYet");
                    break;
                }
                
                if (isClientInClub(event.clientName)) {
                    addErrorEvent(event.time, "YouShallNotPass");
                    break;
                }
                
                clients.insert(event.clientName);
                break;
            case 2:
                processSitEvent(event.time, event.clientName, event.tableNumber);
                break;
            case 3:
                processWaitingEvent(event.time, event.clientName);
                break;
            case 4:
                processLeaveEvent(event.time, event.clientName);
                break;
            default:
                throw invalid_argument("Unknown event ID");
        }
    }
    
    void closeClub() {
        vector<string> remainingClients(clients.begin(), clients.end());
        sort(remainingClients.begin(), remainingClients.end());
        
        for (const auto& client : remainingClients) {
            if (clientToTable.count(client)) {
                int tableNumber = clientToTable[client];
                Time startTime = tables[tableNumber].startTime;
                Time duration = closeTime - startTime;
                
                int hours = duration.toMinutes() / 60;
                if (duration.toMinutes() % 60 != 0) {
                    hours++;
                }
                
                tables[tableNumber].revenue += hours * hourCost;
                tables[tableNumber].busyTime = tables[tableNumber].busyTime + duration;
            }
            addOutgoingEvent(Event(closeTime, 11, client));
        }
    }
    
    void printResults() const {
        cout << openTime.toString() << endl;
        
        for (const auto& event : events) {
            cout << event.toString() << endl;
        }
        
        cout << closeTime.toString() << endl;
        
        for (int i = 1; i <= tablesCount; ++i) {
            const auto& table = tables.at(i);
            cout << i << " " << table.revenue << " " << table.busyTime.toString() << endl;
        }
    }
};

vector<string> split(const string& s) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (tokenStream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    ifstream inputFile(argv[1]);
    if (!inputFile) {
        cerr << "Error: Could not open input file" << endl;
        return 1;
    }

    try {
        string line;
        
        if (!getline(inputFile, line)) throw runtime_error("Missing number of tables");
        int tablesCount = stoi(line);
        if (tablesCount <= 0) throw runtime_error("Invalid number of tables");
        
        if (!getline(inputFile, line)) throw runtime_error("Missing working hours");
        auto timeTokens = split(line);
        if (timeTokens.size() != 2) throw runtime_error("Invalid working hours format");
        auto openTime = ComputerClub::Time::fromString(timeTokens[0]);
        auto closeTime = ComputerClub::Time::fromString(timeTokens[1]);
        if (closeTime <= openTime) throw runtime_error("Close time must be after open time");
        
        if (!getline(inputFile, line)) throw runtime_error("Missing hour cost");
        int hourCost = stoi(line);
        if (hourCost <= 0) throw runtime_error("Invalid hour cost");
        
        ComputerClub club(tablesCount, openTime, closeTime, hourCost);
        
        while (getline(inputFile, line)) {
            if (line.empty()) continue;
            
            auto tokens = split(line);
            if (tokens.size() < 2) throw runtime_error("Invalid event format");
            
            auto time = ComputerClub::Time::fromString(tokens[0]);
            int eventId = stoi(tokens[1]);
            
            switch (eventId) {
                case 1:
                    if (tokens.size() != 3) throw runtime_error("Invalid event 1 format");
                    club.processEvent(ComputerClub::Event(time, eventId, tokens[2]));
                    break;
                case 2:
                    if (tokens.size() != 4) throw runtime_error("Invalid event 2 format");
                    club.processEvent(ComputerClub::Event(time, eventId, tokens[2], stoi(tokens[3])));
                    break;
                case 3:
                    if (tokens.size() != 3) throw runtime_error("Invalid event 3 format");
                    club.processEvent(ComputerClub::Event(time, eventId, tokens[2]));
                    break;
                case 4:
                    if (tokens.size() != 3) throw runtime_error("Invalid event 4 format");
                    club.processEvent(ComputerClub::Event(time, eventId, tokens[2]));
                    break;
                default:
                    throw runtime_error("Unknown event ID");
            }
        }
        
        club.closeClub();
        club.printResults();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
