#include "parameters.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <optional>

static std::string findConfigFile(std::string fileName) {
  // local directory
  {
    std::ifstream configFile(fileName);

    // Keep the first match
    if (configFile.good()) {
      return fileName;
    }
  }

  // Unix user settings directory
  {
    char *home = getenv("HOME");

    if (home) {
      std::string prefix = std::string(home) + "/.config";
      std::string fullpath = prefix + "/" + fileName;
      std::ifstream configFile(fullpath);

      // Keep the first match
      if (configFile.good()) {
        return fullpath;
      }
    }
  }

  // Windows user settings directory
  {
    char *appdata = getenv("APPDATA");

    if (appdata) {
      std::string prefix = std::string(appdata);
      std::string fullpath = prefix + "/" + fileName;
      std::ifstream configFile(fullpath);

      // Keep the first match
      if (configFile.good()) {
        return fullpath;
      }
    }
  }

  // Unix system settings directory
  {
    std::string fullpath = "/etc/" + fileName;
    std::ifstream configFile(fullpath);

    // Keep the first match
    if (configFile.good()) {
      return fullpath;
    }
  }

  // We have to return something, even though we already know this will
  // fail
  return fileName;
}

template <typename Type>
void getParameter(std::string parameter,
                  std::map<std::string, std::string> const &dataMap,
                  Type &data) {
  auto iter = dataMap.find(parameter);
  if (iter == dataMap.cend())
    std::exit(-1);
  if constexpr (std::is_same_v<bool, Type>) {
    data = iter->second == "true";
  } else if constexpr (std::is_integral_v<Type>) {
    data = 0;
    if (!iter->second.empty())
      data = std::stoi(iter->second);
  } else if constexpr (std::is_same_v<double, Type>) {
    data = 0.0;
    if (!iter->second.empty())
      data = std::stod(iter->second);
  } else {
    data = iter->second;
  }
}

Parameters::Parameters(std::string fileName) {
  std::map<std::string, std::string> dataMap;
  {
    std::ifstream configFile(findConfigFile(fileName));
    if (!configFile.is_open()) {
      std::cout << "ERROR: " << fileName << " cannot be open.\n";
      exit(EXIT_FAILURE);
    }
    readAllParameters(configFile, dataMap);
    if (dataMap.empty())
      std::exit(-1);
  }

  getParameter("SpreadEntry", dataMap, spreadEntry);
  getParameter("SpreadTarget", dataMap, spreadTarget);
  getParameter("MaxLength", dataMap, maxLength);
  getParameter("PriceDeltaLimit", dataMap, priceDeltaLim);
  getParameter("TrailingSpreadLim", dataMap, trailingLim);
  getParameter("TrailingSpreadCount", dataMap, trailingCount);
  getParameter("OrderBookFactor", dataMap, orderBookFactor);
  getParameter("DemoMode", dataMap, isDemoMode);
  getParameter("Leg1", dataMap, leg1);
  getParameter("Leg2", dataMap, leg2);

  getParameter("Verbose", dataMap, verbose);
  getParameter("Interval", dataMap, interval);
  getParameter("DebugMaxIteration", dataMap, debugMaxIteration);
  getParameter("UseFullExposure", dataMap, useFullExposure);
  getParameter("TestedExposure", dataMap, testedExposure);
  getParameter("MaxExposure", dataMap, maxExposure);
  getParameter("UseVolatility", dataMap, useVolatility);
  getParameter("VolatilityPeriod", dataMap, volatilityPeriod);
  getParameter("CACert", dataMap, cacert);
  getParameter("BitfinexApiKey", dataMap, bitfinexApi);
  getParameter("BitfinexSecretKey", dataMap, bitfinexSecret);
  getParameter("BitfinexFees", dataMap, bitfinexFees);
  getParameter("BitfinexEnable", dataMap, bitfinexEnable);
  getParameter("OkCoinApiKey", dataMap, okcoinApi);
  getParameter("OkCoinSecretKey", dataMap, okcoinSecret);
  getParameter("OkCoinFees", dataMap, okcoinFees);
  getParameter("OkCoinEnable", dataMap, okcoinEnable);
  getParameter("BitstampClientId", dataMap, bitstampClientId);
  getParameter("BitstampApiKey", dataMap, bitstampApi);
  getParameter("BitstampSecretKey", dataMap, bitstampSecret);
  getParameter("BitstampFees", dataMap, bitstampFees);
  getParameter("BitstampEnable", dataMap, bitstampEnable);
  getParameter("GeminiApiKey", dataMap, geminiApi);
  getParameter("GeminiSecretKey", dataMap, geminiSecret);
  getParameter("GeminiFees", dataMap, geminiFees);
  getParameter("GeminiEnable", dataMap, geminiEnable);
  getParameter("KrakenApiKey", dataMap, krakenApi);
  getParameter("KrakenSecretKey", dataMap, krakenSecret);
  getParameter("KrakenFees", dataMap, krakenFees);
  getParameter("KrakenEnable", dataMap, krakenEnable);
  getParameter("ItBitApiKey", dataMap, itbitApi);
  getParameter("ItBitSecretKey", dataMap, itbitSecret);
  getParameter("ItBitFees", dataMap, itbitFees);
  getParameter("ItBitEnable", dataMap, itbitEnable);
  getParameter("WEXApiKey", dataMap, wexApi);
  getParameter("WEXSecretKey", dataMap, wexSecret);
  getParameter("WEXFees", dataMap, wexFees);
  getParameter("WEXEnable", dataMap, wexEnable);
  getParameter("PoloniexApiKey", dataMap, poloniexApi);
  getParameter("PoloniexSecretKey", dataMap, poloniexSecret);
  getParameter("PoloniexFees", dataMap, poloniexFees);
  getParameter("PoloniexEnable", dataMap, poloniexEnable);
  getParameter("GDAXApiKey", dataMap, coinbaseApi);
  getParameter("GDAXSecretKey", dataMap, coinbaseSecret);
  getParameter("GDAXPhrase", dataMap, coinbasePhrase);
  getParameter("GDAXFees", dataMap, coinbaseFees);
  getParameter("GDAXEnable", dataMap, coinbaseEnable);
  getParameter("QuadrigaApiKey", dataMap, quadrigaApi);
  getParameter("QuadrigaSecretKey", dataMap, quadrigaSecret);
  getParameter("QuadrigaFees", dataMap, quadrigaFees);
  getParameter("QuadrigaClientId", dataMap, quadrigaClientId);
  getParameter("QuadrigaEnable", dataMap, quadrigaEnable);
  getParameter("ExmoApiKey", dataMap, exmoApi);
  getParameter("ExmoSecretKey", dataMap, exmoSecret);
  getParameter("ExmoFees", dataMap, exmoFees);
  getParameter("ExmoEnable", dataMap, exmoEnable);
  getParameter("CexioClientId", dataMap, cexioClientId);
  getParameter("CexioApiKey", dataMap, cexioApi);
  getParameter("CexioSecretKey", dataMap, cexioSecret);
  getParameter("CexioFees", dataMap, cexioFees);
  getParameter("CexioEnable", dataMap, cexioEnable);
  getParameter("BittrexApiKey", dataMap, bittrexApi);
  getParameter("BittrexSecretKey", dataMap, bittrexSecret);
  getParameter("BittrexFees", dataMap, bittrexFees);
  getParameter("BittrexEnable", dataMap, bittrexEnable);
  getParameter("BinanceApiKey", dataMap, binanceApi);
  getParameter("BinanceSecretKey", dataMap, binanceSecret);
  getParameter("BinanceFees", dataMap, binanceFees);
  getParameter("BinanceEnable", dataMap, binanceEnable);

  getParameter("SendEmail", dataMap, sendEmail);
  getParameter("SenderAddress", dataMap, senderAddress);
  getParameter("SenderUsername", dataMap, senderUsername);
  getParameter("SenderPassword", dataMap, senderPassword);
  getParameter("SmtpServerAddress", dataMap, smtpServerAddress);
  getParameter("ReceiverAddress", dataMap, receiverAddress);
  getParameter("DBFile", dataMap, dbFile);
}

void Parameters::addExchange(std::string const &exchangeName, double const fee,
                             bool const shorting,
                             bool const featureImplemented) {
  exchangeNames.push_back(exchangeName);
  fees.push_back(fee);
  canShort.push_back(shorting);
  isImplemented.push_back(featureImplemented);
}

void ltrim(std::string &s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}

void rtrim(std::string &s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}

void trim(std::string &s) {
  ltrim(s);
  rtrim(s);
}

std::string trimCopy(std::string s) {
  trim(s);
  return s;
}

void readAllParameters(std::ifstream &configFile,
                       std::map<std::string, std::string> &dataMap) {
  assert(configFile);
  std::string line;
  configFile.clear();
  configFile.seekg(0);

  while (std::getline(configFile, line)) {
    trim(line);
    if (!line.empty() && line[0] != '#') {
      auto const key = trimCopy(line.substr(0, line.find('=')));
      auto const value =
          trimCopy(line.substr(line.find('=') + 1, line.length()));
      if (!key.empty())
        dataMap[key] = value;
    }
  }
}
