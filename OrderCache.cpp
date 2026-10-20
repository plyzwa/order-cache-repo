// Todo: your implementation of the OrderCache...
#include "OrderCache.h"
#include <vector>


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
    
    return order.side() == "Buy" || order.side() == "Sell";
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

unsigned int OrderCache::getMatchingSizeForSecurity(const std::string& securityId)
{
    return 0; // TODO
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
