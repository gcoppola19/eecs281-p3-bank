// Project Identifier: 292F24D17A4455C1B5133EDD8C7CEAA0C9570A98

#include <vector>
#include <iostream>
#include <algorithm> // std::sort
#include <getopt.h>
#include <string> // needed for VS
#include <fstream>
#include <queue>
#include <unordered_map>
#include <unordered_set>

using namespace std;

struct Transaction
{
    uint32_t number;
    uint64_t time;
    string ip;
    string sender;
    string recip;
    uint32_t amount;
    uint64_t exec;
    bool shared;
    uint32_t ID;
    uint32_t fee;
    Transaction(uint64_t time_, string ip_, string sender_, string recip_, uint32_t amount_, uint64_t exec_, bool shared_, uint32_t id_) : time(time_), ip(ip_), sender(sender_), recip(recip_), amount(amount_), exec(exec_), shared(shared_), ID(id_) {}
};
struct User
{
    string userName;
    uint64_t timestamp;
    uint64_t pin;
    uint32_t balance;
    unordered_set<string> ips;
    uint32_t transactionID;
    vector<Transaction *> incoming;
    vector<Transaction *> outgoing;
    User()
    {
        // bool in = false;
    }
    User(string user, uint64_t ts, uint64_t p, uint32_t b) : userName(user), timestamp(ts), pin(p), balance(b) {}
};

class Timestamp
{
public:
    bool operator()(const Transaction *a, const Transaction *b) const
    {
        if (a->exec == b->exec)
        {
            return a->ID > b->ID;
        }
        return a->exec > b->exec;
    }
};

uint64_t convertTime(string timeStamp)
{
    string temp;
    for (size_t i = 0; i < timeStamp.length(); ++i)
    {
        if (timeStamp[i] != ':')
        {
            temp += timeStamp[i];
        }
    }
    uint64_t time = static_cast<uint64_t>(stoul(temp));
    return time;
}

class Bank
{
public:
    void get_options(int argc, char **argv);
    void read_data(ifstream &fin);
    void read_commands();
    void execute(Transaction *transaction, unordered_map<string, User *>::iterator n, unordered_map<string, User *>::iterator n2);
    void doQueries();
    void clear();
    Bank()
    {
        verbose = false;
        count = 0;
    }

private:
    bool verbose;
    uint32_t count;

    priority_queue<Transaction *, vector<Transaction *>, Timestamp> transactions;
    unordered_map<string, User *> accounts;

    vector<Transaction *> completed;

    // unordered_set for IP addresses
};

uint32_t calculateFee(uint32_t amount, uint64_t senderTS, uint64_t execDate)
{
    uint32_t fee = 0;
    fee = static_cast<uint32_t>(amount * 0.01);
    if (fee < 10)
    {
        fee = 10;
    }
    if (fee > 450)
    {
        fee = 450;
    }
    if (execDate > senderTS + 50000000000)
    {
        fee = (fee * 3) / 4;
    }
    return fee;
}
// Read and process command line options.
void Bank::get_options(int argc, char **argv)
{

    opterr = false;
    int option_index = 0;
    int choice;

    option longOpts[] = {{"file", required_argument, nullptr, 'f'},
                         {"help", no_argument, nullptr, 'h'},
                         {"verbose", no_argument, nullptr, 'v'},
                         {nullptr, 0, nullptr, '\0'}};

    while ((choice = getopt_long(argc, argv, "f:hv", longOpts, &option_index)) != -1)
    {
        switch (choice)
        {
        case 'f':
        {
            string file{optarg};
            ifstream fin(file);
            if (!fin)
            {
                cerr << "Error opening file: " << file << "\n";
                exit(1);
            }
            read_data(fin);
            break;
        }
        case 'v':
            verbose = true;
            break;
        case 'h':
            std::cout << "This program reads in a file that contains registrations for a bank,\n"
                      << "then it receives commands for the bank operations and queries\n"
                      << " A user can login, log out, or place a transaction\n"
                      << "then the bank can further list transactions for a certain period\n"
                      << "calculate the revenue at the bank, view customer history, and summarize a day\n"
                      << "Usage: \'./bank\n\t[--verbose | -v]\n"
                      << "\t[--file | -f] <filename>\n"
                      << "\t[--help | -h]\n"
                      << std::endl;
            exit(0);
        default:
            cerr << "Error: invalid option"
                 << "\n";
            exit(1);
        }
    }
}

void Bank::read_data(ifstream &fin)
{
    string temp;
    while (fin.good())
    {
        User *user = new User;
        string userID;
        getline(fin, temp, '|');
        if (temp.empty())
        {
            delete user;
            user = nullptr;
            break;
        }
        user->timestamp = convertTime(temp);
        getline(fin, temp, '|');
        userID = temp;
        user->userName = temp;
        getline(fin, temp, '|');
        user->pin = static_cast<uint64_t>(stoul(temp));
        getline(fin, temp);
        user->balance = static_cast<uint32_t>(stoul(temp));
        accounts[userID] = user;
    }
}
void Bank::read_commands()
{
    string curr;
    string user;
    string IP;
    uint64_t ts = 0;
    uint64_t pinned;
    cin >> curr;
    // operations
    while (cin.good() && curr != "$$$")
    {
        if (cin.fail())
        {
            clear();
            cerr << "Error: Reading from cin has failed" << endl;
            exit(1);
        }
        switch (curr[0])
        {
        case '#':
            getline(cin, curr);
            break;
        case 'l':
        {
            cin >> user;
            cin >> pinned;
            cin >> IP;
            auto found = accounts.find(user);
            if (found == accounts.end())
            {
                if (verbose)
                {
                    cout << "Failed to log in " << user << ".\n";
                }
                break;
            }
            else
            {
                if (found->second->pin == pinned)
                {
                    found->second->ips.insert(IP);
                    if (verbose)
                    {
                        cout << "User " << user << " logged in."
                             << "\n";
                    }
                    break;
                }
                if (verbose)
                {
                    cout << "Failed to log in " << user << ".\n";
                }
            }
            break;
        }
        case 'o':
        {
            cin >> user;
            cin >> IP;
            auto found2 = accounts.find(user);
            if (found2 == accounts.end())
            {
                if (verbose)
                {
                    cout << "Failed to log out " << user << ".\n";
                }
                break;
            }
            else
            {
                if (found2->second->ips.size() > 0)
                {

                    if (found2->second->ips.find(IP) != found2->second->ips.end())
                    {
                        found2->second->ips.erase(IP);
                        if (verbose)
                        {
                            cout << "User " << user << " logged out.\n";
                        }
                        break;
                    }
                    else
                    {
                        if (verbose)
                        {
                            cout << "Failed to log out " << user << ".\n";
                        }
                        break;
                    }
                }
                else
                {
                    if (verbose)
                    {
                        cout << "Failed to log out " << user << ".\n";
                    }
                    break;
                }
            }
            break;
        }
        case 'p':
        {
            string timestamp;
            cin >> timestamp;
            uint64_t prevTs = ts;
            ts = convertTime(timestamp);
            if (ts < prevTs)
            {
                clear();
                cerr << "your timestamp is earlier than previous timestamp";
                exit(1);
            }
            string ip;
            cin >> ip;
            string sender;
            cin >> sender;
            string recip;
            cin >> recip;
            string transAmount;
            cin >> transAmount;
            uint32_t amount = static_cast<uint32_t>(stoul(transAmount));
            string execDate;
            cin >> execDate;
            uint64_t exec = convertTime(execDate);
            if (exec < ts)
            {
                clear();
                cerr << "your execution date is before timestamp";
                exit(1);
            }
            uint64_t time = exec - ts;
            string fee;
            cin >> fee;
            unordered_map<string, User *>::iterator n2;
            unordered_map<string, User *>::iterator n;
            if (time > 3 * 1000000)
            {
                if (verbose)
                {
                    cout << "Select a time less than three days in the future."
                         << "\n";
                }
                break;
            }

            auto found = accounts.find(sender);
            auto found2 = accounts.find(recip);
            if (found == accounts.end())
            {
                if (verbose)
                {
                    cout << "Sender " << sender << " does not exist."
                         << "\n";
                }
                break;
            }
            else if (found2 == accounts.end())
            {
                if (verbose)
                {
                    cout << "Recipient " << recip << " does not exist."
                         << "\n";
                }
                break;
            }
            else
            {
                n = found;
                n2 = found2;

                if (found->second->timestamp > exec || found2->second->timestamp > exec)
                {
                    if (verbose)
                    {
                        cout << "At the time of execution, sender and/or recipient have not registered.\n";
                    }
                    break;
                }
                if (found->second->ips.size() < 1)
                {
                    if (verbose)
                    {
                        cout << "Sender " << sender << " is not logged in.\n";
                    }
                    break;
                }
                if (found->second->ips.find(ip) == found->second->ips.end())
                {
                    if (verbose)
                    {
                        cout << "Fraudulent transaction detected, aborting request.\n";
                    }
                    break;
                }
            }

            bool shared = false;
            if (fee[0] == 's')
            {
                shared = true;
            }

            while (!transactions.empty() && ts >= transactions.top()->exec)
            {
                // execute
                execute(transactions.top(), accounts.find(transactions.top()->sender), accounts.find(transactions.top()->recip));
                // print execute after
                transactions.pop();
            }
            Transaction *transaction = new Transaction{ts, ip, sender, recip, amount, exec, shared, count++};
            transactions.push(transaction);
            if (verbose)
            {
                cout << "Transaction placed at " << ts << ": $" << amount << " from " << sender << " to " << recip << " at " << exec << ".\n";
            }

            break;
        }
        default:
            cout << "invalid option"
                 << "\n";
            break;
        }

        cin >> curr;
    }
    // make sure to see if transactions PQ is empty and execute transactions before queries
    while (!transactions.empty())
    {
        execute(transactions.top(), accounts.find(transactions.top()->sender), accounts.find(transactions.top()->recip));
        transactions.pop();
    }
    doQueries();
}

void Bank::execute(Transaction *transaction, unordered_map<string, User *>::iterator n, unordered_map<string, User *>::iterator n2)
{
    uint32_t amount = transaction->amount;
    uint64_t senderBal = n->second->balance;
    uint64_t recipBal = n2->second->balance;

    uint32_t fee2 = calculateFee(amount, n->second->timestamp, transaction->exec);
    transaction->fee = fee2;
    if (transaction->shared)
    {
        uint32_t senderFee = 0;
        uint32_t recipFee = 0;
        if (fee2 % 2 != 0)
        {
            senderFee = (fee2 / 2) + 1;
            recipFee = (fee2 / 2);
            if (senderBal < amount + senderFee || recipBal < recipFee)
            {
                if (verbose)
                {
                    cout << "Insufficient funds to process transaction " << transaction->ID << ".\n";
                }
            }
            else
            {

                n->second->balance -= amount + senderFee;
                n2->second->balance += amount;
                n2->second->balance -= recipFee;
                completed.push_back(transaction);
                n->second->outgoing.push_back(transaction);
                n2->second->incoming.push_back(transaction);
                if (verbose)
                {
                    cout << "Transaction executed at " << transaction->exec << ": $" << transaction->amount << " from " << transaction->sender << " to " << transaction->recip << ".\n";
                }
            }
        }
        else
        {
            if (senderBal < amount + (fee2 / 2) || recipBal < (fee2 / 2))
            {
                if (verbose)
                {
                    cout << "Insufficient funds to process transaction " << transaction->ID << ".\n";
                }
            }
            else
            {
                n->second->balance -= amount + (fee2 / 2);
                n2->second->balance += amount;
                n2->second->balance -= fee2 / 2;
                completed.push_back(transaction);
                n->second->outgoing.push_back(transaction);
                n2->second->incoming.push_back(transaction);
                if (verbose)
                {
                    cout << "Transaction executed at " << transaction->exec << ": $" << transaction->amount << " from " << transaction->sender << " to " << transaction->recip << ".\n";
                }
            }
        }
    }
    else
    {
        if (senderBal < amount + fee2)
        {
            if (verbose)
            {
                cout << "Insufficient funds to process transaction " << transaction->ID << ".\n";
            }
        }
        else
        {

            n->second->balance -= amount + fee2;
            n2->second->balance += amount;
            completed.push_back(transaction);
            n->second->outgoing.push_back(transaction);
            n2->second->incoming.push_back(transaction);
            if (verbose)
            {
                cout << "Transaction executed at " << transaction->exec << ": $" << transaction->amount << " from " << transaction->sender << " to " << transaction->recip << ".\n";
            }
        }
    }
}

void Bank::doQueries()
{
    string curr;
    cin >> curr;
    while (cin.good())
    {
        if (cin.fail())
        {
            clear();
            cerr << "Error: Reading from cin has failed" << endl;
            exit(1);
        }

        switch (curr[0])
        {
        case 'l':
        {
            string x;
            string y;
            cin >> x;
            cin >> y;
            uint64_t end = 0;
            if (y.empty())
            {
                end = static_cast<uint64_t>(completed[completed.size() - 1]->exec);
            }
            uint64_t start = convertTime(x);
            end = convertTime(y);

            auto lower = std::lower_bound(completed.begin(), completed.end(), start,
                                          [](Transaction *a, uint64_t targetTime)
                                          {
                                              return a->exec < targetTime;
                                          });
       
            auto upper = std::upper_bound(completed.begin(), completed.end(), end,
                                          [](uint64_t targetTime, Transaction *a)
                                          {
                                              return targetTime <= a->exec;
                                          });
          
            for (auto it = lower; it != upper; ++it)
            {
                Transaction *currentTransaction = *it;
                if (currentTransaction->amount == 1)
                {
                    cout << currentTransaction->ID << ": " << currentTransaction->sender << " sent " << currentTransaction->amount << " dollar to " << currentTransaction->recip << " at " << currentTransaction->exec << ".\n";
                }
                else
                {
                    cout << currentTransaction->ID << ": " << currentTransaction->sender << " sent " << currentTransaction->amount << " dollars to " << currentTransaction->recip << " at " << currentTransaction->exec << ".\n";
                }
            }
            if (lower == upper)
            {
                cout << "There were 0 transactions that were placed between time " << start << " to " << end << ".\n";
            }
            else
            {
                cout << "There ";
                if (distance(lower, upper) == 1)
                {
                    cout << "was 1 transaction that was placed between time ";
                }
                else
                {
                    cout << "were " << distance(lower, upper) << " transactions that were placed between time ";
                }
                cout << start << " to " << end << ".\n";
                
            }
           
        }
        case 'r':
        {
            string x;
            string y;
            cin >> x;
            cin >> y;
            uint64_t start = convertTime(x);
            uint64_t end = convertTime(y);
            uint32_t rev = 0;
            // raneg is wrong
            auto lower = std::lower_bound(completed.begin(), completed.end(), start,
                                          [](Transaction *a, uint64_t targetTime)
                                          {
                                              return a->exec < targetTime;
                                          });

            /*auto upper = std::upper_bound(completed.begin(), completed.end(), end,
                                          [](uint64_t targetTime, Transaction *a)
                                          {
                                              return targetTime < a->exec;
                                         });*/
            while ((lower != completed.end()) && (*lower)->exec < end)
            {
                rev += (*lower)->fee;
                lower++;
            }

            // fees are off by 3
            uint64_t time = end - start;

            uint32_t years = static_cast<uint32_t>((time % 1000000000000ull) / 10000000000ull);

            uint32_t months = static_cast<uint32_t>((time % 10000000000ull) / 100000000ull);

            uint32_t days = static_cast<uint32_t>((time % 100000000ull) / 1000000ull);

            uint32_t hours = static_cast<uint32_t>((time % 1000000ull) / 10000ull);

            uint32_t minutes = static_cast<uint32_t>((time % 10000ull) / 100ull);
            uint32_t seconds = static_cast<uint32_t>(time % 100ull);
            cout << "281Bank has collected " << rev << " dollars in fees over";
            if (years == 1)
            {
                cout << " " << years << " year";
            }
            else if (years > 1)
            {
                cout << " " << years << " years";
            }
            if (months == 1)
            {
                cout << " " << months << " month";
            }
            else if (months > 1)
            {
                cout << " " << months << " months";
            }
            if (days == 1)
            {
                cout << " " << days << " day";
            }
            else if (days > 1)
            {
                cout << " " << days << " days";
            }
            if (hours == 1)
            {
                cout << " " << hours << " hour";
            }
            else if (hours > 1)
            {
                cout << " " << hours << " hours";
            }
            if (minutes == 1)
            {
                cout << " " << minutes << " minute";
            }
            if (minutes > 1)
            {
                cout << " " << minutes << " minutes";
            }
            if (seconds == 1)
            {
                cout << " " << seconds << " second";
            }
            else if (seconds > 1)
            {
                cout << " " << seconds << " seconds";
            }
            cout << ".\n";
        }
        break;
        /*case 'h':
        {
            string user;
            cin >> user;
            auto find = accounts.find(user);
            if (find == accounts.end())
            {
                cout << "User " << user << " does not exist.\n";
                break;
            }
            else
            {
                // fix to print the last numPrint things
                cout << "Customer " << user << " account summary:\n";
                cout << "Balance: $" << find->second->balance << "\n";
                size_t size = 0;
                size += find->second->incoming.size();
                size += find->second->outgoing.size();
                cout << "Total # of transactions: " << size << "\n";
                cout << "Incoming " << find->second->incoming.size() << ":\n";
                size_t numPrintInc = std::min(static_cast<size_t>(10), find->second->incoming.size());
                size_t numPrintOut = std::min(static_cast<size_t>(10), find->second->outgoing.size());

                for (size_t i = 0; i < numPrintInc; ++i)
                {
                    if (find->second->incoming[i]->amount == 1)
                    {
                        cout << find->second->incoming[i]->ID << ": " << find->second->incoming[i]->sender << " sent " << find->second->incoming[i]->amount << " dollar to "
                             << find->second->incoming[i]->recip << " at " << find->second->incoming[i]->exec << ".\n";
                    }
                    else
                    {
                        cout << find->second->incoming[i]->ID << ": " << find->second->incoming[i]->sender << " sent " << find->second->incoming[i]->amount << " dollars to "
                             << find->second->incoming[i]->recip << " at " << find->second->incoming[i]->exec << ".\n";
                    }
                }
                cout << "Outgoing " << find->second->outgoing.size() << ":\n";
                for (size_t i = 0; i < numPrintOut; ++i)
                {
                    if (find->second->outgoing[i]->amount == 1)
                    {
                        cout << find->second->outgoing[i]->ID << ": " << find->second->outgoing[i]->sender << " sent " << find->second->outgoing[i]->amount << " dollar to "
                             << find->second->outgoing[i]->recip << " at " << find->second->outgoing[i]->exec << ".\n";
                    }
                    else
                    {
                        cout << find->second->outgoing[i]->ID << ": " << find->second->outgoing[i]->sender << " sent " << find->second->outgoing[i]->amount << " dollars to "
                             << find->second->outgoing[i]->recip << " at " << find->second->outgoing[i]->exec << ".\n";
                    }
                }
            }
            break;
        }*/
        case 'h':
        {
            string user;
            cin >> user;
            auto find = accounts.find(user);
            if (find == accounts.end())
            {
                cout << "User " << user << " does not exist.\n";
                break;
            }
            else
            {
                cout << "Customer " << user << " account summary:\n";
                cout << "Balance: $" << find->second->balance << "\n";
                size_t totalTransactions = find->second->incoming.size() + find->second->outgoing.size();
                cout << "Total # of transactions: " << totalTransactions << "\n";

                // Get the last 10 transactions, starting with the earliest among the latest transactions
                // size_t numPrint = std::min(static_cast<size_t>(10), totalTransactions);
                // size_t incIndex = find->second->incoming.size();
                // size_t outIndex = find->second->outgoing.size();
                size_t totalIncoming = find->second->incoming.size();
                size_t totalOutgoing = find->second->outgoing.size();

                cout << "Incoming " << totalIncoming << ":\n";
                size_t numPrintInc = min(static_cast<size_t>(10), totalIncoming);
                for (size_t i = totalIncoming - numPrintInc; i < totalIncoming; ++i)
                {
                    Transaction *transaction = find->second->incoming[i];
                    cout << transaction->ID << ": " << transaction->sender << " sent " << transaction->amount << " dollar" << (transaction->amount == 1 ? "" : "s")
                         << " to " << transaction->recip << " at " << transaction->exec << ".\n";
                }

                cout << "Outgoing " << totalOutgoing << ":\n";
                size_t numPrintOut = min(static_cast<size_t>(10), totalOutgoing);
                for (size_t i = totalOutgoing - numPrintOut; i < totalOutgoing; ++i)
                {
                    Transaction *transaction = find->second->outgoing[i];
                    cout << transaction->ID << ": " << transaction->sender << " sent " << transaction->amount << " dollar" << (transaction->amount == 1 ? "" : "s")
                         << " to " << transaction->recip << " at " << transaction->exec << ".\n";
                }

                /*for (size_t i = 0; i < numPrint; ++i)
                {
                    if (incIndex > 0 && outIndex > 0)
                    {
                        if (find->second->incoming[incIndex - 1]->exec > find->second->outgoing[outIndex - 1]->exec)
                        {
                            Transaction *transaction = find->second->incoming[--incIndex];
                            cout << transaction->ID << ": " << transaction->sender << " sent " << transaction->amount << " dollar" << (transaction->amount == 1 ? "" : "s")
                                 << " to " << transaction->recip << " at " << transaction->exec << ".\n";
                        }
                        else
                        {
                            Transaction *transaction = find->second->outgoing[--outIndex];
                            cout << transaction->ID << ": " << transaction->sender << " sent " << transaction->amount << " dollar" << (transaction->amount == 1 ? "" : "s")
                                 << " to " << transaction->recip << " at " << transaction->exec << ".\n";
                        }
                    }
                    else if (incIndex > 0)
                    {
                        Transaction *transaction = find->second->incoming[--incIndex];
                        cout << transaction->ID << ": " << transaction->sender << " sent " << transaction->amount << " dollar" << (transaction->amount == 1 ? "" : "s")
                             << " to " << transaction->recip << " at " << transaction->exec << ".\n";
                    }
                    else if (outIndex > 0)
                    {
                        Transaction *transaction = find->second->outgoing[--outIndex];
                        cout << transaction->ID << ": " << transaction->sender << " sent " << transaction->amount << " dollar" << (transaction->amount == 1 ? "" : "s")
                             << " to " << transaction->recip << " at " << transaction->exec << ".\n";
                    }
                }*/
            }
            break;
        }

        case 's':
            uint32_t ta = 0;
            uint32_t fee = 0;
            string time;
            cin >> time;
            uint64_t ts = convertTime(time);
            uint64_t year = ts / 10000000000;
            uint64_t month = (ts / 100000000) % 100;
            uint64_t day = (ts / 1000000) % 100;
            uint64_t modifiedTime = static_cast<uint64_t>(year * 10000000000 + month * 100000000 + day * 1000000);
            uint64_t end = modifiedTime + 1000000;
            cout << "Summary of [" << modifiedTime << ", " << end << "):\n";

            auto lower = std::lower_bound(completed.begin(), completed.end(), modifiedTime,
                                          [](Transaction *a, uint64_t targetTime)
                                          {
                                              return a->exec <= targetTime;
                                          });

            auto upper = std::upper_bound(completed.begin(), completed.end(), end,
                                          [](uint64_t targetTime, Transaction *a)
                                          {
                                              return targetTime < a->exec;
                                          });
            if (lower == completed.end() || upper == completed.begin())
            {
                cout << "There were a total of " << ta << " transactions, 281Bank has collected " << fee << " dollars in fees.\n";
                break;
            }
            // Decrement the upper iterator to get the last element before the end time
            if (upper != completed.begin())
            {
                --upper;
            }

            for (auto it = lower; it != upper + 1; ++it)
            {

                Transaction *currentTransaction = *it;
                if (currentTransaction->amount == 1)
                {
                    cout << currentTransaction->ID << ": " << currentTransaction->sender << " sent " << currentTransaction->amount << " dollar to " << currentTransaction->recip << " at " << currentTransaction->exec << ".\n";
                }
                else
                {
                    cout << currentTransaction->ID << ": " << currentTransaction->sender << " sent " << currentTransaction->amount << " dollars to " << currentTransaction->recip << " at " << currentTransaction->exec << ".\n";
                }
                fee += (*it)->fee;
                ta++;
                // Sum up fees within the range
            }

            if (ta == 1)
            {
                cout << "There was a total of " << ta << " transaction, 281Bank has collected " << fee << " dollars in fees.\n";
            }
            else
            {
                cout << "There were a total of " << ta << " transactions, 281Bank has collected " << fee << " dollars in fees.\n";
            }
            break;
        }
        cin >> curr;
    }
}

void Bank::clear()
{
    while (!transactions.empty())
    {
        delete transactions.top();
        transactions.pop();
    }
    for (auto &pair : accounts)
    {
        delete pair.second;
        pair.second = nullptr;
    }
    accounts.clear();
    for (auto &element : completed)
    {
        delete element;
        element = nullptr;
    }
    completed.clear();
}
int main(int argc, char **argv)
{
    ios_base::sync_with_stdio(false);

    Bank bank;
    // Read and process the command line options.
    bank.get_options(argc, argv);
    bank.read_commands();

    bank.clear();

    return 0;
}
