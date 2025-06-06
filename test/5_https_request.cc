// created by lcc 12/19/2021

#include "lib_dns/client.h"
#include "simdjson.h"

#include <cassert>
#include <iostream>
#include <random>

#include <arpa/inet.h>

int main() {
  auto doc = lib_dns::Client::json_parse(R"({"a": 1,"b":{"bb":"abc"},"c":[3,2,1]})");
  assert(static_cast<int64_t>(doc.find_field("a")) == 1);
  assert(static_cast<int64_t>(doc.at_path("$.a")) == 1);
  assert(doc.at_path("$.b.bb").type() == simdjson::ondemand::json_type::string);
  assert(doc.at_path("$.b.bb") == "abc");
  std::string bb; doc.at_path("$.b.bb").get_string(bb);
  assert(bb == "abc");
  assert(doc.at_path("$.c").type() == simdjson::ondemand::json_type::array);
  long sum = 0;
  for (auto cc : doc.at_path("$.c").get_array()) {
    sum += cc.get_int64();
  }
  assert(sum == 6);

  auto client = lib_dns::Client();

  auto rand_engine = std::mt19937((std::random_device())());

  for (int i = 0; i < 9; i ++) {
    client.query("google.com", 1, [&client, &rand_engine, i](const std::vector<std::string> &dns_data) {
      assert(!dns_data.empty());
      const std::string ip = dns_data[(std::uniform_int_distribution<std::size_t>(0, dns_data.size() - 1))(rand_engine)];
      std::cout << "IP: " << ip << '\n';

      std::string path = std::string("/?") + std::to_string(i);
      client.send_https_request(LIB_DNS_WITH_IPV6 ? AF_INET6 : AF_INET, ip, "google.com", path, [path](std::vector<std::vector<char>> res) {
        const std::string body(res[1].begin(), res[1].end());
        assert(body.find(std::string(path)) != std::string::npos);
      });
    });
  }

  std::string host = "en.wikipedia.org";
  client.query(host, 1, [&client, host](const std::vector<std::string> &dns_data) {
    assert(!dns_data.empty());
    const std::string& ip = dns_data[0];

    const std::string path = "/w/api.php?action=query&format=json&list=random&rn" "namespace=0";
    for (int i = 0; i < 9; i ++) {
      // get wikipedia random title
      client.send_https_request(AF_INET, ip, host, path, [&client, ip, host](std::vector<std::vector<char>> res) {
        const std::string body(res[1].begin(), res[1].end());
        assert(!body.empty());
        auto data = lib_dns::Client::json_parse(body);
        assert(data.at_path("$.query.random.0.title").type() == simdjson::ondemand::json_type::string);
        std::string title; data.at_path("$.query.random.0.title").get_string(title);
        assert(!title.empty());
        std::cout << "Random title: " << title << '\n';

        // get article of wikipedia random title
        const std::string pathP = "/w/api.php?action=parse&format=json&page=" + lib_dns::url_encode(title);
        client.send_https_request(AF_INET, ip, host, pathP, [](std::vector<std::vector<char>> resP) {
          const std::string bodyP(resP[1].begin(), resP[1].end());
          assert(!bodyP.empty());
          auto dataP = lib_dns::Client::json_parse(bodyP);
          assert(dataP.at_path("$.parse.text.*").type() == simdjson::ondemand::json_type::string);
          std::string textP; dataP.at_path("$.parse.text.*").get_string(textP);
          assert(!textP.empty());
          // deal with article
        });
      });
    }
  });

  for (int i = 0; i < 99; i++) { client.receive(9); }

  return 0;
}
