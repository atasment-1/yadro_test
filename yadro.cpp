#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>
#include <queue>
#include <iomanip>

using namespace std;

struct Time {
    int hours;
    int minutes;

    Time() : hours(0), minutes(0) {}
    Time(int h, int m) : hours(h), minutes(m) {}

    bool operator<(const Time& other) const {
        if (hours != other.hours) return hours < other.hours;
        return minutes < other.minutes;
    }

    bool operator<=(const Time& other) const {
        return !(other < *this);
    }

    Time operator+(const Time& other) const {
        int total_minutes = minutes + other.minutes;
        int extra_hours = total_minutes / 60;
        total_minutes %= 60;
        return Time(hours + other.hours + extra_hours, total_minutes);
    }

    Time operator-(const Time& other) const {
        int total_minutes = (hours * 60 + minutes) - (other.hours * 60 + other.minutes);
        if (total_minutes < 0) total_minutes = 0;
        return Time(total_minutes / 60, total_minutes % 60);
    }

    string toString() const {
        stringstream ss;
        ss << setw(2) << setfill('0') << hours << ":" 
           << setw(2) << setfill('0') << minutes;
        return ss.str();
    }
};

Time parseTime(const string& timeStr) {
    size_t colonPos = timeStr.find(':');
    if (colonPos == string::npos || colonPos == 0 || colonPos == timeStr.length() - 1) {
        throw invalid_argument("Invalid time format");
    }
    int hours = stoi(timeStr.substr(0, colonPos));
    int minutes = stoi(timeStr.substr(colonPos + 1));
    if (hours < 0 || hours >= 24 || minutes < 0 || minutes >= 60) {
        throw invalid_argument("Invalid time value");
    }
    return Time(hours, minutes);
}

class ComputerClub {
private:
    int tablesCount;
    Time openTime;
    Time closeTime;
    int hourCost;
    vector<bool> tablesOccupied;
    vector<int> tablesRevenue;
    vector<Time> tablesUsage;
    map<string, int> clientToTable;
    map<int, string> tableToClient;
    queue<string> waitingQueue;
    vector<string> outputEvents;

    void addOutputEvent(const Time& time, int eventId, const string& body) {
        stringstream ss;
        ss << time.toString() << " " << eventId << " " << body;
        outputEvents.push_back(ss.str());
    }

    void handleClientArrived(const Time& time, const string& clientName) {
        if (time < openTime || closeTime < time) {
            addOutputEvent(time, 13, "NotOpenYet");
            return;
        }
        if (clientToTable.find(clientName) != clientToTable.end()) {
            addOutputEvent(time, 13, "YouShallNotPass");
            return;
        }
        clientToTable[clientName] = -1;
    }

    void handleClientSat(const Time& time, const string& clientName, int tableNumber) {
        if (clientToTable.find(clientName) == clientToTable.end()) {
            addOutputEvent(time, 13, "ClientUnknown");
            return;
        }
        if (tableNumber < 1 || tableNumber > tablesCount) {
            addOutputEvent(time, 13, "PlaceIsBusy");
            return;
        }
        if (tablesOccupied[tableNumber - 1]) {
            addOutputEvent(time, 13, "PlaceIsBusy");
            return;
        }

        int prevTable = clientToTable[clientName];
        if (prevTable != -1) {
            tablesOccupied[prevTable - 1] = false;
            tableToClient.erase(prevTable);
            Time usageTime = time - tablesUsage[prevTable - 1];
            int hours = usageTime.hours;
            if (usageTime.minutes > 0 || hours == 0) hours++;
            tablesRevenue[prevTable - 1] += hours * hourCost;
        }

        tablesOccupied[tableNumber - 1] = true;
        clientToTable[clientName] = tableNumber;
        tableToClient[tableNumber] = clientName;
        tablesUsage[tableNumber - 1] = time;
    }

    void handleClientWaiting(const Time& time, const string& clientName) {
        if (clientToTable.find(clientName) == clientToTable.end()) {
            addOutputEvent(time, 13, "ClientUnknown");
            return;
        }

        for (int i = 0; i < tablesCount; ++i) {
            if (!tablesOccupied[i]) {
                addOutputEvent(time, 13, "ICanWaitNoLonger!");
                return;
            }
        }

        if (waitingQueue.size() >= tablesCount) {
            clientToTable.erase(clientName);
            addOutputEvent(time, 11, clientName);
            return;
        }

        waitingQueue.push(clientName);
    }

    void handleClientLeft(const Time& time, const string& clientName) {
        if (clientToTable.find(clientName) == clientToTable.end()) {
            addOutputEvent(time, 13, "ClientUnknown");
            return;
        }

        int tableNumber = clientToTable[clientName];
        clientToTable.erase(clientName);

        if (tableNumber != -1) {
            tablesOccupied[tableNumber - 1] = false;
            tableToClient.erase(tableNumber);
            Time usageTime = time - tablesUsage[tableNumber - 1];
            int hours = usageTime.hours;
            if (usageTime.minutes > 0 || hours == 0) hours++;
            tablesRevenue[tableNumber - 1] += hours * hourCost;

            if (!waitingQueue.empty()) {
                string nextClient = waitingQueue.front();
                waitingQueue.pop();
                tablesOccupied[tableNumber - 1] = true;
                clientToTable[nextClient] = tableNumber;
                tableToClient[tableNumber] = nextClient;
                tablesUsage[tableNumber - 1] = time;
                addOutputEvent(time, 12, nextClient + " " + to_string(tableNumber));
            }
        }
    }

public:
    ComputerClub(int count, const Time& open, const Time& close, int cost) 
        : tablesCount(count), openTime(open), closeTime(close), hourCost(cost),
          tablesOccupied(count, false), tablesRevenue(count, 0), tablesUsage(count) {}

    void processEvent(const Time& time, int eventId, const string& body) {
        stringstream ss(body);
        string clientName;
        int tableNumber;

        switch (eventId) {
            case 1:
                ss >> clientName;
                handleClientArrived(time, clientName);
                break;
            case 2:
                ss >> clientName >> tableNumber;
                handleClientSat(time, clientName, tableNumber);
                break;
            case 3:
                ss >> clientName;
                handleClientWaiting(time, clientName);
                break;
            case 4:
                ss >> clientName;
                handleClientLeft(time, clientName);
                break;
            default:
                break;
        }
    }

    void endOfDay() {
        vector<string> remainingClients;
        for (const auto& pair : clientToTable) {
            remainingClients.push_back(pair.first);
        }
        sort(remainingClients.begin(), remainingClients.end());

        for (const string& client : remainingClients) {
            addOutputEvent(closeTime, 11, client);
            int tableNumber = clientToTable[client];
            if (tableNumber != -1) {
                Time usageTime = closeTime - tablesUsage[tableNumber - 1];
                int hours = usageTime.hours;
                if (usageTime.minutes > 0 || hours == 0) hours++;
                tablesRevenue[tableNumber - 1] += hours * hourCost;
            }
        }
    }

    const vector<string>& getOutputEvents() const {
        return outputEvents;
    }

    void printResults() const {
        for (int i = 0; i < tablesCount; ++i) {
            Time totalUsage = tablesUsage[i];
            if (tablesOccupied[i]) {
                totalUsage = closeTime - tablesUsage[i];
            }
            cout << (i + 1) << " " << tablesRevenue[i] << " " << totalUsage.toString() << endl;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    ifstream inputFile(argv[1]);
    if (!inputFile.is_open()) {
        cerr << "Error opening file: " << argv[1] << endl;
        return 1;
    }

    try {
        int tablesCount;
        string line;
        if (!getline(inputFile, line)) throw runtime_error("Missing tables count");
        tablesCount = stoi(line);
        if (tablesCount <= 0) throw runtime_error("Invalid tables count");

        Time openTime, closeTime;
        if (!getline(inputFile, line)) throw runtime_error("Missing working hours");
        size_t spacePos = line.find(' ');
        if (spacePos == string::npos) throw runtime_error("Invalid working hours format");
        openTime = parseTime(line.substr(0, spacePos));
        closeTime = parseTime(line.substr(spacePos + 1));

        int hourCost;
        if (!getline(inputFile, line)) throw runtime_error("Missing hour cost");
        hourCost = stoi(line);
        if (hourCost <= 0) throw runtime_error("Invalid hour cost");

        ComputerClub club(tablesCount, openTime, closeTime, hourCost);

        cout << openTime.toString() << endl;

        while (getline(inputFile, line)) {
            if (line.empty()) continue;

            stringstream ss(line);
            string timeStr, eventIdStr, body;
            ss >> timeStr >> eventIdStr;
            getline(ss, body);
            if (!body.empty() && body[0] == ' ') body = body.substr(1);

            Time time = parseTime(timeStr);
            int eventId = stoi(eventIdStr);

            cout << line << endl;

            club.processEvent(time, eventId, body);
        }

        club.endOfDay();

        cout << closeTime.toString() << endl;

        club.printResults();

    } catch (const exception& e) {
        cerr << e.what() << endl;
        return 1;
    }

    return 0;
}
