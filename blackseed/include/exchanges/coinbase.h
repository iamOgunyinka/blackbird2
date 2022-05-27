#ifndef GDAX_H
#define GDAX_H

#include "quote_t.h"
#include <string>

struct json_t;
struct Parameters;

namespace coinbase {

quote_t getQuote(Parameters &params);

double getAvail(Parameters &params, std::string const &currency);

double getActivePos(Parameters &params);

double getLimitPrice(Parameters &params, double const volume, bool const isBid);

std::string sendLongOrder(Parameters &params, std::string const &direction,
                          double const quantity, double const price);

bool isOrderComplete(Parameters &params, std::string const &orderId);

json_t *authRequest(Parameters &params, std::string const &method,
                    std::string const &request, std::string const &options);

std::string gettime();

void testGDAX();

} // namespace coinbase

#endif
