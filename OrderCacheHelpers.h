#pragma once

#include "OrderCache.h"
#include "order_generated.h"

#include <vector>
#include <string>
#include <string_view>
#include <cstdint>
#include <stdexcept>


namespace Protobuff
{
    std::string serializeOrder(const Order &domainOrder);
    Order deserializeOrder(const std::string& binaryData);
} // Protobuff

namespace FlatBuff
{


    class OrderFlatBufferAdapter
    {

    public:
    
    static FBS::OrderSide toFbsSide(const std::string_view side);
    static std::string fromFbsSide(const FBS::OrderSide& side);

    static std::vector<uint8_t> serialize(const Order& order);

    static const FBS::FlatOrder* verifyAndAccess(const uint8_t* buffer, const size_t size);

    static Order deserialize(const uint8_t* buffer, const size_t size);
};



}
