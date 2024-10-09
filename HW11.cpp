#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

enum class OrderStatus {
    EMPTY,
    NEW,
    READY
};

class Restaurant {
private:
    OrderStatus orderStatus;
    std::mutex orderMutex;
    std::condition_variable orderBell;

public:
    Restaurant() : orderStatus(OrderStatus::EMPTY) {}

    void waiter() {
        while (true) {
            {
                std::lock_guard<std::mutex> lock(orderMutex);
                if (orderStatus == OrderStatus::EMPTY) {
                    std::cout << "Официант: Новый заказ поступил!" << std::endl;
                    orderStatus = OrderStatus::NEW;
                }
            }
            orderBell.notify_one(); // Уведомить повара

            // Дремать в ожидании сигнала от повара
            {
                std::unique_lock<std::mutex> lock(orderMutex);
                orderBell.wait(lock, [this] { return orderStatus == OrderStatus::READY; });
            }

            // Принести заказ
            std::cout << "Официант: Заказ готов, приношу его клиенту!" << std::endl;

            // Сбросить статус заказа
            {
                std::lock_guard<std::mutex> lock(orderMutex);
                orderStatus = OrderStatus::EMPTY;
            }

            // Небольшая задержка перед новым заказом
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    void chef() {
        while (true) {
            // Ждать, когда принесут новый заказ
            {
                std::unique_lock<std::mutex> lock(orderMutex);
                orderBell.wait(lock, [this] { return orderStatus == OrderStatus::NEW; });
            }

            // Приготовить блюда из заказа
            std::cout << "Повар: Готовлю заказ..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3)); // Время на приготовление

            // Нажать на кнопку вызова официанта
            {
                std::lock_guard<std::mutex> lock(orderMutex);
                orderStatus = OrderStatus::READY;
                std::cout << "Повар: Заказ готов!" << std::endl;
            }
            orderBell.notify_one(); // Уведомить официанта
        }
    }
};

int main() {
    setlocale(LC_ALL, "ru");

    Restaurant restaurant;

    std::thread waiterThread(&Restaurant::waiter, &restaurant);
    std::thread chefThread(&Restaurant::chef, &restaurant);

    waiterThread.join();
    chefThread.join();

    return 0;
}
