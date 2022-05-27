#ifndef CEXIO_H
#define CEXIO_H

#include "quote_t.h"
#include <sstream>
#include <string>

struct json_t;
struct Parameters;

namespace Cexio {

quote_t getQuote(Parameters &params);

double getAvail(Parameters &params, std::string const &currency);

std::string sendLongOrder(Parameters &params, std::string const &direction,
                          double const quantity, double const price);

std::string sendShortOrder(Parameters &params, std::string const &direction,
                           double const quantity, double const price);

std::string sendOrder(Parameters &params, std::string const &direction,
                      double const quantity, double const price);

std::string openPosition(Parameters &params, std::string const &direction,
                         double const quantity, double const price);

std::string closePosition(Parameters &params);

bool isOrderComplete(Parameters &params, std::string const &orderId);

double getActivePos(Parameters &params);

double getLimitPrice(Parameters &params, double const volume, bool const isBid);

void testCexio();

} // namespace Cexio

#endif
