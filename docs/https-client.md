# Simple HTTPS Client in C++

[This script](https://github.com/smart-linux-shell/ishell/blob/19_https-client/https-client/https_client.cpp) is a simple HTTPS client implemented in C++ using `libcurl` and `nlohmann::json`. It allows you to perform various HTTP requests (GET, POST, PUT, DELETE, etc.) and handle JSON responses from REST APIs.

Here is a short documentation that covers:
- [Features](#features)
- [Prerequisites](#prerequisites)
- [Dependencies](#dependencies)
- [Important Notes on SSL Verification](#important-notes-on-ssl-verification)
- [Running the Script](#running-the-script)
- [Usage](#usage)

### Features

- Supports multiple HTTP request types: GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH.
- Handles query parameters and request bodies.
- Captures response headers and body.
- Handles both JSON and non-JSON responses.

### Prerequisites

- C++11 or later
- `libcurl` library
- `nlohmann::json` library

### Dependencies
To minimize dependencies and keep the implementation lightweight, the project uses:
- [**`libcurl`**](https://curl.se/libcurl/): A widely used and powerful library for URL transfers, which supports multiple protocols, including HTTP and HTTPS.
- [**`nlohmann::json`**](https://github.com/nlohmann/json): A single-header library for JSON handling, making it easy to parse and generate JSON objects in C++.

Neither of these libraries is part of the C++ standard library, so installation is required:
- **`libcurl`**: Install via `sudo apt-get install libcurl4-openssl-dev`.
- **`nlohmann::json`**: Can be downloaded as a header file from [here](https://github.com/nlohmann/json/releases).

### Important Notes on SSL Verification

- This code **disables SSL verification** for simplicity, which is not recommended for production environments. Ensure to enable SSL verification (`CURLOPT_SSL_VERIFYPEER` and `CURLOPT_SSL_VERIFYHOST`) when working with sensitive data.

### Running the Script

1. Ensure that libcurl and nlohmann::json are installed on your system.
2. Compile the code with the following command:

```bash
g++ -std=c++11 -lcurl -o https_client https_client.cpp
```
3. Run the compiled program:
```bash
./https_client
```

### Usage

**1. GET Request with Query Parameters**

```cpp
std::string url = "https://httpbin.org/get";
std::map<std::string, std::string> query_params = {{"test", "123"}, {"foo", "bar"}};
json get_response = make_http_request(HttpRequestType::GET, url, query_params);
std::cout << "GET Response: " << get_response.dump(4) << std::endl;
```

**2. POST Request with JSON Body**

```cpp
std::string url = "https://httpbin.org/post";
json post_data = {
    {"name", "John Doe"},
    {"age", 30},
    {"email", "johndoe@example.com"}
};
json post_response = make_http_request(HttpRequestType::POST, url, {}, post_data, {{"Content-Type", "application/json"}});
std::cout << "POST Response: " << post_response.dump(4) << std::endl;
```

**3. DELETE Request**

```cpp
std::string url = "https://httpbin.org/delete";
json delete_response = make_http_request(HttpRequestType::DELETE, url);
std::cout << "DELETE Response: " << delete_response.dump(4) << std::endl;
```

**4. HEAD Request**

```cpp
std::string url = "https://httpbin.org/get";
json head_response = make_http_request(HttpRequestType::HEAD, url);
std::cout << "HEAD Response: " << head_response.dump(4) << std::endl;
```

**5. OPTIONS Request**

```cpp
std::string url = "https://httpbin.org/get";
json options_response = make_http_request(HttpRequestType::OPTIONS, url);
std::cout << "OPTIONS Response: " << options_response.dump(4) << std::endl;
```

**6. PATCH Request with JSON Body**

```cpp
std::string url = "https://httpbin.org/patch";
json patch_data = {
{"operation", "update"},
{"value", "new_value"}
};
json patch_response = make_http_request(HttpRequestType::PATCH, url, {}, patch_data, {{"Content-Type", "application/json"}});
std::cout << "PATCH Response: " << patch_response.dump(4) << std::endl;
```

**7. GET Request with HTTP**

```cpp
std::string url = "http://httpbin.org/get";
std::map<std::string, std::string> query_params_http = {{"test", "123"}, {"foo", "bar"}};
json get_response_http = make_http_request(HttpRequestType::GET, url, query_params_http);
std::cout << "GET Response (HTTP): " << get_response_http.dump(4) << std::endl;
```
