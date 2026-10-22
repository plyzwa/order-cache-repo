#include "OrderCache.h"
#include <RedisClient.h>

#include <vector>
#include <algorithm>
#include <string_view>
#include <mutex>
#include <limits>


constexpr std::string_view BUY{"Buy"};
constexpr std::string_view SELL{"Sell"};

static bool isValid(const Order& order)
{

    if (order.orderId().empty() || order.securityId().empty() || order.company().empty() || order.user().empty())
    {
        return false;
    }
    else if (order.qty() == 0)
    {
        return false;
    }
    
    return order.side() == BUY || order.side() == SELL;
}

OrderCache::OrderCache(std::shared_ptr<Redis::RedisClient> redisClient, const bool loadExisting)
    : m_redisClient(std::move(redisClient))
    {
        if (m_redisClient && loadExisting) loadFromRedis();
    }

void OrderCache::addOrder(Order order)
{
    // It is better to have isValid to be first to improve performance
    if (isValid(order) == false) return;
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    if (m_orders.find(order.orderId()) == m_orders.end() ) 
    {
        updateAggregateAdd(order);

        if (m_redisClient) m_redisClient->saveOrder(order);

        auto id = order.orderId();
        m_orders.emplace(std::move(id), std::move(order));
    }
}

void OrderCache::cancelOrder(const std::string& orderId)
{
    if (orderId.empty()) return;
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    const auto it = m_orders.find(orderId);
    if (it != m_orders.end())
    {
        updateAggregateRemove(it->second);
        m_orders.erase(it);

        if (m_redisClient) m_redisClient->removeOrder(orderId);
    }

}

void OrderCache::cancelOrdersForUser(const std::string& user)
{
    if (user.empty()) return;

    std::unique_lock<std::shared_mutex> lock(m_mutex);

    for (auto it = m_orders.begin(); it != m_orders.end() ; )
    {
        if (it->second.user() == user)
        {
            updateAggregateRemove(it->second);

            if (m_redisClient) m_redisClient->removeOrder(it->first);
            it = m_orders.erase(it);

        }
        else
        {
            ++it;
        }
    }
}

void OrderCache::cancelOrdersForSecIdWithMinimumQty(const std::string& securityId, unsigned int minQty)
{
    if (securityId.empty()) return;
    if(minQty == 0) return;

    std::unique_lock<std::shared_mutex> lock(m_mutex);

    for (auto it = m_orders.begin(); it != m_orders.end() ; )
    {
        if (it->second.securityId() == securityId &&  it->second.qty() >= minQty)
        {
            updateAggregateRemove(it->second);
            
            if (m_redisClient) m_redisClient->removeOrder(it->first);
            it = m_orders.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

unsigned int OrderCache::getMatchingSizeForSecurity(const std::string& securityId)
{
    if (securityId.empty()) return 0;

    std::shared_lock<std::shared_mutex> lock(m_mutex);

    const auto itSecurity = m_securityAggregates.find(securityId);
    if (itSecurity == m_securityAggregates.end()) return 0;

    const auto& aggregate = itSecurity->second;
    const std::uint64_t cap = std::min(aggregate.totalBuyQty, aggregate.totalSellQty);
    if (cap == 0) return 0;

    std::uint64_t matchable = cap;

    for (const auto& [company, volume] : aggregate.companyVolumes)
    {
        const std::uint64_t outsideBuys = aggregate.totalBuyQty - volume.buyQty;
        const std::uint64_t outsideSells = aggregate.totalSellQty - volume.sellQty;
        const std::uint64_t allowedForComp = outsideBuys + outsideSells;

        matchable = std::min(matchable, allowedForComp);
    }


    if (matchable > std::numeric_limits<unsigned int>::max() )
    {
        return std::numeric_limits<unsigned int>::max();
    }

    return static_cast<unsigned int>(matchable);
}

std::vector<Order> OrderCache::getAllOrders() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    std::vector<Order> result;

    result.reserve(m_orders.size());

    for (const auto& orderPair : m_orders)
    {
        result.push_back(orderPair.second);
    }

    return result;

}

void OrderCache::loadFromRedis()
{
    if (!m_redisClient) return;

    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    // Clear current state before loading
    m_orders.clear();
    m_securityAggregates.clear();

    // Fetch all active order keys from Redis using SCAN or KEYS
    // (Assuming we add a helper in RedisClient or query keys directly)
    const auto& orderIds = m_redisClient->getAllOrderIds(); // We can define this helper in RedisClient
    
    for (const auto& orderId : orderIds)
    {
        auto orderOpt = m_redisClient->getOrder(orderId);
        if (orderOpt.has_value())
        {
            Order& order = *orderOpt;
            updateAggregateAdd(order);
            m_orders.emplace(order.orderId(), std::move(order));
        }
    }
}


// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- Private Methods -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void OrderCache::updateAggregateAdd(const Order& order)
{
    auto& aggregate = m_securityAggregates[order.securityId()];
    auto& companyVolume = aggregate.companyVolumes[order.company()];

    if (order.side() == BUY)
    {
        aggregate.totalBuyQty += order.qty();
        companyVolume.buyQty += order.qty();
    }
    else
    {
        aggregate.totalSellQty += order.qty();
        companyVolume.sellQty += order.qty();
    }
}

void OrderCache::updateAggregateRemove(const Order& order)
{
    const auto itSecurity = m_securityAggregates.find(order.securityId());
    if (itSecurity == m_securityAggregates.end()) return;
    
    auto& aggregate = itSecurity->second;
    auto itCompany = aggregate.companyVolumes.find(order.company());

    if (itCompany == aggregate.companyVolumes.end() ) return;

    if (order.side() == BUY)
    {
        aggregate.totalBuyQty -= order.qty();
        itCompany->second.buyQty -= order.qty();
    }
    else
    {
        aggregate.totalSellQty -= order.qty();
        itCompany->second.sellQty -= order.qty();
    }

    if (itCompany->second.buyQty == 0 && itCompany->second.sellQty  == 0)
    {
        aggregate.companyVolumes.erase(itCompany);
    }

    if (aggregate.totalBuyQty == 0 && aggregate.totalSellQty == 0)
    {
        m_securityAggregates.erase(itSecurity);
    }
}
