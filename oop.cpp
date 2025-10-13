#include <iostream>
#include <cstdio>
#include <stdio.h>
using namespace std;

// Abstract base class (Abstraction)
class Account {
protected:
    string name;     // Encapsulation: protected attribute
    double balance;
public:
    Account(string n, double b=0) : name(n), balance(b) {}
    virtual ~Account() {}

    void deposit(double amount) { balance += amount; }
    double get_balance() const { return balance; }

    virtual void withdraw(double amount) = 0; // Pure virtual function (Abstraction)
    virtual void print_info() const {
        cout << "Account: " << name << ", Balance: " << balance << endl;
    }
};

// Normal account (Inheritance)
class NormalAccount : public Account {
public:
    NormalAccount(string n, double b=0) : Account(n,b) {}
    void withdraw(double amount) override {   // Polymorphism
        if(amount <= balance) balance -= amount;
        else printf("Insufficient funds\n");
    }
};

// Savings account (Inheritance + Polymorphism)
class SavingsAccount : public Account {
private:
    double interest;
public:
    SavingsAccount(string n, double b=0, double i=0.02) 
        : Account(n,b), interest(i) {}
    void withdraw(double amount) override {
        if(amount <= balance) balance -= amount;
        else printf("Cannot overdraft a savings account\n");
    }
    void add_interest() { balance += balance * interest; }
};

int main() {
    NormalAccount a1("Alice", 100);
    SavingsAccount a2("Bob", 200);

    a1.withdraw(50);
    a2.add_interest();

    a1.print_info();
    a2.print_info();
    return 0;
}
