#include "RedisClient.h"

namespace Redis
{

RedisClient::RedisClient(const Config& config)
{
    sw::redis::ConnectionOptions connOpts;
    connOpts.host = config.host;
    connOpts.port = config.port;
    connOpts.connect_timeout = config.connectTimeout;
    connOpts.socket_timeout = config.socketTimeout;

    sw::redis::ConnectionPoolOptions poolOpts;
    poolOpts.size = config.poolSize;

    m_redis = std::make_unique<sw::redis::Redis>(connOpts, poolOpts);
}

bool RedisClient::ping() noexcept
{
    try
    {
        std::string response = m_redis->ping();
        return response == "PONG";
    }
    catch (const sw::redis::Error&)
    {
        return false;
    }
}

bool RedisClient::set(const std::string_view key, const std::string_view value, 
                      std::chrono::milliseconds ttl) noexcept
{
    try
    {
        if (ttl.count() > 0)
        {
            m_redis->set(std::string(key), std::string(value), ttl);
        }
        else
        {
            m_redis->set(std::string(key), std::string(value));
        }
        return true;
    }
    catch (const sw::redis::Error&)
    {
        return false;
    }
}

std::optional<std::string> RedisClient::get(const std::string_view key) noexcept
{
    try
    {
        const auto val = m_redis->get(std::string(key));
        if (val)
        {
            return *val;
        }
        return std::nullopt;
    }
    catch (const sw::redis::Error&)
    {
        return std::nullopt;
    }
}

bool RedisClient::del(const std::string_view key) noexcept
{
    try
    {
        uint64_t count = m_redis->del(std::string(key));
        return count > 0;
    }
    catch (const sw::redis::Error&)
    {
        return false;
    }
}

bool RedisClient::saveOrder(const Order& order) noexcept
{
    try
    {
        // Serialize domain order using FlatBuffers
        const auto buffer = FlatBuff::OrderFlatBufferAdapter::serialize(order);
        const std::string serializedData(reinterpret_cast<const char*>(buffer.data()), buffer.size());

        std::string key = "order:" + order.orderId();
        return set(key, serializedData);
    }
    catch (...)
    {
        return false;
    }
}

std::optional<Order> RedisClient::getOrder(const std::string_view orderId) noexcept
{
    try
    {
        const std::string key = "order:" + std::string(orderId);
        const auto data = get(key);
        if (!data)
        {
            return std::nullopt;
        }

        // Deserialize back to domain Order
        const auto* bytes = reinterpret_cast<const uint8_t*>(data->data());
        return FlatBuff::OrderFlatBufferAdapter::deserialize(bytes, data->size());
    }
    catch (...)
    {
        return std::nullopt;
    }
}

bool RedisClient::removeOrder(const std::string_view orderId) noexcept
{
    try
    {
        const std::string key = "order:" + std::string(orderId);
        return del(key);
    }
    catch (...)
    {
        return false;
    }
}

std::vector<std::string> RedisClient::getAllOrderIds() noexcept
{
    try
    {
        std::unordered_set<std::string> keys;
        m_redis->keys("order:*", std::inserter(keys, keys.end()));
        std::vector<std::string> orderIds;
        orderIds.reserve(keys.size());
        for (const auto& key : keys)
        {
            // Strip "order:" prefix (6 characters)
            if (key.rfind("order:", 0) == 0)
            {
                orderIds.push_back(key.substr(6));
            }
        }
        return orderIds;
    }
    catch (...)
    {
        return {};
    }
}

} // namespace Redis