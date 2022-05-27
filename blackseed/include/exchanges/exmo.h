#ifndef EXMO_H
#define EXMO_H

#include "quote_t.h"
#include <string>

struct json_t;
struct Parameters;

namespace Exmo {

quote_t getQuote(Parameters &params);

double getAvail(Parameters &params, std::string const &currency);

std::string sendLongOrder(Parameters &params, std::string const &direction,
                          double const quantity, double const price);
// TODO multi currency support
// std::string sendLongOrder(Parameters& params, std::string direction, double
// quantity, double price, std::string pair = "btc_usd");

bool isOrderComplete(Parameters &params, std::string const &orderId);

double getActivePos(Parameters &params);

double getLimitPrice(Parameters &params, double const volume, bool const isBid);

void testExmo();

} // namespace Exmo

#endif
