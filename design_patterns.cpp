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
#include <vector>


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


class Dispenser;


class DispenserObserver {
public:
    virtual void update(Dispenser* dispenser) = 0;
};


class Dispenser {
public:
    Dispenser(int count = 0, int value = 0, Currency currency = Currency::rub)
        : count_{count}
        , value_{value}
        , currency_{currency}
    {
    }

    void withdraw(int amount, Currency currency, DispenserObserver* observer = nullptr) {
        if (amount <= 0) {
            throw logic_error{"cat not withdraw non-positive amount"};
        }

        const auto extraction = currency == currency_ and value_ != 0 ? min(amount / value_, count_) : 0;
        const auto result = amount - extraction * value_;

        if (next() and result > 0) {
            next()->withdraw(result, currency, observer);
        } else {
            if (result != 0) {
                throw logic_error{"cat not withdraw the given amount"};
            }
        }

        if (extraction > 0) {
            cout << "dispensing " << extraction << " * " << value_ << " = " << extraction * value_ << ' ' << getCurrencyName(currency_) << '\n';
        }

        count_ -= extraction;

        if (count_ == 0 and observer) {
            observer->update(this);
        }
    }

    int balance(Currency currency = Currency::rub) const {
        return currency == currency_ ? count_ * value_ : 0;
    }

    void restock(int count) {
        count_ += count;
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

    unique_ptr<Dispenser> clone() const {
        return make_unique<Dispenser>(count_, value_, currency_);
    }

private:
    unique_ptr<Dispenser> next_ = nullptr;
    int count_;
    int value_;
    Currency currency_;
};


class Atm {
public:
    Atm(DispenserObserver* observer = nullptr)
        : dispensers_{make_unique<Dispenser>()}
        , observer_{observer}
    {
    }

    void withdraw(int amount, Currency currency = Currency::rub) {
        try {
            dispensers_->withdraw(amount, currency, observer_);
            cout << "withdrawen " << amount << ' ' << getCurrencyName(currency) << '\n';
        } catch (logic_error& error) {
            cout << "error withdrawing " << amount << ' ' << getCurrencyName(currency) << ": " << error.what() << '\n';
            throw;
        }
    }

    int balance(Currency currency = Currency::rub) {
        int amount = 0;
        for (auto it = dispensers_->first(); not it->isDone(); it = it->next()) {
            amount += it->balance(currency);
        }
        return amount;
    }

    Atm& addObserver(DispenserObserver* observer) {
        observer_ = observer;
        return *this;
    }

    Atm& addDispenser(unique_ptr<Dispenser> dispenser) {
        Dispenser* prev = nullptr;
        auto it = dispensers_->first();
        while (not it->shouldFollow(dispenser.get())) {
            prev = it;
            it = it->next();
        }

        if (prev) {
            prev->insert(move(dispenser));
        } else {
            dispenser->setNext(move(dispensers_));
            dispensers_ = move(dispenser);
        }

        return *this;
    }

    template <typename... Args>
    Atm& addDispenser(Args... args) {
        return addDispenser(make_unique<Dispenser>(forward<Args>(args)...));
    }

    unique_ptr<Atm> clone() const {
        auto atm = make_unique<Atm>(observer_);
        for (auto it = dispensers_->first(); not it->isDone(); it = it->next()) {
            atm->addDispenser(it->clone());
        }
        return atm;
    }

private:
    unique_ptr<Dispenser> dispensers_;
    DispenserObserver* observer_;
};


using AtmRegistry = unordered_map<string, Atm>;


class Bank : public DispenserObserver {
public:
    Bank() {
        registry["EUR"]
            .addObserver(this)
            .addDispenser(100, 500, Currency::eur)
            .addDispenser(100, 100, Currency::eur)
            .addDispenser(100, 50, Currency::eur)
            .addDispenser(100, 20, Currency::eur)
            .addDispenser(100, 10, Currency::eur)
            .addDispenser(100, 5, Currency::eur);
        registry["RUB"]
            .addObserver(this)
            .addDispenser(100, 5000, Currency::rub)
            .addDispenser(100, 1000, Currency::rub)
            .addDispenser(100, 500, Currency::rub)
            .addDispenser(100, 100, Currency::rub);
        registry["USD"]
            .addObserver(this)
            .addDispenser(100, 100, Currency::usd)
            .addDispenser(100, 50, Currency::usd)
            .addDispenser(100, 20, Currency::usd);
        registry["all"]
            .addObserver(this)
            .addDispenser(100, 5000, Currency::rub)
            .addDispenser(100, 500, Currency::rub)
            .addDispenser(100, 100, Currency::eur)
            .addDispenser(100, 100, Currency::usd);
    }

    Atm* addAtm(const string& name) {
        if (registry.count(name)) {
            atms.push_back(registry[name].clone());
            return atms.back().get();
        } else {
            return nullptr;
        }
    }

    void update(Dispenser* dispenser) override {
        if (dispenser->balance() == 0) {
            dispenser->restock(100);
        }
    }

private:
    AtmRegistry registry;
    vector<unique_ptr<Atm>> atms;
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

    testTransactionRollback(*rub100, 0, Currency::rub);
    testTransactionRollback(*rub100, -100, Currency::rub);
    testTransactionRollback(*rub100, 123, Currency::rub);

    rub100->withdraw(100, Currency::rub);
    assert(rub100->balance(Currency::rub) == 900);
    rub100->withdraw(300, Currency::rub);
    assert(rub100->balance(Currency::rub) == 600);

    testTransactionRollback(*rub100, 100, Currency::usd);

    assert(rub100->balance(Currency::eur) == 0);
}


void testAtmSingleDispenser() {
    Atm atm{nullptr};
    atm.addDispenser(10, 100, Currency::rub);

    assert(atm.balance(Currency::rub) == 1000);

    testTransactionRollback(atm, 0, Currency::rub);
    testTransactionRollback(atm, -100, Currency::rub);
    testTransactionRollback(atm, 123, Currency::rub);

    atm.withdraw(100, Currency::rub);
    assert(atm.balance(Currency::rub) == 900);
    atm.withdraw(300, Currency::rub);
    assert(atm.balance(Currency::rub) == 600);

    testTransactionRollback(atm, 100, Currency::usd);
}


void testAtmMultiDispenser() {
    Atm atm{nullptr};
    atm.addDispenser(3, 100, Currency::rub);
    atm.addDispenser(3, 500, Currency::rub);
    atm.addDispenser(3, 200, Currency::rub);

    const auto initialBalance = 3 * (100 + 500 + 200);

    assert(atm.balance(Currency::rub) == initialBalance);
    assert(atm.balance(Currency::eur) == 0);
    assert(atm.balance(Currency::usd) == 0);

    testTransactionRollback(atm, 0, Currency::rub);
    testTransactionRollback(atm, -100, Currency::rub);
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


class MockDispenserObserver : public DispenserObserver {
public:
    MockDispenserObserver(function<void(Dispenser* dispenser)> update)
        : update_{update}
    {
    }

    void update(Dispenser* dispenser) override {
        update_(dispenser);
    }

private:
    function<void(Dispenser* dispenser)> update_;
};


void testAtmMultiCurrency() {
    int updates = 0;
    MockDispenserObserver observer([&updates](Dispenser* dispenser) {
        ++updates;
    });

    Atm atm{&observer};
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
    testTransactionRollback(atm, initialBalanceUsd, Currency::eur);
    testTransactionRollback(atm, initialBalanceUsd, Currency::rub);

    atm.withdraw(80, Currency::eur);
    atm.withdraw(700, Currency::rub);
    assert(atm.balance(Currency::eur) == initialBalanceEur - 80);
    assert(atm.balance(Currency::rub) == initialBalanceRub - 700);

    assert(updates == 0);
    atm.withdraw(initialBalanceUsd, Currency::usd);
    assert(updates == 1);
    assert(atm.balance(Currency::usd) == 0);
}


void testAtm() {
    testAtmSingleDispenser();
    testAtmMultiDispenser();
    testAtmMultiCurrency();
}


void testBank() {
    Bank bank;

    auto eur1 = bank.addAtm("EUR");
    auto eur2 = bank.addAtm("EUR");
    auto rub = bank.addAtm("RUB");
    auto usd = bank.addAtm("USD");
    auto all = bank.addAtm("all");

    assert(eur1->balance(Currency::eur) == eur2->balance(Currency::eur));

    eur1->withdraw(12345, Currency::eur);
    assert(eur1->balance(Currency::eur) == eur2->balance(Currency::eur) - 12345);

    eur2->withdraw(12345, Currency::eur);
    assert(eur1->balance(Currency::eur) == eur2->balance(Currency::eur));

    assert(eur1->balance(Currency::rub) == 0);
    assert(eur1->balance(Currency::usd) == 0);

    assert(rub->balance(Currency::eur) == 0);
    assert(rub->balance(Currency::rub) > 0);
    assert(rub->balance(Currency::usd) == 0);

    assert(all->balance(Currency::eur) > 0);
    assert(all->balance(Currency::rub) > 0);
    assert(all->balance(Currency::usd) > 0);

    const auto initialUsdBalance = usd->balance(Currency::usd);
    usd->withdraw(100, Currency::usd);
    assert(usd->balance(Currency::usd) < initialUsdBalance);
    usd->withdraw(usd->balance(Currency::usd), Currency::usd);
    assert(usd->balance(Currency::usd) == initialUsdBalance);
    assert(bank.addAtm("USD")->balance(Currency::usd) == initialUsdBalance);
}


int main() {
    testDispenser();
    testAtm();
    testBank();
    cout << "All tests passed.\n";
    return 0;
}
