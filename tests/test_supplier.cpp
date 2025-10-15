// tests/test_supplier.cpp
#include <gtest/gtest.h>
#include <pcosynchro/pcothread.h>
#include <memory>
#include <vector>
#include "supplier.h"
#include "costs.h"
#include "day_clock.h"

class TestableSupplier : public Supplier {
public:
    using Supplier::Supplier;
    using Seller::stocks;
    using Seller::money;
    using Seller::nbEmployeesPaid;

    void setStock(ItemType it, int qty) { stocks[it] = qty; }
    int getStock(ItemType it) const {
        auto itf = stocks.find(it);
        return itf == stocks.end() ? 0 : itf->second;
    }
    int getFund() const { return money; }
};

TEST(SupplierBasic, SellsResource_MatchesCatalog) {
    TestableSupplier s(1, 0, {ItemType::Pill, ItemType::Thermometer});
    EXPECT_TRUE(s.sellsResource(ItemType::Pill));
    EXPECT_TRUE(s.sellsResource(ItemType::Thermometer));
    EXPECT_FALSE(s.sellsResource(ItemType::Syringe));
}

TEST(SupplierBasic, Buy_ReturnsZero_WhenInsufficientStock) {
    TestableSupplier s(2, 0, {ItemType::Pill});
    s.setStock(ItemType::Pill, 0);
    int bill = s.buy(ItemType::Pill, 1);
    EXPECT_EQ(bill, 0);
    EXPECT_EQ(s.getStock(ItemType::Pill), 0);
}

TEST(SupplierBasic, Buy_ReducesStock_AndReturnsCorrectBill) {
    TestableSupplier s(3, 0, {ItemType::Pill});
    s.setStock(ItemType::Pill, 5);

    int unit = getCostPerUnit(ItemType::Pill);
    int bill = s.buy(ItemType::Pill, 2);

    EXPECT_EQ(bill, 2 * unit);
    EXPECT_EQ(s.getStock(ItemType::Pill), 3);
}

TEST(SupplierBasic, Pay_IncreasesFunds) {
    TestableSupplier s(4, 0, {ItemType::Pill});
    s.pay(250);
    EXPECT_EQ(s.getFund(), 250);
}

TEST(SupplierThreading, PayIsThreadSafe) {
    TestableSupplier s(6, 0, {ItemType::Pill});
    const int N = 1000;
    std::vector<std::unique_ptr<PcoThread>> ts;
    for (int i = 0; i < 4; ++i) {
        ts.emplace_back(std::make_unique<PcoThread>([&]() {
            for (int k = 0; k < N; ++k) s.pay(1);
        }));
    }
    for (auto& t : ts) t->join();
    EXPECT_EQ(s.getFund(), 4 * N);
}

TEST(SupplierRun, ProducesItemsWhenFundsSufficient) {
    TestableSupplier s(7, /*fund*/100'000, {ItemType::Pill, ItemType::Thermometer, ItemType::Syringe});

    // DayClock avec 1 participant (le supplier uniquement)
    DayClock clock(/*participants*/1);
    s.setClock(&clock);

    std::unique_ptr<PcoThread> th = std::make_unique<PcoThread>([&](){ s.run(); });

    // Simuler quelques journées
    const int DAYS = 5;
    for (int d = 0; d < DAYS; ++d) {
        clock.start_next_day();
        clock.wait_all_done();
    }

    // Arrêt propre
    th->requestStop();
    clock.start_next_day(); // libère un éventuel wait au début de journée
    th->join();

    int totalStock = s.getStock(ItemType::Pill)
                   + s.getStock(ItemType::Thermometer)
                   + s.getStock(ItemType::Syringe);
    EXPECT_GE(totalStock, 5);
    EXPECT_EQ(s.getAmountPaidToEmployees(EmployeeType::Supplier), 5 * getEmployeeSalary(EmployeeType::Supplier));
}
