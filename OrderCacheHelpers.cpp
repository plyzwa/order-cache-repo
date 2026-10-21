#include "OrderCacheHelpers.h"
#include "order.pb.h"


#include <string>

namespace Protobuff
{


// =================================== PROTOBUF ===================================
std::string serializeOrder(const Order& order)
{
    trading::Order protoOrder;

    // Map scalar fields
    protoOrder.set_order_id(order.orderId());
    protoOrder.set_security_id(order.securityId());
    protoOrder.set_qty(static_cast<uint32_t>(order.qty()));
    protoOrder.set_user(order.user());
    protoOrder.set_company(order.company());

    if (order.side() == "Buy")
    {
        protoOrder.set_side(trading::SIDE_BUY);
    }
    else if (order.side() == "Sell")
    {
        protoOrder.set_side(trading::SIDE_SELL);
    }
    else
    {
        protoOrder.set_side(trading::SIDE_UNSPECIFIED);
    }

    std::string binaryData;
    protoOrder.SerializeToString(&binaryData);

    return binaryData;
}

Order deserializeOrder(const std::string& binaryData)
{
    trading::Order protoOrder;

    if (!protoOrder.ParseFromString(binaryData))
    {
        throw std::runtime_error("Failed to parse Order from protobuf binary stream!");
    }

    std::string sideStr = "Buy";
    if (protoOrder.side() == trading::SIDE_SELL)
    {
        sideStr = "Sell";
    }
    else if (protoOrder.side() == trading::SIDE_UNSPECIFIED)
    {
        sideStr = "Unspecified";
    }

    Order order{protoOrder.order_id(),
        protoOrder.security_id(),
        sideStr,
        static_cast<unsigned int>(protoOrder.qty()),
        protoOrder.user(),
        protoOrder.company()};

    return std::move(order);
}

} // Protobuff