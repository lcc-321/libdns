// created by lcc 12/19/2021

#include "lib_dns/client.h"

int main() {
  if ("0.1" != lib_dns::VERSION) {
    return 1;
  }

  bool stop = false;

  lib_dns::Client client;

  client.query("one.one.one.one", lib_dns::RRS.at("A"), [&stop](const std::vector<std::string>& data) {
    stop = true;

    bool found = false;
    for (const auto& row : data) {
      if ("1.1.1.1" == row) {
        found = true;
      }
    }

    if (found) {
      exit(0);
    } else {
      exit(1);
    }
  });

  while (!stop) { client.receive(); }
}
