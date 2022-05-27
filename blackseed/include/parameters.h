#pragma once

#include "unique_sqlite.hpp"
#include <curl/curl.h>
#include <fstream>
#include <map>
#include <string>
#include <vector>

// Stores all the parameters defined
// in the configuration file.
struct Parameters {

  std::vector<std::string> exchangeNames;
  std::vector<double> fees;
  std::vector<bool> canShort;
  std::vector<bool> isImplemented;
  std::vector<std::string> tickerUrl;

  CURL *curl;
  double spreadEntry;
  double spreadTarget;
  unsigned maxLength;
  double priceDeltaLim;
  double trailingLim;
  unsigned trailingCount;
  double orderBookFactor;
  bool isDemoMode;
  std::string leg1;
  std::string leg2;
  bool verbose;
  std::ofstream *logFile;
  unsigned interval;
  unsigned debugMaxIteration;
  bool useFullExposure;
  double testedExposure;
  double maxExposure;
  bool useVolatility;
  unsigned volatilityPeriod;
  std::string cacert;

  std::string bitfinexApi;
  std::string bitfinexSecret;
  double bitfinexFees;
  bool bitfinexEnable;
  std::string okcoinApi;
  std::string okcoinSecret;
  double okcoinFees;
  bool okcoinEnable;
  std::string bitstampClientId;
  std::string bitstampApi;
  std::string bitstampSecret;
  double bitstampFees;
  bool bitstampEnable;
  std::string geminiApi;
  std::string geminiSecret;
  double geminiFees;
  bool geminiEnable;
  std::string krakenApi;

  std::string krakenSecret;
  double krakenFees;
  bool krakenEnable;
  std::string itbitApi;
  std::string itbitSecret;
  double itbitFees;
  bool itbitEnable;
  std::string wexApi;
  std::string wexSecret;
  double wexFees;
  bool wexEnable;
  std::string poloniexApi;
  std::string poloniexSecret;
  double poloniexFees;
  bool poloniexEnable;
  std::string coinbaseApi;
  std::string coinbaseSecret;
  std::string coinbasePhrase;
  double coinbaseFees;
  bool coinbaseEnable;
  std::string quadrigaApi;
  std::string quadrigaSecret;
  std::string quadrigaClientId;
  double quadrigaFees;
  bool quadrigaEnable;
  std::string exmoApi;
  std::string exmoSecret;
  double exmoFees;
  bool exmoEnable;
  std::string cexioClientId;
  std::string cexioApi;
  std::string cexioSecret;
  double cexioFees;
  bool cexioEnable;
  std::string bittrexApi;
  std::string bittrexSecret;
  double bittrexFees;
  bool bittrexEnable;
  std::string binanceApi;
  std::string binanceSecret;
  double binanceFees;
  bool binanceEnable;

  bool sendEmail;
  std::string senderAddress;
  std::string senderUsername;
  std::string senderPassword;
  std::string smtpServerAddress;
  std::string receiverAddress;

  std::string dbFile;
  unique_sqlite dbConn;

  Parameters(std::string fileName);

  void addExchange(std::string const &exchangeName, double const fee,
                   bool const shorting, bool const featureImplemented);

  std::vector<std::string>::size_type nbExch() const {
    return exchangeNames.size();
  }
};

// Copies the parameters from the configuration file
// to the Parameter structure.
void readAllParameters(std::ifstream &configFile,
                       std::map<std::string, std::string> &dataMap);
