#pragma once

#include <sw/redis++/redis++.h>
#include <string>
#include <string_view>
#include <optional>
#include <chrono>
#include <memory>

#include "OrderCacheHelpers.h"

class Order;

namespace Redis
{

    struct Config
    {
        std::string host = "127.0.0.1";
        int port = 6379;
        std::chrono::milliseconds connectTimeout{100};
        std::chrono::milliseconds socketTimeout{100};
        std::size_t poolSize = 4;
    };

    class RedisClient
    {
    public:
        
        explicit RedisClient(const Config& config = Config{});

        bool ping() noexcept;

        bool set(const std::string_view key, const std::string_view value,
                std::chrono::milliseconds ttl = std::chrono::milliseconds{0}) noexcept;

        std::optional<std::string> get(const std::string_view key) noexcept;

        bool del(const std::string_view key) noexcept;

        bool saveOrder(const Order& order) noexcept;
        std::optional<Order> getOrder(const std::string_view orderId) noexcept;
        bool removeOrder(const std::string_view orderId) noexcept;

        std::vector<std::string> getAllOrderIds() noexcept;


    private:
            std::unique_ptr<sw::redis::Redis> m_redis;

    };


    






} // Redis