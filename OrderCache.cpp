#include "OrderCache.h"

#include <vector>
#include <algorithm>
#include <string_view>
#include <mutex>


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

void OrderCache::addOrder(Order order)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    // It is better to have isValid to be first to improve performance
    if (isValid(order) && m_orders.find(order.orderId()) == m_orders.end() ) 
    {
        updateAggregateAdd(order);
        m_orders.emplace(order.orderId(), std::move(order));
    }
}

void OrderCache::cancelOrder(const std::string& orderId)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    const auto it = m_orders.find(orderId);
    if (it != m_orders.end())
    {
        updateAggregateRemove(it->second);
        m_orders.erase(it);
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
    const unsigned int cap = std::min(aggregate.totalBuyQty, aggregate.totalSellQty);
    if (cap == 0) return 0;

    unsigned int matchable = cap;

    for (const auto& [company, volume] : aggregate.companyVolumes)
    {
        const unsigned int outsideBuys = aggregate.totalBuyQty - volume.buyQty;
        const unsigned int outsideSells = aggregate.totalSellQty - volume.sellQty;
        const unsigned int allowedForComp = outsideBuys + outsideSells;

        matchable = std::min(matchable, allowedForComp);
    }

    return matchable;
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
        aggregate.companyVolumes.erase(order.company());
    }

    if (aggregate.totalBuyQty == 0 && aggregate.totalSellQty == 0)
    {
        m_securityAggregates.erase(order.securityId());
    }
}