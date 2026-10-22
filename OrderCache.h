#pragma once

#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Order
{
public:
    // do not alter signature of this constructor
    Order(
        const std::string& ordId,
        const std::string& secId,
        const std::string& side,
        const unsigned int qty,
        const std::string& user,
        const std::string& company)
        : m_orderId(ordId),
          m_securityId(secId),
          m_side(side),
          m_qty(qty),
          m_user(user),
          m_company(company) { }

    // do not alter these accessor methods
    std::string orderId() const    { return m_orderId; }
    std::string securityId() const { return m_securityId; }
    std::string side() const       { return m_side; }
    std::string user() const       { return m_user; }
    std::string company() const    { return m_company; }
    unsigned int qty() const       { return m_qty; }

private:
    // use the below to hold the order data
    // do not remove these member variables
    std::string m_orderId;     // unique order id
    std::string m_securityId;  // security identifier
    std::string m_side;        // side of the order, eg Buy or Sell
    unsigned int m_qty;        // qty for this order
    std::string m_user;        // user name who owns this order
    std::string m_company;     // company for user
};

// Provide an implementation for the OrderCacheInterface interface class.
// Your implementation class should hold all relevant data structures you think
// are needed.
class OrderCacheInterface
{
public:
    virtual ~OrderCacheInterface() = default;

    // implement the 6 methods below, do not alter signatures

    // add order to the cache
    virtual void addOrder(Order order) = 0;

    // remove order with this unique order id from the cache
    virtual void cancelOrder(const std::string& orderId) = 0;

    // remove all orders in the cache for this user
    virtual void cancelOrdersForUser(const std::string& user) = 0;

    // remove all orders in the cache for this security with qty >= minQty
    virtual void cancelOrdersForSecIdWithMinimumQty(const std::string& securityId, unsigned int minQty) = 0;

    // return the total qty that can match for the security id
    virtual unsigned int getMatchingSizeForSecurity(const std::string& securityId) = 0;

    // return all orders in cache in a vector
    virtual std::vector<Order> getAllOrders() const = 0;
};

namespace Redis
{
    class RedisClient;
}

/**
 * @brief Thread-safe in-memory order cache and real-time matching engine.
 *
 * Uses a Single-Writer, Multiple-Reader (SWMR) concurrency model via std::shared_mutex
 * and maintains incremental per-security and per-company volume aggregates to evaluate
 * getMatchingSizeForSecurity() in O(K) closed-form time (where K is the number of
 * distinct companies actively trading the security).
 */
class OrderCache final : public OrderCacheInterface
{
public:
    OrderCache() = default;
    explicit OrderCache(std::shared_ptr<Redis::RedisClient> redisClient, const bool loadExisting = false);
    ~OrderCache() override = default;

    void addOrder(Order order) override;

    void cancelOrder(const std::string& orderId) override;

    void cancelOrdersForUser(const std::string& user) override;

    void cancelOrdersForSecIdWithMinimumQty(const std::string& securityId, unsigned int minQty) override;

    unsigned int getMatchingSizeForSecurity(const std::string& securityId) override;

    std::vector<Order> getAllOrders() const override;

    void loadFromRedis();

private:
    struct CompanyVolume
    {
        std::uint64_t buyQty{0};
        std::uint64_t sellQty{0};
    };

    struct SecurityAggregate
    {
        std::uint64_t totalBuyQty{0};
        std::uint64_t totalSellQty{0};
        std::unordered_map<std::string, CompanyVolume> companyVolumes;
    };

    std::unordered_map<std::string, Order> m_orders;
    std::unordered_map<std::string, SecurityAggregate> m_securityAggregates;

    void updateAggregateAdd(const Order& order);
    void updateAggregateRemove(const Order& order);

    std::shared_ptr<Redis::RedisClient> m_redisClient{nullptr};
    mutable std::shared_mutex m_mutex;
};
