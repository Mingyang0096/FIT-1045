from abc import ABC, abstractmethod

class Account(ABC):
    def __init__(self, name, balance=0):
        self._name = name
        self._balance = balance

    @abstractmethod
    def withdraw(self, amount):
        pass

    def deposit(self, amount):
        self._balance += amount

    def get_balance(self):
        return self._balance

    def print_info(self):
        print(f"Account: {self._name}, Balance: {self._balance}")

class NormalAccount(Account):
    def withdraw(self, amount):
        if amount <= self._balance:
            self._balance -= amount
        else:
            print("Insufficient funds")

class SavingsAccount(Account):
    def __init__(self, name, balance=0, interest=0.02):
        self._interest = interest

    def withdraw(self, amount):
        if amount <= self._balance:
            self._balance -= amount
        else:
            print("Cannot overdraft a savings account")

    def add_interest(self):
        self._balance += self._balance * self._interest

a1 = NormalAccount("Alice", 100)
a2 = SavingsAccount("Bob", 200)

a1.withdraw(50)
a2.add_interest()
a1.print_info()
a2.print_info()

