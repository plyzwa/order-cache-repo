#pragma once

#include "OrderCache.h"


namespace Protobuff
{
    std::string serializeOrder(const Order &domainOrder);
    Order deserializeOrder(const std::string& binaryData);
} // Protobuff
