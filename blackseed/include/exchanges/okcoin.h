#ifndef OKCOIN_H
#define OKCOIN_H

#include "quote_t.h"
#include <string>

struct json_t;
struct Parameters;

namespace OKCoin {

quote_t getQuote(Parameters &params);

double getAvail(Parameters &params, std::string const &currency);

std::string sendLongOrder(Parameters &params, std::string const &direction,
                          double const quantity, double const price);

std::string sendShortOrder(Parameters &params, std::string const &direction,
                           double const quantity, double const price);

bool isOrderComplete(Parameters &params, std::string const &orderId);

double getActivePos(Parameters &params);

double getLimitPrice(Parameters &params, double const volume, bool const isBid);

json_t *authRequest(Parameters &params, std::string const &url,
                    std::string const &signature, std::string const &content);

void getBorrowInfo(Parameters &params);

int borrowBtc(Parameters &params, double const amount);

void repayBtc(Parameters &params, int const borrowId);

} // namespace OKCoin

#endif
