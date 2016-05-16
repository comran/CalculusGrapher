#include "calculus/server.h"

#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <complex>

#include "seasocks/Server.h"

#include "aos/linux_code/init.h"
#include "aos/common/logging/logging.h"
#include "aos/common/time.h"
#include "aos/common/util/phased_loop.h"
#include "aos/common/mutex.h"

#include "calculus/embedded.h"

namespace y2016 {
namespace calculus {

DataCollector::DataCollector()
    : cur_raw_data_("no data"),
      sample_id_(0),
      measure_index_(0),
      overflow_id_(1) {}

void DataCollector::RunIteration() {
  ::aos::MutexLocker locker(&mutex_);
  measure_index_ = 0;

  // Add recorded data here. /////
  AddPoint("test", ::std::rand() % 4);
  AddPoint("test2", ::std::rand() % 3);
  AddPoint("test3", ::std::rand() % 3 - 1);

  // Get ready for next iteration. /////
  sample_id_++;
}

void DataCollector::AddPoint(const ::std::string &name, double value) {
  // Mutex should be locked when this method is called to synchronize packets.
  CHECK(mutex_.OwnedBySelf());

  size_t index = GetIndex(sample_id_);

  ItemDatapoint datapoint{value, ::aos::time::Time::Now()};
  if (measure_index_ >= sample_items_.size()) {
    // New item in our data table.
    ::std::vector<ItemDatapoint> datapoints;
    SampleItem item{name, datapoints};
    sample_items_.emplace_back(item);
  } else if (index >= sample_items_.at(measure_index_).datapoints.size()) {
    // New data point for an already existing item.
    sample_items_.at(measure_index_).datapoints.emplace_back(datapoint);
  } else {
    // Overwrite an already existing data point for an already existing item.
    sample_items_.at(measure_index_).datapoints.at(index) = datapoint;
  }

  measure_index_++;
}

::std::string DataCollector::Fetch(int32_t from_sample) {
  ::aos::MutexLocker locker(&mutex_);

  ::std::stringstream message;
  message.precision(10);

  // Send out the names of each item when requested by the client.
  // Example: *item_one_name,item_two_name,item_three_name
  if (from_sample == 0) {
    message << "*";  // Begin name packet.

    // Add comma-separated list of names.
    for (size_t cur_data_name = 0; cur_data_name < sample_items_.size();
         cur_data_name++) {
      if (cur_data_name > 0) {
        message << ",";
      }
      message << sample_items_.at(cur_data_name).name;
    }
    return message.str();
  }

  // Send out one sample containing the data.
  // Samples are split with dollar signs, info with percent signs, and
  // measurements with commas.
  // Example of data with two samples: $289%2803.13%10,67$290%2803.14%12,68

  // Note that we are ignoring the from_sample being sent to keep up with the
  // live data without worrying about client lag.
  int32_t cur_sample = sample_id_;
  int32_t adjusted_index = GetIndex(cur_sample);
  message << "$";  // Begin data packet.

  // Make sure we are not out of range.
  if (sample_items_.size() > 0) {
    if (static_cast<size_t>(adjusted_index) <
        sample_items_.at(0).datapoints.size()) {
      message << cur_sample << "%"
              << sample_items_.at(0)
                     .datapoints.at(adjusted_index)
                     .time.ToSeconds() << "%";  // Send time.
      // Add comma-separated list of data points.
      for (size_t cur_measure = 0; cur_measure < sample_items_.size();
           cur_measure++) {
        if (cur_measure > 0) {
          message << ",";
        }
        message << sample_items_.at(cur_measure)
                       .datapoints.at(adjusted_index)
                       .value;
      }
    }
  }

  return message.str();
}

size_t DataCollector::GetIndex(size_t sample_id) {
  return sample_id % overflow_id_;
}

void DataCollector::operator()() {
  ::aos::SetCurrentThreadName("DataCollector");

  while (run_) {
    ::aos::time::PhasedLoopXMS(100, 0);
    RunIteration();
  }
}

SocketHandler::SocketHandler()
    : data_collector_thread_(::std::ref(data_collector_)) {}

void SocketHandler::onConnect(seasocks::WebSocket *connection) {
  connections_.insert(connection);
  LOG(INFO, "Connected: %s : %s\n", connection->getRequestUri().c_str(),
      seasocks::formatAddress(connection->getRemoteAddress()).c_str());
}

void SocketHandler::onData(seasocks::WebSocket *connection, const char *data) {
  int32_t from_sample = atoi(data);

  ::std::string send_data = data_collector_.Fetch(from_sample);
  connection->send(send_data.c_str());
}

void SocketHandler::onDisconnect(seasocks::WebSocket *connection) {
  connections_.erase(connection);
  LOG(INFO, "Disconnected: %s : %s\n", connection->getRequestUri().c_str(),
      seasocks::formatAddress(connection->getRemoteAddress()).c_str());
}

void SocketHandler::Quit() {
  data_collector_.Quit();
  data_collector_thread_.join();
}

SeasocksLogger::SeasocksLogger(Level min_level_to_log)
    : PrintfLogger(min_level_to_log) {}

void SeasocksLogger::log(Level level, const char *message) {
  // Convert Seasocks error codes to AOS.
  log_level aos_level;
  switch (level) {
    case seasocks::Logger::INFO:
      aos_level = INFO;
      break;
    case seasocks::Logger::WARNING:
      aos_level = WARNING;
      break;
    case seasocks::Logger::ERROR:
    case seasocks::Logger::SEVERE:
      aos_level = ERROR;
      break;
    case seasocks::Logger::DEBUG:
    case seasocks::Logger::ACCESS:
    default:
      aos_level = DEBUG;
      break;
  }
  LOG(aos_level, "Seasocks: %s\n", message);
}

}  // namespace calculus
}  // namespace y2016

int main(int, char *[]) {
  ::aos::InitNRT();

  ::seasocks::Server server(::std::shared_ptr<seasocks::Logger>(
      new ::y2016::calculus::SeasocksLogger(seasocks::Logger::INFO)));
  ::y2016::calculus::SocketHandler socket_handler;

  server.addWebSocketHandler(
      "/ws",
      ::std::shared_ptr<::y2016::calculus::SocketHandler>(&socket_handler));
  server.serve(".", 1180);

  socket_handler.Quit();

  ::aos::Cleanup();
  return 0;
}
