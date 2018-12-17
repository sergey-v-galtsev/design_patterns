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
#include <cassert>
#include <iostream>
#include <memory>
#include <utility>


using namespace std;


enum class Currency {
    rub,
    usd,
    eur,
};


class Dispenser {
public:
    Dispenser(int count = 0, int value = 0, Currency currency = Currency::rub)
        : count_{count}
        , value_{value}
        , currency_{currency}
    {
    }

    int withdraw(int amount, Currency currency = Currency::rub) {
        if (currency == currency_ and value_ != 0) {
            const auto number = min(amount / value_, count_);
            amount -= number * value_;
            count_ -= number;
        }

        if (next()) {
            return next()->withdraw(amount);
        } else {
            return amount;
        }
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
        return next_ == nullptr;
    }

    void setNext(unique_ptr<Dispenser> next) {
        next_ = move(next);
    }

private:
    unique_ptr<Dispenser> next_ = nullptr;
    int count_;
    int value_;
    Currency currency_;
};


void testDispenser() {
    auto null = make_unique<Dispenser>();

    assert(null->isDone());
    assert(null->withdraw(1) == 1);

    const auto rawNull = null.get();

    auto rub100 = make_unique<Dispenser>(10, 100, Currency::rub);
    rub100->setNext(move(null));

    assert(rub100->first() == rub100.get());
    assert(rub100->next() == rawNull);
    assert(not rub100->isDone());
    assert(rub100->next()->isDone());
    assert(rub100->withdraw(123, Currency::rub) == 23);
    assert(rub100->balance(Currency::rub) == 900);
    assert(rub100->withdraw(300, Currency::rub) == 0);
    assert(rub100->balance(Currency::rub) == 600);
    assert(rub100->withdraw(100, Currency::usd) == 100);
    assert(rub100->balance(Currency::rub) == 600);
    assert(rub100->balance(Currency::eur) == 0);
}


int main() {
    testDispenser();
    cout << "All tests passed.\n";
    return 0;
}
