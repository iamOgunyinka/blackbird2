#ifndef BINANCE_H
#define BINANCE_H

#include "quote_t.h"
#include <string>

struct Parameters;

namespace Binance {

quote_t getQuote(Parameters &params);

double getAvail(Parameters &params, std::string const &currency);

std::string sendLongOrder(Parameters &params, std::string const &direction,
                          double const quantity, double const price);

std::string sendShortOrder(Parameters &params, std::string const &direction,
                           double const quantity, double const price);

bool isOrderComplete(Parameters &params, std::string const &orderId);

double getActivePos(Parameters &params);

double getLimitPrice(Parameters &params, double const volume, bool const isBid);

void testBinance();
} // namespace Binance

#endif
