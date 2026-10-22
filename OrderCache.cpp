#include "OrderCache.h"

#include <algorithm>
#include <limits>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    constexpr std::string_view BUY{"Buy"};
    constexpr std::string_view SELL{"Sell"};

    bool isValid(const Order& order)
    {
        if (order.qty() == 0)
        {
            return false;
        }

        if (order.orderId().empty() ||
            order.securityId().empty() ||
            order.company().empty() ||
            order.user().empty())
        {
            return false;
        }

        const std::string side = order.side();
        return side == BUY || side == SELL;
    }
} // namespace

void OrderCache::addOrder(Order order)
{
    // Validate input outside the exclusive lock to minimize lock contention
    if (!isValid(order))
    {
        return;
    }

    std::string id = order.orderId();

    std::unique_lock<std::shared_mutex> lock(m_mutex);

    if (m_orders.find(id) == m_orders.end())
    {
        updateAggregateAdd(order);
        m_orders.emplace(std::move(id), std::move(order));
    }
}

void OrderCache::cancelOrder(const std::string& orderId)
{
    if (orderId.empty())
    {
        return;
    }

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
    if (user.empty())
    {
        return;
    }

    std::unique_lock<std::shared_mutex> lock(m_mutex);

    for (auto it = m_orders.begin(); it != m_orders.end(); )
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
    if (securityId.empty() || minQty == 0)
    {
        return;
    }

    std::unique_lock<std::shared_mutex> lock(m_mutex);

    for (auto it = m_orders.begin(); it != m_orders.end(); )
    {
        if (it->second.qty() >= minQty && it->second.securityId() == securityId)
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
    if (securityId.empty())
    {
        return 0;
    }

    std::shared_lock<std::shared_mutex> lock(m_mutex);

    const auto itSecurity = m_securityAggregates.find(securityId);
    if (itSecurity == m_securityAggregates.end())
    {
        return 0;
    }

    const auto& aggregate = itSecurity->second;
    const std::uint64_t cap = std::min(aggregate.totalBuyQty, aggregate.totalSellQty);
    if (cap == 0)
    {
        return 0;
    }

    std::uint64_t matchable = cap;

    // Closed-form O(K) bipartite matching bound (Hall's Marriage Theorem / Min-Cut):
    // Since orders from the same company cannot match against each other, the maximum
    // matchable volume is bounded by min(totalBuyQty, totalSellQty) and, for every
    // company C, by the total volume available outside of C:
    //   (totalBuyQty - buyQty[C]) + (totalSellQty - sellQty[C]).
    for (const auto& [company, volume] : aggregate.companyVolumes)
    {
        const std::uint64_t outsideBuys = aggregate.totalBuyQty - volume.buyQty;
        const std::uint64_t outsideSells = aggregate.totalSellQty - volume.sellQty;
        const std::uint64_t allowedForComp = outsideBuys + outsideSells;

        matchable = std::min(matchable, allowedForComp);
    }

    if (matchable > std::numeric_limits<unsigned int>::max())
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

// -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- Private Methods -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

void OrderCache::updateAggregateAdd(const Order& order)
{
    auto& aggregate = m_securityAggregates[order.securityId()];
    auto& companyVolume = aggregate.companyVolumes[order.company()];
    const unsigned int qty = order.qty();

    if (order.side() == BUY)
    {
        aggregate.totalBuyQty += qty;
        companyVolume.buyQty += qty;
    }
    else
    {
        aggregate.totalSellQty += qty;
        companyVolume.sellQty += qty;
    }
}

void OrderCache::updateAggregateRemove(const Order& order)
{
    const auto itSecurity = m_securityAggregates.find(order.securityId());
    if (itSecurity == m_securityAggregates.end())
    {
        return;
    }

    auto& aggregate = itSecurity->second;
    const auto itCompany = aggregate.companyVolumes.find(order.company());
    if (itCompany == aggregate.companyVolumes.end())
    {
        return;
    }

    const unsigned int qty = order.qty();

    if (order.side() == BUY)
    {
        aggregate.totalBuyQty -= qty;
        itCompany->second.buyQty -= qty;
    }
    else
    {
        aggregate.totalSellQty -= qty;
        itCompany->second.sellQty -= qty;
    }

    if (itCompany->second.buyQty == 0 && itCompany->second.sellQty == 0)
    {
        aggregate.companyVolumes.erase(itCompany);
    }

    if (aggregate.totalBuyQty == 0 && aggregate.totalSellQty == 0)
    {
        m_securityAggregates.erase(itSecurity);
    }
}