#ifndef GEMINI_H
#define GEMINI_H

#include "quote_t.h"
#include <string>

struct json_t;
struct Parameters;

namespace Gemini {

quote_t getQuote(Parameters &params);

double getAvail(Parameters &params, std::string const &currency);

std::string sendLongOrder(Parameters &params, std::string const &direction,
                          double const quantity, double const price);

bool isOrderComplete(Parameters &params, std::string const &orderId);

double getActivePos(Parameters &params);

double getLimitPrice(Parameters &params, double const volume, bool const isBid);

json_t *authRequest(Parameters &params, std::string const &url,
                    std::string const &request, std::string const &options);

} // namespace Gemini

#endif
