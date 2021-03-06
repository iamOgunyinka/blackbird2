#include "bitcoin.h"
#include "result.h"
#include "time_fun.h"
#include "curl_fun.h"
#include "db_fun.h"
#include "parameters.h"
#include "check_entry_exit.h"
#include "exchanges/bitfinex.h"
#include "exchanges/okcoin.h"
#include "exchanges/bitstamp.h"
#include "exchanges/gemini.h"
#include "exchanges/kraken.h"
#include "exchanges/quadrigacx.h"
#include "exchanges/itbit.h"
#include "exchanges/wex.h"
#include "exchanges/poloniex.h"
#include "exchanges/coinbase.h"
#include "exchanges/exmo.h"
#include "exchanges/cexio.h"
#include "exchanges/bittrex.h"
#include "exchanges/binance.h"
#include "utils/send_email.h"
#include "getpid.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <curl/curl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>

// The 'typedef' declarations needed for the function arrays
// These functions contain everything needed to communicate with
// the exchanges, like getting the quotes or the active positions.
// Each function is implemented in the files located in the 'exchanges' folder.

using getQuoteType = quote_t (*)(Parameters &);
using getAvailType = double (*)(Parameters &, std::string const &currency);
using sendOrderType = std::string (*)(Parameters &params,
                                      std::string const &direction,
                                      double const quantity,
                                      double const price);
using isOrderCompleteType = bool (*)(Parameters &, std::string const &orderId);
using getActivePosType = double (*)(Parameters &);
using getLimitPriceType = double (*)(Parameters &, double const volume,
                                     bool const isBid);

// This structure contains the balance of both exchanges,
// *before* and *after* an arbitrage trade.
// This is used to compute the performance of the trade,
// by comparing the balance before and after the trade.

struct Balance {
  double leg1 = 0.0;
  double leg2 = 0.0;
  double leg1After = 0.0;
  double leg2After = 0.0;
};

struct callback_t {
  getQuoteType getQuote = nullptr;
  getAvailType getAvail = nullptr;
  sendOrderType sendLongOrder = nullptr;
  sendOrderType sendShortOrder = nullptr;
  isOrderCompleteType isOrderComplete = nullptr;
  getActivePosType getActivePos = nullptr;
  getLimitPriceType getLimitPrice = nullptr;
  std::string dbTableName = nullptr;
};

// 'main' function.
// Blackbird doesn't require any arguments for now.
int main(int argc, char **argv) {
  std::cout << "Blackbird Bitcoin Arbitrage" << std::endl;
  std::cout << "DISCLAIMER: USE THE SOFTWARE AT YOUR OWN RISK\n" << std::endl;
  // Replaces the C++ global locale with the user-preferred locale
  std::locale mylocale("");
  // Loads all the parameters
  Parameters params("blackbird.conf");
  // Does some verifications about the parameters
  if (!params.isDemoMode) {
    if (!params.useFullExposure) {
      if (params.testedExposure < 10.0 && params.leg2.compare("USD") == 0) {
        // TODO do the same check for other currencies. Is there a limi?
        std::cout << "ERROR: Minimum USD needed: $10.00" << std::endl;
        std::cout << "       Otherwise some exchanges will reject the orders\n"
                  << std::endl;
        exit(EXIT_FAILURE);
      }
      if (params.testedExposure > params.maxExposure) {
        std::cout << "ERROR: Test exposure (" << params.testedExposure
                  << ") is above max exposure (" << params.maxExposure << ")\n"
                  << std::endl;
        exit(EXIT_FAILURE);
      }
    }
  }

  {
    // check if the params.ca_cert file exists, so we use it, otherwise
    // remove it
    std::ifstream iFile(params.cacert, std::ios::in);
    if (!iFile)
      params.cacert.clear();
  }

  // Connects to the SQLite3 database.
  // This database is used to collect bid and ask information
  // from the exchanges. Not really used for the moment, but
  // would be useful to collect historical bid/ask data.
  if (createDbConnection(params) != 0) {
    std::cerr << "ERROR: cannot connect to the database \'" << params.dbFile
              << "\'\n"
              << std::endl;
    exit(EXIT_FAILURE);
  }

  // We only trade BTC/USD for the moment
  if (params.leg1.compare("BTC") != 0 || params.leg2.compare("USD") != 0) {
    std::cout << "ERROR: Valid currency pair is only BTC/USD for now.\n"
              << std::endl;
    exit(EXIT_FAILURE);
  }

  // Function arrays containing all the exchanges functions
  // using the 'typedef' declarations from above.
  std::vector<callback_t> callbacks;

  // Adds the exchange functions to the arrays for all the defined exchanges
  if (params.bitfinexEnable &&
      (params.bitfinexApi.empty() == false || params.isDemoMode)) {
    params.addExchange("Bitfinex", params.bitfinexFees, true, true);
    callback_t bitfinexApiCallback;

    bitfinexApiCallback.getQuote = Bitfinex::getQuote;
    bitfinexApiCallback.getAvail = Bitfinex::getAvail;
    bitfinexApiCallback.sendLongOrder = Bitfinex::sendLongOrder;
    bitfinexApiCallback.sendShortOrder = Bitfinex::sendShortOrder;
    bitfinexApiCallback.isOrderComplete = Bitfinex::isOrderComplete;
    bitfinexApiCallback.getActivePos = Bitfinex::getActivePos;
    bitfinexApiCallback.getLimitPrice = Bitfinex::getLimitPrice;

    bitfinexApiCallback.dbTableName = "bitfinex";
    createTable(bitfinexApiCallback.dbTableName, params);
    callbacks.push_back(std::move(bitfinexApiCallback));
  }

  if (params.okcoinEnable &&
      (params.okcoinApi.empty() == false || params.isDemoMode)) {
    params.addExchange("OKCoin", params.okcoinFees, false, true);
    callback_t okcoinApiCallback;
    okcoinApiCallback.getQuote = OKCoin::getQuote;
    okcoinApiCallback.getAvail = OKCoin::getAvail;
    okcoinApiCallback.sendLongOrder = OKCoin::sendLongOrder;
    okcoinApiCallback.sendShortOrder = OKCoin::sendShortOrder;
    okcoinApiCallback.isOrderComplete = OKCoin::isOrderComplete;
    okcoinApiCallback.getActivePos = OKCoin::getActivePos;
    okcoinApiCallback.getLimitPrice = OKCoin::getLimitPrice;

    okcoinApiCallback.dbTableName = "okcoin";
    createTable(okcoinApiCallback.dbTableName, params);
    callbacks.push_back(std::move(okcoinApiCallback));
  }

  if (params.bitstampEnable &&
      (params.bitstampClientId.empty() == false || params.isDemoMode)) {
    params.addExchange("Bitstamp", params.bitstampFees, false, true);

    callback_t bitstampApiCallback;
    bitstampApiCallback.getQuote = Bitstamp::getQuote;
    bitstampApiCallback.getAvail = Bitstamp::getAvail;
    bitstampApiCallback.sendLongOrder = Bitstamp::sendLongOrder;
    bitstampApiCallback.isOrderComplete = Bitstamp::isOrderComplete;
    bitstampApiCallback.getActivePos = Bitstamp::getActivePos;
    bitstampApiCallback.getLimitPrice = Bitstamp::getLimitPrice;

    bitstampApiCallback.dbTableName = "bitstamp";
    createTable(bitstampApiCallback.dbTableName, params);
    callbacks.push_back(std::move(bitstampApiCallback));
  }

  if (params.geminiEnable &&
      (params.geminiApi.empty() == false || params.isDemoMode)) {
    params.addExchange("Gemini", params.geminiFees, false, true);

    callback_t geminiApiCallback;
    geminiApiCallback.getQuote = Gemini::getQuote;
    geminiApiCallback.getAvail = Gemini::getAvail;
    geminiApiCallback.sendLongOrder = Gemini::sendLongOrder;
    geminiApiCallback.isOrderComplete = Gemini::isOrderComplete;
    geminiApiCallback.getActivePos = Gemini::getActivePos;
    geminiApiCallback.getLimitPrice = Gemini::getLimitPrice;

    geminiApiCallback.dbTableName = "gemini";
    createTable(geminiApiCallback.dbTableName, params);
    callbacks.push_back(std::move(geminiApiCallback));
  }

  if (params.krakenEnable &&
      (params.krakenApi.empty() == false || params.isDemoMode)) {
    params.addExchange("Kraken", params.krakenFees, false, true);

    callback_t krakenApiCallback;
    krakenApiCallback.getQuote = Kraken::getQuote;
    krakenApiCallback.getAvail = Kraken::getAvail;
    krakenApiCallback.sendLongOrder = Kraken::sendLongOrder;
    krakenApiCallback.sendShortOrder = Kraken::sendShortOrder;
    krakenApiCallback.isOrderComplete = Kraken::isOrderComplete;
    krakenApiCallback.getActivePos = Kraken::getActivePos;
    krakenApiCallback.getLimitPrice = Kraken::getLimitPrice;

    krakenApiCallback.dbTableName = "kraken";
    createTable(krakenApiCallback.dbTableName, params);
    callbacks.push_back(std::move(krakenApiCallback));
  }

  if (params.itbitEnable &&
      (params.itbitApi.empty() == false || params.isDemoMode)) {
    params.addExchange("ItBit", params.itbitFees, false, false);
    callback_t itbitApiCallback;
    itbitApiCallback.getQuote = ItBit::getQuote;
    itbitApiCallback.getAvail = ItBit::getAvail;
    itbitApiCallback.getActivePos = ItBit::getActivePos;
    itbitApiCallback.getLimitPrice = ItBit::getLimitPrice;

    itbitApiCallback.dbTableName = "itbit";
    createTable(itbitApiCallback.dbTableName, params);

    callbacks.push_back(std::move(itbitApiCallback));
  }

  if (params.wexEnable &&
      (params.wexApi.empty() == false || params.isDemoMode)) {
    params.addExchange("WEX", params.wexFees, false, true);

    callback_t wexApiCallback;
    wexApiCallback.getQuote = WEX::getQuote;
    wexApiCallback.getAvail = WEX::getAvail;
    wexApiCallback.sendLongOrder = WEX::sendLongOrder;
    wexApiCallback.isOrderComplete = WEX::isOrderComplete;
    wexApiCallback.getActivePos = WEX::getActivePos;
    wexApiCallback.getLimitPrice = WEX::getLimitPrice;

    wexApiCallback.dbTableName = "wex";
    createTable(wexApiCallback.dbTableName, params);

    callbacks.push_back(std::move(wexApiCallback));
  }

  if (params.poloniexEnable &&
      (params.poloniexApi.empty() == false || params.isDemoMode)) {
    params.addExchange("Poloniex", params.poloniexFees, true, false);

    callback_t poloniexApiCallback;

    poloniexApiCallback.getQuote = Poloniex::getQuote;
    poloniexApiCallback.getAvail = Poloniex::getAvail;
    poloniexApiCallback.sendLongOrder = Poloniex::sendLongOrder;
    poloniexApiCallback.sendShortOrder = Poloniex::sendShortOrder;
    poloniexApiCallback.isOrderComplete = Poloniex::isOrderComplete;
    poloniexApiCallback.getActivePos = Poloniex::getActivePos;
    poloniexApiCallback.getLimitPrice = Poloniex::getLimitPrice;

    poloniexApiCallback.dbTableName = "poloniex";
    createTable(poloniexApiCallback.dbTableName, params);

    callbacks.push_back(std::move(poloniexApiCallback));
  }

  if (params.coinbaseEnable &&
      (params.coinbaseApi.empty() == false || params.isDemoMode)) {
    params.addExchange("CoinBasePro", params.coinbaseFees, false, true);

    callback_t coinbaseApiCallback;
    coinbaseApiCallback.getQuote = coinbase::getQuote;
    coinbaseApiCallback.getAvail = coinbase::getAvail;
    coinbaseApiCallback.getActivePos = coinbase::getActivePos;
    coinbaseApiCallback.getLimitPrice = coinbase::getLimitPrice;
    coinbaseApiCallback.sendLongOrder = coinbase::sendLongOrder;
    coinbaseApiCallback.isOrderComplete = coinbase::isOrderComplete;
    coinbaseApiCallback.dbTableName = "CoinbasePro";
    createTable(coinbaseApiCallback.dbTableName, params);

    callbacks.push_back(std::move(coinbaseApiCallback));
  }

  // Quadriga no longer trade crypto, so it's been removed here.

  if (params.exmoEnable &&
      (params.exmoApi.empty() == false || params.isDemoMode)) {
    params.addExchange("Exmo", params.exmoFees, false, true);

    callback_t exmoApiCallback;
    exmoApiCallback.getQuote = Exmo::getQuote;
    exmoApiCallback.getAvail = Exmo::getAvail;
    exmoApiCallback.sendLongOrder = Exmo::sendLongOrder;
    exmoApiCallback.isOrderComplete = Exmo::isOrderComplete;
    exmoApiCallback.getActivePos = Exmo::getActivePos;
    exmoApiCallback.getLimitPrice = Exmo::getLimitPrice;

    exmoApiCallback.dbTableName = "exmo";
    createTable(exmoApiCallback.dbTableName, params);

    callbacks.push_back(std::move(exmoApiCallback));
  }

  if (params.cexioEnable &&
      (params.cexioApi.empty() == false || params.isDemoMode)) {
    params.addExchange("Cexio", params.cexioFees, false, true);

    callback_t cexiApiCallback;
    cexiApiCallback.getQuote = Cexio::getQuote;
    cexiApiCallback.getAvail = Cexio::getAvail;
    cexiApiCallback.sendLongOrder = Cexio::sendLongOrder;
    cexiApiCallback.sendShortOrder = Cexio::sendShortOrder;
    cexiApiCallback.isOrderComplete = Cexio::isOrderComplete;
    cexiApiCallback.getActivePos = Cexio::getActivePos;
    cexiApiCallback.getLimitPrice = Cexio::getLimitPrice;

    cexiApiCallback.dbTableName = "cexio";
    createTable(cexiApiCallback.dbTableName, params);

    callbacks.push_back(std::move(cexiApiCallback));
  }

  if (params.bittrexEnable &&
      (params.bittrexApi.empty() == false || params.isDemoMode)) {
    params.addExchange("Bittrex", params.bittrexFees, false, true);

    callback_t bittrexApiCallback;

    bittrexApiCallback.getQuote = Bittrex::getQuote;
    bittrexApiCallback.getAvail = Bittrex::getAvail;
    bittrexApiCallback.sendLongOrder = Bittrex::sendLongOrder;
    bittrexApiCallback.sendShortOrder = Bittrex::sendShortOrder;
    bittrexApiCallback.isOrderComplete = Bittrex::isOrderComplete;
    bittrexApiCallback.getActivePos = Bittrex::getActivePos;
    bittrexApiCallback.getLimitPrice = Bittrex::getLimitPrice;
    bittrexApiCallback.dbTableName = "bittrex";
    createTable(bittrexApiCallback.dbTableName, params);

    callbacks.push_back(std::move(bittrexApiCallback));
  }

  if (params.binanceEnable &&
      (params.binanceApi.empty() == false || params.isDemoMode)) {
    params.addExchange("Binance", params.binanceFees, false, true);

    callback_t binanceApiCallback;
    binanceApiCallback.getQuote = Binance::getQuote;
    binanceApiCallback.getAvail = Binance::getAvail;
    binanceApiCallback.sendLongOrder = Binance::sendLongOrder;
    binanceApiCallback.sendShortOrder = Binance::sendShortOrder;
    binanceApiCallback.isOrderComplete = Binance::isOrderComplete;
    binanceApiCallback.getActivePos = Binance::getActivePos;
    binanceApiCallback.getLimitPrice = Binance::getLimitPrice;
    binanceApiCallback.dbTableName = "binance";
    createTable(binanceApiCallback.dbTableName, params);

    callbacks.push_back(std::move(binanceApiCallback));
  }

  // We need at least two exchanges to run Blackbird
  if (callbacks.size() < 2) {
    std::cout << "ERROR: Blackbird needs at least two Bitcoin exchanges. "
                 "Please edit the config.json file to add new exchanges\n"
              << std::endl;
    exit(EXIT_FAILURE);
  }
  // Creates the CSV file that will collect the trade results
  std::string currDateTime = printDateTimeFileName();
  std::string csvFileName = "output/blackbird_result_" + currDateTime + ".csv";
  std::ofstream csvFile(csvFileName, std::ofstream::trunc);
  csvFile
      << "TRADE_ID,EXCHANGE_LONG,EXHANGE_SHORT,ENTRY_TIME,EXIT_TIME,DURATION,"
      << "TOTAL_EXPOSURE,BALANCE_BEFORE,BALANCE_AFTER,RETURN" << std::endl;
  // Creates the log file where all events will be saved
  std::string logFileName = "output/blackbird_log_" + currDateTime + ".log";
  std::ofstream logFile(logFileName, std::ofstream::trunc);
  logFile.imbue(mylocale);
  logFile.precision(2);
  logFile << std::fixed;
  params.logFile = &logFile;
  // Log file header
  logFile << "--------------------------------------------" << std::endl;
  logFile << "|   Blackbird Bitcoin Arbitrage Log File   |" << std::endl;
  logFile << "--------------------------------------------\n" << std::endl;
  logFile << "Blackbird started on " << printDateTime() << "\n" << std::endl;

  logFile << "Connected to database \'" << params.dbFile << "\'\n" << std::endl;

  if (params.isDemoMode) {
    logFile << "Demo mode: trades won't be generated\n" << std::endl;
  }

  // Shows which pair we are trading (BTC/USD only for the moment)
  logFile << "Pair traded: " << params.leg1 << "/" << params.leg2 << "\n"
          << std::endl;

  std::cout << "Log file generated: " << logFileName
            << "\nBlackbird is running... (pid " << cgetpid() << ")\n"
            << std::endl;

  // The btcVec vector contains details about every exchange,
  // like fees, as specified in bitcoin.h
  std::vector<Bitcoin> btcVec;
  btcVec.reserve(callbacks.size());
  // Creates a new Bitcoin structure within btcVec for every exchange we want to
  // trade on
  for (size_t i = 0; i < callbacks.size(); ++i) {
    btcVec.push_back(Bitcoin(i, params.exchangeNames[i], params.fees[i],
                             params.canShort[i], params.isImplemented[i]));
  }

  // Inits cURL connections
  params.curl = curl_easy_init();
  // Shows the spreads
  logFile << "[ Targets ]\n"
          << std::setprecision(2)
          << "   Spread Entry:  " << params.spreadEntry * 100.0 << "%\n"
          << "   Spread Target: " << params.spreadTarget * 100.0 << "%\n";

  // SpreadEntry and SpreadTarget have to be positive,
  // Otherwise we will loose money on every trade.
  if (params.spreadEntry <= 0.0) {
    logFile << "   WARNING: Spread Entry should be positive" << std::endl;
  }
  if (params.spreadTarget <= 0.0) {
    logFile << "   WARNING: Spread Target should be positive" << std::endl;
  }
  logFile << std::endl;
  logFile << "[ Current balances ]" << std::endl;
  // Gets the the balances from every exchange
  // This is only done when not in Demo mode.
  std::vector<Balance> balance(callbacks.size());
  if (!params.isDemoMode)
    std::transform(callbacks.begin(), callbacks.end(), begin(balance),
                   [&params](auto &callbackData) {
                     Balance tmp{};
                     tmp.leg1 = callbackData.getAvail(params, "btc");
                     tmp.leg2 = callbackData.getAvail(params, "usd");
                     return tmp;
                   });

  // Checks for a restore.txt file, to see if
  // the program exited with an open position.
  Result res;
  res.reset();
  bool inMarket = res.loadPartialResult("restore.txt");

  // Writes the current balances into the log file
  for (size_t i = 0; i < callbacks.size(); ++i) {
    logFile << "   " << params.exchangeNames[i] << ":\t";
    if (params.isDemoMode) {
      logFile << "n/a (demo mode)" << std::endl;
    } else if (!params.isImplemented[i]) {
      logFile << "n/a (API not implemented)" << std::endl;
    } else {
      logFile << std::setprecision(2) << balance[i].leg2 << " " << params.leg2
              << "\t" << std::setprecision(6) << balance[i].leg1 << " "
              << params.leg1 << std::endl;
    }
    if (balance[i].leg1 > 0.0050 && !inMarket) { // FIXME: hard-coded number
      logFile << "ERROR: All " << params.leg1
              << " accounts must be empty before starting Blackbird"
              << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  logFile << std::endl;
  logFile << "[ Cash exposure ]\n";
  if (params.isDemoMode) {
    logFile << "   No cash - Demo mode\n";
  } else {
    if (params.useFullExposure) {
      logFile << "   FULL exposure used!\n";
    } else {
      logFile << "   TEST exposure used\n   Value: " << std::setprecision(2)
              << params.testedExposure << '\n';
    }
  }
  logFile << std::endl;
  // Code implementing the loop function, that runs
  // every 'Interval' seconds.
  time_t rawtime = time(nullptr);
  tm timeinfo = *std::localtime(&rawtime);
  using std::this_thread::sleep_for;
  using millisecs = std::chrono::milliseconds;
  using secs = std::chrono::seconds;
  // Waits for the next 'interval' seconds before starting the loop
  while ((int)timeinfo.tm_sec % params.interval != 0) {
    sleep_for(millisecs(100));
    time(&rawtime);
    timeinfo = *std::localtime(&rawtime);
  }
  if (!params.verbose) {
    logFile << "Running..." << std::endl;
  }

  int resultId = 0;
  unsigned currIteration = 0;
  bool stillRunning = true;
  time_t currTime;
  time_t diffTime;

  // Main analysis loop
  while (stillRunning) {
    currTime = mktime(&timeinfo);
    time(&rawtime);
    diffTime = difftime(rawtime, currTime);
    // Checks if we are already too late in the current iteration
    // If that's the case we wait until the next iteration
    // and we show a warning in the log file.
    if (diffTime > 0) {
      logFile << "WARNING: " << diffTime << " second(s) too late at "
              << printDateTime(currTime) << std::endl;
      timeinfo.tm_sec +=
          (ceil(diffTime / params.interval) + 1) * params.interval;
      currTime = mktime(&timeinfo);
      sleep_for(secs(params.interval - (diffTime % params.interval)));
      logFile << std::endl;
    } else if (diffTime < 0) {
      sleep_for(secs(-diffTime));
    }
    // Header for every iteration of the loop
    if (params.verbose) {
      if (!inMarket) {
        logFile << "[ " << printDateTime(currTime) << " ]" << std::endl;
      } else {
        logFile << "[ " << printDateTime(currTime) << " IN MARKET: Long "
                << res.exchNameLong << " / Short " << res.exchNameShort << " ]"
                << std::endl;
      }
    }
    // Gets the bid and ask of all the exchanges
    for (int i = 0; i < callbacks.size(); ++i) {
      auto &callbackData = callbacks[i];
      auto quote = callbackData.getQuote(params);
      double bid = quote.bid();
      double ask = quote.ask();
      std::cout << params.exchangeNames[i]
                << " --> "
                   "Bid: "
                << bid << ", Ask: " << ask << std::endl;

      // Saves the bid/ask into the SQLite database
      addBidAskToDb(callbackData.dbTableName, printDateTimeDb(currTime), bid,
                    ask, params);

      // If there is an error with the bid or ask (i.e. value is null),
      // we show a warning but we don't stop the loop.
      if (bid == 0.0) {
        logFile << "   WARNING: " << params.exchangeNames[i] << " bid is null"
                << std::endl;
      }
      if (ask == 0.0) {
        logFile << "   WARNING: " << params.exchangeNames[i] << " ask is null"
                << std::endl;
      }
      // Shows the bid/ask information in the log file
      if (params.verbose) {
        logFile << "   " << params.exchangeNames[i] << ": \t"
                << std::setprecision(2) << bid << " / " << ask << std::endl;
      }
      // Updates the Bitcoin vector with the latest bid/ask data
      btcVec[i].updateData(quote);
      curl_easy_reset(params.curl);
    }
    if (params.verbose) {
      logFile << "   ----------------------------" << std::endl;
    }
    // Stores all the spreads in arrays to
    // compute the volatility. The volatility
    // is not used for the moment.
    if (params.useVolatility) {
      for (int i = 0; i < callbacks.size(); ++i) {
        for (int j = 0; j < callbacks.size(); ++j) {
          if (i != j) {
            if (btcVec[j].getHasShort()) {
              double longMidPrice = btcVec[i].getMidPrice();
              double shortMidPrice = btcVec[j].getMidPrice();
              if (longMidPrice > 0.0 && shortMidPrice > 0.0) {
                if (res.volatility[i][j].size() >= params.volatilityPeriod) {
                  res.volatility[i][j].pop_back();
                }
                res.volatility[i][j].push_front((longMidPrice - shortMidPrice) /
                                                longMidPrice);
              }
            }
          }
        }
      }
    }
    // Looks for arbitrage opportunities on all the exchange combinations
    if (!inMarket) {
      for (int i = 0; i < callbacks.size(); ++i) {
        for (int j = 0; j < callbacks.size(); ++j) {
          if (i != j) {
            if (checkEntry(&btcVec[i], &btcVec[j], res, params)) {
              // An entry opportunity has been found!
              res.exposure = (std::min)(balance[res.idExchLong].leg2,
                                        balance[res.idExchShort].leg2);
              if (params.isDemoMode) {
                logFile << "INFO: Opportunity found but no trade will be "
                           "generated (Demo mode)"
                        << std::endl;
                break;
              }
              if (res.exposure == 0.0) {
                logFile << "WARNING: Opportunity found but no cash available. "
                           "Trade canceled"
                        << std::endl;
                break;
              }
              if (params.useFullExposure == false &&
                  res.exposure <= params.testedExposure) {
                logFile << "WARNING: Opportunity found but no enough cash. "
                           "Need more than TEST cash (min. $"
                        << std::setprecision(2) << params.testedExposure
                        << "). Trade canceled" << std::endl;
                break;
              }
              if (params.useFullExposure) {
                // Removes 1% of the exposure to have
                // a little bit of margin.
                res.exposure -= 0.01 * res.exposure;
                if (res.exposure > params.maxExposure) {
                  logFile << "WARNING: Opportunity found but exposure ("
                          << std::setprecision(2) << res.exposure
                          << ") above the limit\n"
                          << "         Max exposure will be used instead ("
                          << params.maxExposure << ")" << std::endl;
                  res.exposure = params.maxExposure;
                }
              } else {
                res.exposure = params.testedExposure;
              }
              // Checks the volumes and, based on that, computes the limit
              // prices that will be sent to the exchanges
              double volumeLong =
                  res.exposure / btcVec[res.idExchLong].getAsk();
              double volumeShort =
                  res.exposure / btcVec[res.idExchShort].getBid();
              double limPriceLong = callbacks[res.idExchLong].getLimitPrice(
                  params, volumeLong, false);
              double limPriceShort = callbacks[res.idExchShort].getLimitPrice(
                  params, volumeShort, true);
              if (limPriceLong == 0.0 || limPriceShort == 0.0) {
                logFile
                    << "WARNING: Opportunity found but error with the order "
                       "books (limit price is null). Trade canceled\n";
                logFile.precision(2);
                logFile << "         Long limit price:  " << limPriceLong
                        << std::endl;
                logFile << "         Short limit price: " << limPriceShort
                        << std::endl;
                res.trailing[res.idExchLong][res.idExchShort] = -1.0;
                break;
              }
              if (limPriceLong - res.priceLongIn > params.priceDeltaLim ||
                  res.priceShortIn - limPriceShort > params.priceDeltaLim) {
                logFile << "WARNING: Opportunity found but not enough "
                           "liquidity. Trade canceled\n";
                logFile.precision(2);
                logFile << "         Target long price:  " << res.priceLongIn
                        << ", Real long price:  " << limPriceLong << std::endl;
                logFile << "         Target short price: " << res.priceShortIn
                        << ", Real short price: " << limPriceShort << std::endl;
                res.trailing[res.idExchLong][res.idExchShort] = -1.0;
                break;
              }
              // We are in market now, meaning we have positions on leg1 (the
              // hedged on) We store the details of that first trade into the
              // Result structure.
              inMarket = true;
              resultId++;
              res.id = resultId;
              res.entryTime = currTime;
              res.priceLongIn = limPriceLong;
              res.priceShortIn = limPriceShort;
              res.printEntryInfo(*params.logFile);
              res.maxSpread[res.idExchLong][res.idExchShort] = -1.0;
              res.minSpread[res.idExchLong][res.idExchShort] = 1.0;
              res.trailing[res.idExchLong][res.idExchShort] = 1.0;

              // Send the orders to the two exchanges
              auto longOrderId = callbacks[res.idExchLong].sendLongOrder(
                  params, "buy", volumeLong, limPriceLong);
              auto shortOrderId = callbacks[res.idExchShort].sendShortOrder(
                  params, "sell", volumeShort, limPriceShort);
              logFile << "Waiting for the two orders to be filled..."
                      << std::endl;
              sleep_for(millisecs(5000));
              bool isLongOrderComplete =
                  callbacks[res.idExchLong].isOrderComplete(params,
                                                            longOrderId);
              bool isShortOrderComplete =
                  callbacks[res.idExchShort].isOrderComplete(params,
                                                             shortOrderId);
              // Loops until both orders are completed
              while (!isLongOrderComplete || !isShortOrderComplete) {
                sleep_for(millisecs(3000));
                if (!isLongOrderComplete) {
                  logFile << "Long order on "
                          << params.exchangeNames[res.idExchLong]
                          << " still open..." << std::endl;
                  isLongOrderComplete =
                      callbacks[res.idExchLong].isOrderComplete(params,
                                                                longOrderId);
                }
                if (!isShortOrderComplete) {
                  logFile << "Short order on "
                          << params.exchangeNames[res.idExchShort]
                          << " still open..." << std::endl;
                  isShortOrderComplete =
                      callbacks[res.idExchShort].isOrderComplete(params,
                                                                 shortOrderId);
                }
              }
              // Both orders are now fully executed
              logFile << "Done" << std::endl;

              // Stores the partial result to file in case
              // the program exits before closing the position.
              res.savePartialResult("restore.txt");

              // Resets the order ids
              longOrderId = "0";
              shortOrderId = "0";
              break;
            }
          }
        }
        if (inMarket) {
          break;
        }
      }
      if (params.verbose) {
        logFile << std::endl;
      }
    } else if (inMarket) {
      // We are in market and looking for an exit opportunity
      if (checkExit(&btcVec[res.idExchLong], &btcVec[res.idExchShort], res,
                    params, currTime)) {
        // An exit opportunity has been found!
        // We check the current leg1 exposure
        std::vector<double> btcUsed(callbacks.size());
        for (int i = 0; i < callbacks.size(); ++i) {
          btcUsed[i] = callbacks[i].getActivePos(params);
        }
        // Checks the volumes and computes the limit prices that will be sent to
        // the exchanges
        double volumeLong = btcUsed[res.idExchLong];
        double volumeShort = btcUsed[res.idExchShort];
        double limPriceLong =
            callbacks[res.idExchLong].getLimitPrice(params, volumeLong, true);
        double limPriceShort = callbacks[res.idExchShort].getLimitPrice(
            params, volumeShort, false);
        if (limPriceLong == 0.0 || limPriceShort == 0.0) {
          logFile << "WARNING: Opportunity found but error with the order "
                     "books (limit price is null). Trade canceled\n";
          logFile.precision(2);
          logFile << "         Long limit price:  " << limPriceLong
                  << std::endl;
          logFile << "         Short limit price: " << limPriceShort
                  << std::endl;
          res.trailing[res.idExchLong][res.idExchShort] = 1.0;
        } else if (res.priceLongOut - limPriceLong > params.priceDeltaLim ||
                   limPriceShort - res.priceShortOut > params.priceDeltaLim) {
          logFile << "WARNING: Opportunity found but not enough liquidity. "
                     "Trade canceled\n";
          logFile.precision(2);
          logFile << "         Target long price:  " << res.priceLongOut
                  << ", Real long price:  " << limPriceLong << std::endl;
          logFile << "         Target short price: " << res.priceShortOut
                  << ", Real short price: " << limPriceShort << std::endl;
          res.trailing[res.idExchLong][res.idExchShort] = 1.0;
        } else {
          res.exitTime = currTime;
          res.priceLongOut = limPriceLong;
          res.priceShortOut = limPriceShort;
          res.printExitInfo(*params.logFile);

          logFile.precision(6);
          logFile << params.leg1 << " exposure on "
                  << params.exchangeNames[res.idExchLong] << ": " << volumeLong
                  << '\n'
                  << params.leg1 << " exposure on "
                  << params.exchangeNames[res.idExchShort] << ": "
                  << volumeShort << '\n'
                  << std::endl;
          auto longOrderId = callbacks[res.idExchLong].sendLongOrder(
              params, "sell", fabs(btcUsed[res.idExchLong]), limPriceLong);
          auto shortOrderId = callbacks[res.idExchShort].sendShortOrder(
              params, "buy", fabs(btcUsed[res.idExchShort]), limPriceShort);
          logFile << "Waiting for the two orders to be filled..." << std::endl;
          sleep_for(millisecs(5000));
          bool isLongOrderComplete = callbacks[res.idExchLong].
              isOrderComplete(params, longOrderId);
          bool isShortOrderComplete =
              callbacks[res.idExchShort].isOrderComplete(params, shortOrderId);
          // Loops until both orders are completed
          while (!isLongOrderComplete || !isShortOrderComplete) {
            sleep_for(millisecs(3000));
            if (!isLongOrderComplete) {
              logFile << "Long order on "
                      << params.exchangeNames[res.idExchLong]
                      << " still open..." << std::endl;
              isLongOrderComplete = callbacks[res.idExchLong].isOrderComplete(
                  params, longOrderId);
            }
            if (!isShortOrderComplete) {
              logFile << "Short order on "
                      << params.exchangeNames[res.idExchShort]
                      << " still open..." << std::endl;
              isShortOrderComplete = callbacks[res.idExchShort].isOrderComplete(
                  params, shortOrderId);
            }
          }
          logFile << "Done\n" << std::endl;
          longOrderId = "0";
          shortOrderId = "0";
          inMarket = false;

          size_t const numExch = callbacks.size();

          for (size_t i = 0; i < numExch; ++i) {
            balance[i].leg2After = callbacks[i].getAvail(
                params, "usd"); // FIXME: currency hard-coded
            balance[i].leg1After = callbacks[i].getAvail(
                params, "btc"); // FIXME: currency hard-coded
          }
          for (int i = 0; i < numExch; ++i) {
            logFile << "New balance on " << params.exchangeNames[i] << ":  \t";
            logFile.precision(2);
            logFile << balance[i].leg2After << " " << params.leg2 << " (perf "
                    << balance[i].leg2After - balance[i].leg2 << "), ";
            logFile << std::setprecision(6) << balance[i].leg1After << " "
                    << params.leg1 << "\n";
          }
          logFile << std::endl;
          // Update total leg2 balance
          for (int i = 0; i < numExch; ++i) {
            res.leg2TotBalanceBefore += balance[i].leg2;
            res.leg2TotBalanceAfter += balance[i].leg2After;
          }
          // Update current balances
          for (int i = 0; i < numExch; ++i) {
            balance[i].leg2 = balance[i].leg2After;
            balance[i].leg1 = balance[i].leg1After;
          }
          // Prints the result in the result CSV file
          logFile.precision(2);
          logFile << "ACTUAL PERFORMANCE: "
                  << "$" << res.leg2TotBalanceAfter - res.leg2TotBalanceBefore
                  << " (" << res.actualPerf() * 100.0 << "%)\n"
                  << std::endl;
          csvFile << res.id << "," << res.exchNameLong << ","
                  << res.exchNameShort << "," << printDateTimeCsv(res.entryTime)
                  << "," << printDateTimeCsv(res.exitTime) << ","
                  << res.getTradeLengthInMinute() << "," << res.exposure * 2.0
                  << "," << res.leg2TotBalanceBefore << ","
                  << res.leg2TotBalanceAfter << "," << res.actualPerf()
                  << std::endl;
          // Sends an email with the result of the trade
          if (params.sendEmail) {
            sendEmail(res, params);
            logFile << "Email sent" << std::endl;
          }
          res.reset();
          // Removes restore.txt since this trade is done.
          std::ofstream resFile("restore.txt", std::ofstream::trunc);
          resFile.close();
        }
      }
      if (params.verbose)
        logFile << '\n';
    }
    // Moves to the next iteration, unless
    // the maxmum is reached.
    timeinfo.tm_sec += params.interval;
    currIteration++;
    if (currIteration >= params.debugMaxIteration) {
      logFile << "Max iteration reached (" << params.debugMaxIteration << ")"
              << std::endl;
      stillRunning = false;
    }
    // Exits if a 'stop_after_notrade' file is found
    // Warning: by default on GitHub the file has a underscore
    // at the end, so Blackbird is not stopped by default.
    std::ifstream infile("stop_after_notrade");
    if (infile && !inMarket) {
      logFile << "Exit after last trade (file stop_after_notrade found)\n";
      stillRunning = false;
    }
  }
  // Analysis loop exited, does some cleanup
  curl_easy_cleanup(params.curl);
  csvFile.close();
  logFile.close();

  return 0;
}
