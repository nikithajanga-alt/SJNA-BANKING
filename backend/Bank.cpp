
#include "Bank.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>

// FIND ACCOUNT
BankAccount* Bank::findAccount(string number)
{
    for (size_t i = 0; i < accounts.size(); i++)
    {
        if (accounts[i].getAccountNumber() == number)
            return &accounts[i];
    }

    return NULL;
}

// REGISTER ACCOUNT
bool Bank::registerAccount(string number, string name,
                           string pin, string accountType)
{
    if (number.empty() || name.empty() || pin.length() != 4)
        return false;

    if (accountType != "Savings" &&
        accountType != "Current")
        return false;

    for (size_t i = 0; i < pin.length(); i++)
    {
        if (pin[i] < '0' || pin[i] > '9')
            return false;
    }

    if (findAccount(number) != NULL)
        return false;

    accounts.push_back(
        BankAccount(number, name, pin, 0.0, accountType)
    );

    if (!saveAccounts())
    {
        accounts.pop_back();
        return false;
    }

    saveTransaction(
        "ACCOUNT CREATED | " + number + " | " +
        name + " | " + accountType
    );

    return true;
}

// LOGIN
bool Bank::login(string number, string pin)
{
    BankAccount* account = findAccount(number);

    return account != NULL && account->checkPin(pin);
}

// GET ACCOUNT DETAILS
bool Bank::getAccountDetails(string number,
                             string& name, double& balance)
{
    BankAccount* account = findAccount(number);

    if (account == NULL)
        return false;

    name = account->getAccountHolder();
    balance = account->getBalance();

    return true;
}

// GET ACCOUNT TYPE
bool Bank::getAccountType(string number, string& accountType)
{
    BankAccount* account = findAccount(number);

    if (account == NULL)
        return false;

    accountType = account->getAccountType();

    return true;
}

// CHECK BALANCE
double Bank::checkBalance(string number)
{
    BankAccount* account = findAccount(number);

    if (account == NULL)
        return -1.0;

    return account->getBalance();
}

// DEPOSIT MONEY
bool Bank::depositMoney(string number, double amount)
{
    BankAccount* account = findAccount(number);

    if (account == NULL || !std::isfinite(amount) || amount <= 0)
        return false;

    double oldBalance = account->getBalance();

    account->deposit(amount);

    if (!saveAccounts())
    {
        account->setBalance(oldBalance);
        return false;
    }

    ostringstream line;

    line << fixed << setprecision(2)
         << "DEPOSIT | " << number << " | " << amount;

    saveTransaction(line.str());

    return true;
}

// WITHDRAW MONEY
bool Bank::withdrawMoney(string number, double amount)
{
    BankAccount* account = findAccount(number);

    if (account == NULL || !std::isfinite(amount) || amount <= 0)
        return false;

    double oldBalance = account->getBalance();

    if (!account->withdraw(amount))
        return false;

    if (!saveAccounts())
    {
        account->setBalance(oldBalance);
        return false;
    }

    ostringstream line;

    line << fixed << setprecision(2)
         << "WITHDRAWAL | " << number << " | " << amount;

    saveTransaction(line.str());

    return true;
}

// TRANSFER MONEY
bool Bank::transferMoney(string sender, string receiver, double amount)
{
    if (sender == receiver || !std::isfinite(amount) || amount <= 0)
        return false;

    BankAccount* from = findAccount(sender);
    BankAccount* to = findAccount(receiver);

    if (from == NULL || to == NULL)
        return false;

    double senderOldBalance = from->getBalance();
    double receiverOldBalance = to->getBalance();

    if (!from->withdraw(amount))
        return false;

    to->deposit(amount);

    if (!saveAccounts())
    {
        from->setBalance(senderOldBalance);
        to->setBalance(receiverOldBalance);
        return false;
    }

    ostringstream line;

    line << fixed << setprecision(2)
         << "TRANSFER | FROM " << sender
         << " | TO " << receiver
         << " | AMOUNT " << amount;

    saveTransaction(line.str());

    return true;
}

// LOAD ACCOUNTS
void Bank::loadAccounts()
{
    accounts.clear();

    ifstream file("data/accounts.txt");

    if (!file)
        return;

    string line;

    while (getline(file, line))
    {
        if (line.empty())
            continue;

        string number, name, pin, balanceText;
        string accountType = "Savings";

        size_t first = line.find('|');

        if (first != string::npos)
        {
            size_t second = line.find('|', first + 1);

            if (second == string::npos)
                continue;

            size_t third = line.find('|', second + 1);

            if (third == string::npos)
                continue;

            size_t fourth = line.find('|', third + 1);

            number = line.substr(0, first);

            name = line.substr(
                first + 1, second - first - 1
            );

            pin = line.substr(
                second + 1, third - second - 1
            );

            if (fourth != string::npos)
            {
                balanceText = line.substr(
                    third + 1, fourth - third - 1
                );

                accountType = line.substr(fourth + 1);
            }
            else
            {
                balanceText = line.substr(third + 1);
            }
        }
        else
        {
            istringstream oldFormat(line);

            if (!(oldFormat >> number >> name >> pin >> balanceText))
                continue;
        }

        if (accountType != "Savings" &&
            accountType != "Current")
            continue;

        try
        {
            size_t used = 0;

            double amount = stod(balanceText, &used);

            if (used != balanceText.length() ||
                !std::isfinite(amount) || amount < 0)
                continue;

            accounts.push_back(
                BankAccount(number, name, pin, amount, accountType)
            );
        }
        catch (...)
        {
            continue;
        }
    }
}

// SAVE ACCOUNTS
bool Bank::saveAccounts()
{
    ofstream file("data/accounts.txt", ios::trunc);

    if (!file)
        return false;

    file << fixed << setprecision(2);

    for (size_t i = 0; i < accounts.size(); i++)
    {
        file << accounts[i].getAccountNumber() << "|"
             << accounts[i].getAccountHolder() << "|"
             << accounts[i].getPin() << "|"
             << accounts[i].getBalance() << "|"
             << accounts[i].getAccountType() << "\n";
    }

    file.flush();

    return file.good();
}

// SAVE TRANSACTION
void Bank::saveTransaction(string details)
{
    ofstream file("data/transactions.txt", ios::app);

    if (!file)
        return;

    time_t now = time(NULL);

    file << (long long)now << " | "
         << details << "\n";
}

// GET TRANSACTION HISTORY
string Bank::getTransactions(string number)
{
    ifstream file("data/transactions.txt");

    string line;
    string result;

    while (getline(file, line))
    {
        // CHECK IF THIS IS A TRANSFER RECORD
        size_t transferPos = line.find("TRANSFER | FROM ");

        if (transferPos != string::npos)
        {
            size_t senderStart = transferPos +
                                 string("TRANSFER | FROM ").length();

            size_t receiverMarker = line.find(" | TO ", senderStart);

            if (receiverMarker == string::npos)
                continue;

            size_t amountMarker = line.find(" | AMOUNT ", receiverMarker);

            if (amountMarker == string::npos)
                continue;

            string sender = line.substr(
                senderStart,
                receiverMarker - senderStart
            );

            size_t receiverStart = receiverMarker +
                                   string(" | TO ").length();

            string receiver = line.substr(
                receiverStart,
                amountMarker - receiverStart
            );

            string amount = line.substr(
                amountMarker + string(" | AMOUNT ").length()
            );

            string timestamp = line.substr(0, transferPos);

            // SENDER'S HISTORY
            if (number == sender)
            {
                result += timestamp +
                          "TRANSFER SENT | TO " +
                          receiver +
                          " | AMOUNT " +
                          amount + "\n";
            }
            // RECEIVER'S HISTORY
            else if (number == receiver)
            {
                result += timestamp +
                          "TRANSFER RECEIVED | FROM " +
                          sender +
                          " | AMOUNT " +
                          amount + "\n";
            }

            continue;
        }

        // NORMAL TRANSACTION HISTORY
        size_t position = line.find(number);

        while (position != string::npos)
        {
            bool leftBoundary =
                position == 0 ||
                line[position - 1] == ' ' ||
                line[position - 1] == '|';

            size_t end = position + number.length();

            bool rightBoundary =
                end == line.length() ||
                line[end] == ' ' ||
                line[end] == '|';

            if (leftBoundary && rightBoundary)
            {
                result += line + "\n";
                break;
            }

            position = line.find(number, position + 1);
        }
    }

    return result;
}

// GET ALL ACCOUNTS
vector<BankAccount> Bank::getAccounts()
{
    return accounts;
}
