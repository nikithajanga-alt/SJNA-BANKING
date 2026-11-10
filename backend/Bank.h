
#ifndef BANK_H
#define BANK_H

#include "BankAccount.h"
#include <vector>
#include <string>

using namespace std;

class Bank
{
private:
    vector<BankAccount> accounts;

    BankAccount* findAccount(string number);

public:
    bool registerAccount(string number, string name,
                         string pin, string accountType = "Savings");

    bool login(string number, string pin);

    bool getAccountDetails(string number, string& name, double& balance);

    bool getAccountType(string number, string& accountType);

    double checkBalance(string number);

    bool depositMoney(string number, double amount);

    bool withdrawMoney(string number, double amount);

    bool transferMoney(string sender, string receiver, double amount);

    void loadAccounts();

    bool saveAccounts();

    void saveTransaction(string details);

    string getTransactions(string number);

    vector<BankAccount> getAccounts();
};

#endif
