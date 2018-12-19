# Design Patterns

## Создайте модель банкомата:

1. В банкомат могут быть загружено несколько типовых кассет (кассета это класс или интерфейс?) с различным количеством банкнот определенных номиналов и валют (например, 100р, 500р, 1000р или $5, $20, $100).
2. Используйте паттерн Сhain of Responsibility для выдачи наличных по запросу пользователя (метод `withdraw(int amount): error`). Выполнение метода может закончиться с ошибкой, если в банкомате недостаточно денег или банкнот определенного номинала.
3. Используйте паттерн Iterator для вычисления остатка денег в банкомате (метод `balance(): int`). Примечание: паттерн Chain of Responsibility можно красиво интегрировать с Iterator, а можно сделать "в лоб".
4. Создайте модель банка для работы с сетью банкоматов.
5. Банк может устанавливать новые банкоматы на основе какой-то типовой конфигурации. Для этого используйте паттерн Prototype.
6. Используйте паттерн Observer для получения банком оповещений о том что в кассете банкомата закончились банкноты.

## Travis CI

[![Build Status](https://travis-ci.org/sergey-v-galtsev/design_patterns.svg?branch=master)](https://travis-ci.org/sergey-v-galtsev/design_patterns)
