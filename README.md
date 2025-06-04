# lib_dns [![Build Status](https://app.travis-ci.com/lcc-321/libdns.svg?branch=main)](https://app.travis-ci.com/lcc-321/libdns)
a tiny DNS client library asynchronously(use epoll or kqueue) for C++ (especially in docker alpine)

## How to start
```
# Clone this project
git clone --recurse-submodules https://github.com/lcc-321/libdns.git

# Enter this project & update submodule
cd libdns

# Configure
cmake -G Ninja -S . -B build

# Build
cmake --build build

# Test
ctest --test-dir build

# Run an example to query DNS A record of google.com 
./build/lib_dns_example google.com A
```

## Example Code
#### See detail: [example/main.cc](https://github.com/lcc-321/libdns/blob/main/example/main.cc), or see an actual usage: https://github.com/lcc-321/tdscript
```C++
std::vector<std::pair<std::string, std::string>> params = {  // for test
    { "google.com", "AAAA" },
    { "google.com", "A" },
    { "wikipedia.org", "TXT" },
    { "wikipedia.org", "AAAA" },
    { "en.wikipedia.org", "A" },
};

lib_dns::Client client;

bool stop = false;

for (const auto& param : params) {
  // query adn get callback
  client.query(param.first, lib_dns::RRS.at(param.second), [](std::vector<std::string> data) {
    for (const auto& row : data) {
      std::cout << row << '\n';
    }
    
    stop = true;
  });
}

while (!stop) {  // event loop
  client.receive();
}
```

### Output
```
2404:6800:4005:81d::200e
142.251.10.138
103.102.166.224
google-site-verification=AMHkgs-4ViEvIJf5znZle-BSE2EPNFqM1nDJGRyn2qk
v=spf1 include:wikimedia.org ~all
2001:df2:e500:ed1a::1
```


## Integration
### cmake
```
# clone or download this project and add it to CMakeLists.txt
add_subdirectory(lib_dns)

target_link_libraries(${PROJECT_NAME} PRIVATE lib_dns)
```
