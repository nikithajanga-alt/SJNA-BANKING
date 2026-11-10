
#ifndef BANKACCOUNT_H
#define BANKACCOUNT_H

#include <string>
using namespace std;

class BankAccount
{
private:
    string accountNumber;
    string accountHolder;
    string pin;
    string accountType;
    double balance;

public:
    BankAccount();
    BankAccount(string number, string holder, string password,
                double amount, string type = "Savings");

    string getAccountNumber() const;
    string getAccountHolder() const;
    string getPin() const;
    string getAccountType() const;
    double getBalance() const;

    bool checkPin(string password) const;
    void deposit(double amount);
    bool withdraw(double amount);
    void setBalance(double amount);
};

#endif
