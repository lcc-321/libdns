// created by lcc 12/19/2021

#include "lib_dns/client.h"
#include "rapidjson/document.h"

#include <cassert>
#include <iostream>
#include <random>

#include <arpa/inet.h>

int main() {
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

    const std::string path = "/w/api.php?action=query&format=json&list=random&rnnamespace=0";
    for (int i = 0; i < 9; i ++) {
      // get wikipedia random title
      client.send_https_request(AF_INET, ip, host, path, [&client, ip, host](std::vector<std::vector<char>> res) {
        const std::string body(res[1].begin(), res[1].end());
        assert(!body.empty());
        rapidjson::Document data;
        data.Parse(body.c_str());
        assert(data["query"].IsObject() && data["query"]["random"].IsArray() && data["query"]["random"][0].IsObject());
        const std::string title = data["query"]["random"][0]["title"].GetString();
        assert(!title.empty());
        std::cout << "Random title: " << title << '\n';

        // get article of wikipedia random title
        const std::string pathP = "/w/api.php?action=parse&format=json&page=" + lib_dns::url_encode(title);
        client.send_https_request(AF_INET, ip, host, pathP, [](std::vector<std::vector<char>> resP) {
          const std::string bodyP(resP[1].begin(), resP[1].end());
          assert(!bodyP.empty());
          rapidjson::Document dataP;
          dataP.Parse(bodyP.c_str());
          assert(dataP["parse"].IsObject() && dataP["parse"]["text"].IsObject() && dataP["parse"]["text"]["*"].IsString());
          const std::string textP = dataP["parse"]["text"]["*"].GetString();
          assert(!textP.empty());
          // deal with article
        });
      });
    }
  });


  for (int i = 0; i < 36; i++) { client.receive(9); }

  return 0;
}
