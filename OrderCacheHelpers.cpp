#include "OrderCacheHelpers.h"
#include "order.pb.h"

#include <string>


// =================================== PROTOBUFFER ===================================
namespace Protobuff
{

std::string serializeOrder(const Order& order)
{
    trading::Order protoOrder;

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
        throw std::runtime_error("Failed to parse Order from protobuf binary stream.");
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


// =================================== FLATBUFFER ===================================

namespace FlatBuff
{

FBS::OrderSide OrderFlatBufferAdapter::toFbsSide(const std::string_view side)
{
    if (side == "Buy")  return FBS::OrderSide_BUY;
    if (side == "Sell") return FBS::OrderSide_SELL;
    throw std::invalid_argument("Unknown order side: " + std::string(side));
}

std::string OrderFlatBufferAdapter::fromFbsSide(const FBS::OrderSide& side) 
{
    switch (side)
    {
        case FBS::OrderSide_BUY:  return "Buy";
        case FBS::OrderSide_SELL: return "Sell";
        default:                  return "Unspecified";
    }
}

std::vector<uint8_t> OrderFlatBufferAdapter::serialize(const Order& order)
{
    flatbuffers::FlatBufferBuilder builder(256);

    // Step A: All variable-length payloads (strings) MUST be created first
    const auto orderIdOffset = builder.CreateString(order.orderId());
    const auto secIdOffset   = builder.CreateString(order.securityId());
    const auto userOffset    = builder.CreateString(order.user());
    const auto compOffset    = builder.CreateString(order.company());
    const auto sideEnum      = toFbsSide(order.side());

    // Step B: Build the table offsets
    FBS::FlatOrderBuilder orderBuilder(builder);
    orderBuilder.add_order_id(orderIdOffset);
    orderBuilder.add_security_id(secIdOffset);
    orderBuilder.add_side(sideEnum);
    orderBuilder.add_qty(order.qty());
    orderBuilder.add_user(userOffset);
    orderBuilder.add_company(compOffset);

    const auto root = orderBuilder.Finish();
    builder.Finish(root);

    const uint8_t* ptr = builder.GetBufferPointer();
    return std::vector<uint8_t>(ptr, ptr + builder.GetSize());
}

const FBS::FlatOrder* OrderFlatBufferAdapter::verifyAndAccess(const uint8_t* buffer, const size_t size)
{
    if (!buffer || size == 0)
    {
        throw std::invalid_argument("Buffer pointer is null or size is 0");
    }

    flatbuffers::Verifier verifier(buffer, size);
    if (FBS::VerifyFlatOrderBuffer(verifier) == false)
    {
        throw std::runtime_error("Corrupted or malformed FlatBuffer byte stream detected");
    }

    return FBS::GetFlatOrder(buffer);
}

Order OrderFlatBufferAdapter::deserialize(const uint8_t* buffer, const size_t size)
{
    const auto* flatOrder = verifyAndAccess(buffer, size);

    Order order{
        flatOrder->order_id()   ? flatOrder->order_id()->str()   : "",
        flatOrder->security_id()? flatOrder->security_id()->str(): "",
        fromFbsSide(flatOrder->side()),
        flatOrder->qty(),
        flatOrder->user()       ? flatOrder->user()->str()       : "",
        flatOrder->company()    ? flatOrder->company()->str()    : ""
        };

        return order;
}

} // namespace FlatBuff