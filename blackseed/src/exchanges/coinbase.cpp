#include "coinbase.h"
#include "openssl/hmac.h"
#include "openssl/sha.h"
#include "parameters.h"
#include "unique_json.hpp"
#include "utils/base64.h"
#include "utils/gettime.hpp"
#include "utils/restapi.h"
#include <array>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <vector>

namespace coinbase {

static RestApi &queryHandle(Parameters &params) {
  static RestApi query("https://api.exchange.coinbase.com",
                       params.cacert.c_str(), *params.logFile);
  return query;
}

quote_t getQuote(Parameters &params) {
  auto &exchange = queryHandle(params);
  std::string pair;
  pair = "/products/";
  pair += params.leg1.c_str();
  pair += "-";
  pair += params.leg2.c_str();
  pair += "/ticker";
  curl_slist *headerList = nullptr;
  headerList = curl_slist_append(headerList, "Content-Type: application/json");
  RestApi::unique_slist sheaderList(headerList);
  unique_json root{exchange.getRequest(pair, std::move(sheaderList))};
  const char *bid = nullptr;
  char const *ask = nullptr;
  int unpack_fail =
      json_unpack(root.get(), "{s:s, s:s}", "bid", &bid, "ask", &ask);
  if (unpack_fail) {
    bid = "0";
    ask = "0";
  }
  if (bid && ask)
    return std::make_pair(std::stod(bid), std::stod(ask));
  return std::make_pair(0.0, 0.0);
}

double getAvail(Parameters &params, std::string const &currency) {
  unique_json root{authRequest(params, "GET", "/accounts", "")};
  size_t arraySize = json_array_size(root.get());
  double available = 0.0;
  const char *currstr;
  for (size_t i = 0; i < arraySize; i++) {
    std::string tmpCurrency = json_string_value(
        json_object_get(json_array_get(root.get(), i), "currency"));
    if (tmpCurrency.compare(currency.c_str()) == 0) {
      currstr = json_string_value(
          json_object_get(json_array_get(root.get(), i), "available"));
      if (currstr != NULL) {
        available = atof(currstr);
      } else {
        *params.logFile << "<coinbase> Error with currency string" << std::endl;
        available = 0.0;
      }
    }
  }
  return available;
}

double getActivePos(Parameters &params) {
  // TODO: this is not really a good way to get active positions
  return getAvail(params, "BTC");
}

double getLimitPrice(Parameters &params, double const volume,
                     bool const isBid) {
  auto &exchange = queryHandle(params);
  // TODO: Build a real URL with leg1 leg2 and auth post it
  // FIXME: using level 2 order book - has aggregated data but should be
  // sufficient for now.
  unique_json root{exchange.getRequest("/products/BTC-USD/book?level=2")};
  auto bidask = json_object_get(root.get(), isBid ? "bids" : "asks");
  *params.logFile << "<coinbase> Looking for a limit price to fill "
                  << std::setprecision(8) << fabs(volume) << " Legx...\n";
  double tmpVol = 0.0;
  double p = 0.0;
  double v;
  int i = 0;
  while (tmpVol < fabs(volume) * params.orderBookFactor) {
    p = atof(json_string_value(json_array_get(json_array_get(bidask, i), 0)));
    v = atof(json_string_value(json_array_get(json_array_get(bidask, i), 1)));
    *params.logFile << "<coinbase> order book: " << std::setprecision(8) << v
                    << " @$" << std::setprecision(8) << p << std::endl;
    tmpVol += v;
    i++;
  }
  return p;
}

std::string sendLongOrder(Parameters &params, std::string const &direction,
                          double const quantity, double const price) {
  if (direction.compare("buy") != 0 && direction.compare("sell") != 0) {
    *params.logFile << "<coinbase> Error: Neither \"buy\" nor \"sell\" selected"
                    << std::endl;
    return "0";
  }
  *params.logFile << "<coinbase> Trying to send a \"" << direction
                  << "\" limit order: " << std::setprecision(8) << quantity
                  << " @ $" << std::setprecision(8) << price << "...\n";
  std::string pair = "BTC-USD";
  std::string type = direction;
  char buff[300];
  snprintf(buff, 300,
           "{\"size\":\"%.8f\",\"price\":\"%.8f\",\"side\":\"%s\",\"product_"
           "id\": \"%s\"}",
           quantity, price, type.c_str(), pair.c_str());
  unique_json root{authRequest(params, "POST", "/orders", buff)};
  auto txid = json_string_value(json_object_get(root.get(), "id"));

  *params.logFile << "<coinbase> Done (transaction ID: " << txid << ")\n"
                  << std::endl;
  return txid;
}

bool isOrderComplete(Parameters &params, std::string const &orderId) {

  unique_json root{authRequest(params, "GET", "/orders", "")};
  size_t arraySize = json_array_size(root.get());
  bool complete = true;
  const char *idstr;
  for (size_t i = 0; i < arraySize; i++) {
    std::string tmpId =
        json_string_value(json_object_get(json_array_get(root.get(), i), "id"));
    if (tmpId.compare(orderId.c_str()) == 0) {
      idstr = json_string_value(
          json_object_get(json_array_get(root.get(), i), "status"));
      *params.logFile << "<coinbase> Order still open (Status:" << idstr << ")"
                      << std::endl;
      complete = false;
    }
  }
  return complete;
}

json_t *authRequest(Parameters &params, std::string const &method,
                    std::string const &request, std::string const &options) {
  // create timestamp

  // static uint64_t nonce = time(nullptr);
  std::string nonce = gettime();
  // create data string

  std::string post_data = nonce + method + request + options;

  // if (!options.empty())
  //  post_data += options;

  // create decoded key

  std::string decoded_key = base64_decode(params.coinbaseSecret);

  // Message Signature using HMAC-SHA256 of (NONCE+ METHOD??? + PATH + body)

  uint8_t *hmac_digest =
      HMAC(EVP_sha256(), decoded_key.c_str(), decoded_key.length(),
           reinterpret_cast<const uint8_t *>(post_data.data()),
           post_data.size(), NULL, NULL);

  // encode the HMAC to base64
  std::string api_sign_header =
      base64_encode(hmac_digest, SHA256_DIGEST_LENGTH);

  // cURL header

  std::array<std::string, 5> headers{
      "CB-ACCESS-KEY:" + params.coinbaseApi,
      "CB-ACCESS-SIGN:" + api_sign_header,
      "CB-ACCESS-TIMESTAMP:" + nonce,
      "CB-ACCESS-PASSPHRASE:" + params.coinbasePhrase,
      "Content-Type: application/json; charset=utf-8",
  };

  // cURL request
  auto &exchange = queryHandle(params);

  // TODO: this sucks please do something better
  if (method.compare("GET") == 0) {
    return exchange.getRequest(
        request, make_slist(std::begin(headers), std::end(headers)));
  } else if (method.compare("POST") == 0) {
    return exchange.postRequest(
        request, make_slist(std::begin(headers), std::end(headers)), options);
  } else {
    std::cout << "Error With Auth method. Exiting with code 0" << std::endl;
    exit(0);
  }
}

std::string gettime() {
  timeval curTime;
  time_t &tv_sec = *reinterpret_cast<time_t *>(&curTime.tv_sec);
  cgettimeofday(&curTime, NULL);
  int milli = curTime.tv_usec / 1000;
  char buffer[80]{};
  strftime(buffer, 80, "%Y-%m-%dT%H:%M:%S", gmtime(&tv_sec));
  char time_buffer2[200]{};
  snprintf(time_buffer2, 200, "%s.%d000Z", buffer, milli);
  snprintf(time_buffer2, 200, "%sZ", buffer);

  // return time_buffer2;

  time_t result = time(NULL);
  long long result2 = result;
  char buff4[40]{};
  snprintf(buff4, 40, "%lld", result2);

  char buff5[40]{};
  snprintf(buff5, 40, "%lld.%d000", result2, milli);
  return buff5;
}

void testcoinbase() {

  Parameters params("bird.conf");
  params.logFile = new std::ofstream("./test.log", std::ofstream::trunc);

  std::string orderId;

  // std::cout << "Current value LEG1_LEG2 bid: " << getQuote(params).bid() <<
  // std::endl; std::cout << "Current value LEG1_LEG2 ask: " <<
  // getQuote(params).ask() << std::endl; std::cout << "Current balance BTC: "
  // << getAvail(params, "BTC") << std::endl; std::cout << "Current balance USD:
  // "
  // << getAvail(params, "USD")<< std::endl; std::cout << "Current balance ETH:
  // "
  // << getAvail(params, "ETH")<< std::endl; std::cout << "Current balance BTC:
  // "
  // << getAvail(params, "BCH")<< std::endl; std::cout << "current bid limit
  // price for 10 units: " << getLimitPrice(params, 10 , true) << std::endl;
  // std::cout << "Current ask limit price for .09 units: " <<
  // getLimitPrice(params, 0.09, false) << std::endl; std::cout << "Sending buy
  // order for 0.005 BTC @ ASK! USD - TXID: " << std::endl; orderId =
  // sendLongOrder(params, "buy", 0.005, getLimitPrice(params,.005,false));
  // std::cout << orderId << std::endl;
  // std::cout << "Buy Order is complete: " << isOrderComplete(params, orderId)
  // << std::endl;

  // std::cout << "Sending sell order for 0.02 BTC @ 10000 USD - TXID: " <<
  // std::endl; orderId = sendLongOrder(params, "sell", 0.02, 10000); std::cout
  // << orderId << std::endl; std::cout << "Sell order is complete: " <<
  // isOrderComplete(params, orderId) << std::endl; std::cout << "Active
  // Position: " << getActivePos(params);
}
} // namespace coinbase
