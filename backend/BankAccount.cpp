
#include "BankAccount.h"

BankAccount::BankAccount()
{
    accountNumber = "";
    accountHolder = "";
    pin = "";
    accountType = "Savings";
    balance = 0.0;
}

BankAccount::BankAccount(string number, string holder,
                         string password, double amount,
                         string type)
{
    accountNumber = number;
    accountHolder = holder;
    pin = password;
    accountType = type;
    balance = amount;
}

string BankAccount::getAccountNumber() const
{
    return accountNumber;
}

string BankAccount::getAccountHolder() const
{
    return accountHolder;
}

string BankAccount::getPin() const
{
    return pin;
}

string BankAccount::getAccountType() const
{
    return accountType;
}

double BankAccount::getBalance() const
{
    return balance;
}

bool BankAccount::checkPin(string password) const
{
    return pin == password;
}

void BankAccount::deposit(double amount)
{
    if (amount > 0)
        balance += amount;
}

bool BankAccount::withdraw(double amount)
{
    if (amount <= 0 || amount > balance)
        return false;

    balance -= amount;
    return true;
}

void BankAccount::setBalance(double amount)
{
    if (amount >= 0)
        balance = amount;
}
