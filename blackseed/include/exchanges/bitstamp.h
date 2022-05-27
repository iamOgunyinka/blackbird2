#ifndef BITSTAMP_H
#define BITSTAMP_H

#include "quote_t.h"

#include <string>

struct json_t;
struct Parameters;

namespace Bitstamp {

quote_t getQuote(Parameters &params);

double getAvail(Parameters &params, std::string const &currency);

std::string sendLongOrder(Parameters &params, std::string const &direction,
                          double const quantity, double const price);

bool isOrderComplete(Parameters &params, std::string const &orderId);

double getActivePos(Parameters &params);

double getLimitPrice(Parameters &params, double const volume, bool const isBid);

} // namespace Bitstamp

#endif
