#include "OrderCache.h"

#include <vector>
#include <algorithm>
#include <string_view>


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
    if (m_orders.find(order.orderId()) == m_orders.end() && isValid(order)) 
    {
        m_orders.emplace(order.orderId(), std::move(order));
    }
}

void OrderCache::cancelOrder(const std::string& orderId)
{
    m_orders.erase(orderId);
}

void OrderCache::cancelOrdersForUser(const std::string& user)
{
    for (auto it = m_orders.begin(); it != m_orders.end() ; )
    {
        if (it->second.user() == user)
        {
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
    for (auto it = m_orders.begin(); it != m_orders.end() ; )
    {
        if (it->second.securityId() == securityId &&  it->second.qty() >= minQty)
        {
            it = m_orders.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

struct MatchingCandidate
{
    std::string company;
    unsigned int remainingQty;

    MatchingCandidate(const std::string& company, const unsigned int remainingQty)
     : company(company), remainingQty(remainingQty)
     {}
};


unsigned int OrderCache::getMatchingSizeForSecurity(const std::string& securityId)
{
    unsigned int matched{0};

    std::vector<MatchingCandidate> buyOrders;
    std::vector<MatchingCandidate> sellOrders;


    // Collecting Buy and Sell order
    for (const auto& order : m_orders)
    {
        if (order.second.securityId() == securityId)
        {
            if (order.second.side() == BUY)
            {
                buyOrders.emplace_back(order.second.company(), order.second.qty() );
            }
            else
            {
                sellOrders.emplace_back(order.second.company(), order.second.qty() );
            }
        }
    }

    // Matching algorithm
    for (auto& currentBuy : buyOrders)
    {
        if (currentBuy.remainingQty == 0) continue;

        for (auto& currentSell : sellOrders)
        {
            // Constrains from the documentation
            if (currentSell.remainingQty == 0) continue;
            if (currentBuy.company == currentSell.company) continue;

            unsigned int matchQty = std::min(currentBuy.remainingQty, currentSell.remainingQty);

            matched += matchQty;
            currentBuy.remainingQty -= matchQty;
            currentSell.remainingQty -= matchQty;

             // If remaining Buy drops to d not waste processor time and skip loop to the begginig
            if (currentBuy.remainingQty == 0) break;
        }
    }

    return matched;
}


std::vector<Order> OrderCache::getAllOrders() const
{
    std::vector<Order> result;

    result.reserve(m_orders.size());

    for (const auto& orderPair : m_orders)
    {
        result.push_back(orderPair.second);
    }

    return result;

}
