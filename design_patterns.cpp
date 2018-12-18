/*
Design Patterns
Создайте модель банкомата:

В банкомат могут быть загружено несколько типовых кассет (кассета это класс или интерфейс?) с различным количеством банкнот определенных номиналов и валют (например, 100р, 500р, 1000р или $5, $20, $100).
Используйте паттерн Сhain of Responsibility для выдачи наличных по запросу пользователя (метод withdraw(int amount): error). Выполнение метода может закончиться с ошибкой, если в банкомате недостаточно денег или банкнот определенного номинала.
Используйте паттерн Iterator для вычисления остатка денег в банкомате (метод balance(): int). Примечание: паттерн Chain of Responsibility можно красиво интегрировать с Iterator, а можно сделать "в лоб".
Создайте модель банка для работы с сетью банкоматов.
Банк может устанавливать новые банкоматы на основе какой-то типовой конфигурации. Для этого используйте паттерн Prototype.
Используйте паттерн Observer для получения банком оповещений о том что в кассете банкомата закончились банкноты.
*/


#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>


using namespace std;


enum class Currency {
    eur = 0,
    rub,
    usd,
    currencySize
};


const string& getCurrencyName(Currency currency) {
    static const array<string, static_cast<size_t>(Currency::currencySize)> names = {"EUR", "RUB", "USD"};
    const size_t index = static_cast<size_t>(currency);
    assert(index < names.size());
    return names[index];
}


class Dispenser {
public:
    Dispenser(int count = 0, int value = 0, Currency currency = Currency::rub)
        : count_{count}
        , value_{value}
        , currency_{currency}
    {
    }

    void withdraw(int amount, Currency currency) {
        const auto extraction = currency == currency_ and value_ != 0 ? min(amount / value_, count_) : 0;
        const auto result = amount - extraction * value_;

        if (next()) {
            next()->withdraw(result, currency);
        } else {
            if (result != 0) {
                throw logic_error{"cat not withdraw the given amount"};
            }
        }

        if (extraction > 0) {
            cout << "Dispensing " << extraction << " * " << value_ << " = " << extraction * value_ << ' ' << getCurrencyName(currency_) << '\n';
        }

        count_ -= extraction;
    }

    int balance(Currency currency = Currency::rub) const {
        return currency == currency_ ? count_ * value_ : 0;
    }

    Dispenser* first() {
        return this;
    }

    Dispenser* next() const {
        return next_.get();
    }

    bool isDone() const {
        return next() == nullptr;
    }

    void setNext(unique_ptr<Dispenser> next) {
        next_ = move(next);
    }

    bool shouldFollow(Dispenser* that) const {
        return isDone() or (currency_ == that->currency_ and value_ < that->value_);
    }

    void insert(unique_ptr<Dispenser> next) {
        next->setNext(move(next_));
        setNext(move(next));
    }

private:
    unique_ptr<Dispenser> next_ = nullptr;
    int count_;
    int value_;
    Currency currency_;
};


class Atm {
public:
    Atm()
        : dispensers(make_unique<Dispenser>())
    {
    }

    void withdraw(int amount, Currency currency = Currency::rub) {
        try {
            dispensers->withdraw(amount, currency);
            cout << "Withdrawen " << amount << ' ' << getCurrencyName(currency) << '\n';
        } catch (logic_error&) {
            cout << "Can not withdraw " << amount << ' ' << getCurrencyName(currency) << '\n';
            throw;
        }
    }

    int balance(Currency currency = Currency::rub) {
        int amount = 0;
        for (auto it = dispensers->first(); not it->isDone(); it = it->next()) {
            amount += it->balance(currency);
        }
        return amount;
    }

    template <typename... Args>
    void addDispenser(Args... args) {
        auto dispenser = make_unique<Dispenser>(forward<Args>(args)...);
        Dispenser* prev = nullptr;
        auto it = dispensers->first();
        while (not it->shouldFollow(dispenser.get())) {
            prev = it;
            it = it->next();
        }

        if (prev) {
            prev->insert(move(dispenser));
        } else {
            dispenser->setNext(move(dispensers));
            dispensers = move(dispenser);
        }
    }

private:
    unique_ptr<Dispenser> dispensers;
};


template <typename Withdrawable>
void testTransactionRollback(Withdrawable& withdrawable, int amount, Currency currency) {
    const auto balance = withdrawable.balance(currency);
    try {
        withdrawable.withdraw(amount, currency);
        assert(false);
    } catch (logic_error&) {
    }
    assert(withdrawable.balance(currency) == balance);
}


void testDispenser() {
    auto null = make_unique<Dispenser>();

    assert(null->isDone());
    assert(null->balance() == 0);

    const auto rawNull = null.get();

    auto rub100 = make_unique<Dispenser>(10, 100, Currency::rub);
    rub100->setNext(move(null));

    assert(rub100->first() == rub100.get());
    assert(rub100->next() == rawNull);
    assert(not rub100->isDone());
    assert(rub100->next()->isDone());
    assert(rub100->balance(Currency::rub) == 1000);

    testTransactionRollback(*rub100, 123, Currency::rub);

    rub100->withdraw(100, Currency::rub);
    assert(rub100->balance(Currency::rub) == 900);
    rub100->withdraw(300, Currency::rub);
    assert(rub100->balance(Currency::rub) == 600);

    testTransactionRollback(*rub100, 100, Currency::usd);

    assert(rub100->balance(Currency::eur) == 0);
}


void testAtmSingleDispenser() {
    Atm atm;
    atm.addDispenser(10, 100, Currency::rub);

    assert(atm.balance(Currency::rub) == 1000);

    testTransactionRollback(atm, 123, Currency::rub);

    atm.withdraw(100, Currency::rub);
    assert(atm.balance(Currency::rub) == 900);
    atm.withdraw(300, Currency::rub);
    assert(atm.balance(Currency::rub) == 600);

    testTransactionRollback(atm, 100, Currency::usd);
}


void testAtmMultiDispenser() {
    Atm atm;
    atm.addDispenser(3, 100, Currency::rub);
    atm.addDispenser(3, 500, Currency::rub);
    atm.addDispenser(3, 200, Currency::rub);

    const auto initialBalance = 3 * (100 + 500 + 200);

    assert(atm.balance(Currency::rub) == initialBalance);
    assert(atm.balance(Currency::eur) == 0);
    assert(atm.balance(Currency::usd) == 0);

    testTransactionRollback(atm, 123, Currency::rub);

    atm.withdraw(100, Currency::rub);
    assert(atm.balance(Currency::rub) == initialBalance - 100);

    atm.withdraw(300, Currency::rub);
    assert(atm.balance(Currency::rub) == initialBalance - 400);

    atm.withdraw(100, Currency::rub);
    assert(atm.balance(Currency::rub) == initialBalance - 500);

    testTransactionRollback(atm, 100, Currency::rub);

    atm.withdraw(400, Currency::rub);
    assert(atm.balance(Currency::rub) == initialBalance - 900);

    testTransactionRollback(atm, 200, Currency::rub);
    assert(atm.balance(Currency::rub) == initialBalance - 900);

    atm.withdraw(1000, Currency::rub);
    assert(atm.balance(Currency::rub) == initialBalance - 1900);
}


void testAtmMultiCurrency() {
    Atm atm;
    atm.addDispenser(3, 100, Currency::rub);
    atm.addDispenser(3, 50, Currency::eur);
    atm.addDispenser(3, 7, Currency::usd);
    atm.addDispenser(3, 10, Currency::eur);
    atm.addDispenser(3, 500, Currency::rub);
    atm.addDispenser(3, 20, Currency::eur);
    atm.addDispenser(3, 200, Currency::rub);

    const auto initialBalanceEur = 3 * (50 + 10 + 20);
    const auto initialBalanceRub = 3 * (100 + 500 + 200);
    const auto initialBalanceUsd = 3 * 7;

    assert(atm.balance(Currency::eur) == initialBalanceEur);
    assert(atm.balance(Currency::rub) == initialBalanceRub);
    assert(atm.balance(Currency::usd) == initialBalanceUsd);

    testTransactionRollback(atm, 80, Currency::rub);
    testTransactionRollback(atm, 80, Currency::usd);
    testTransactionRollback(atm, 700, Currency::eur);
    testTransactionRollback(atm, 700, Currency::usd);
    testTransactionRollback(atm, 7, Currency::eur);
    testTransactionRollback(atm, 7, Currency::rub);

    atm.withdraw(80, Currency::eur);
    atm.withdraw(700, Currency::rub);
    atm.withdraw(7, Currency::usd);

    assert(atm.balance(Currency::eur) == initialBalanceEur - 80);
    assert(atm.balance(Currency::rub) == initialBalanceRub - 700);
    assert(atm.balance(Currency::usd) == initialBalanceUsd - 7);
}


void testAtm() {
    testAtmSingleDispenser();
    testAtmMultiDispenser();
    testAtmMultiCurrency();
}


int main() {
    testDispenser();
    testAtm();
    cout << "All tests passed.\n";
    return 0;
}
