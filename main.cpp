#define BOOST_BEAST_USE_STD_STRING_VIEW
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <gsl/gsl>
#include <cstdlib>
#include <string>
#include <array>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <regex>
#include <nlohmann/json.hpp>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using nlohmann::json;
boost::asio::io_context ioc;

struct http_error : public std::runtime_error {
    int status;
    template <typename Response>
    http_error(Response const& response)
        : std::runtime_error(std::string{response.reason()}), status(response.result_int())
    {
    }
};

struct url {
    std::string host, port, query;
    template<typename Iter>
    url(Iter begin, Iter end)
    {
        std::regex  uri_regex{"([a-z]+)://([^:/]+)(:\\d+)?(/.*)"};
        std::match_results<Iter> m;
        if (!std::regex_search(begin, end, m,
                               uri_regex)) {
            throw std::logic_error{"Couldn't parse URI"};
        }
        host             = m[2];
        port = m[3].length() == 0 ? (m[1].str() == "https" ? ":443" : ":80")
                                  : m[3].str();
        query = m[4];
        port.erase(port.begin(), port.begin() + 1);
    }
    template <typename Range> url(Range range) : url(range.begin(), range.end())
    {
    }
};

std::string get_with_redirect(std::string_view hostname, std::string_view port,
                              std::string_view path)
{
    std::string target{path}, host{hostname};
    // These objects perform our I/O
    tcp::resolver resolver{ioc};

    ssl::context ctx(ssl::context::tlsv12_client);
    // Verify the remote server's certificate
    // ctx.set_verify_mode(ssl::verify_peer);
    ssl::stream<tcp::socket> stream(ioc, ctx);

    if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
        boost::system::error_code ec{static_cast<int>(::ERR_get_error()),
                                     boost::asio::error::get_ssl_category()};
        throw boost::system::system_error{ec};
    }

    // Look up the domain name
    auto const results{resolver.resolve(host, port)};

    // Make the connection on the IP address we get from a lookup
    boost::asio::connect(stream.next_layer(), results.begin(), results.end());
    // Perform the SSL handshake
    stream.handshake(ssl::stream_base::client);

    // Set up an HTTP GET request message
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::host, host);
    req.set(http::field::accept, "application/citeproc+json");
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Send the HTTP request to the remote host
    http::write(stream, req);

    // This buffer is used for reading and must be persisted
    boost::beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::string_body> res;

    // Receive the HTTP response
    http::read(stream, buffer, res);
    try {
        if (res.result_int() >= 300 && res.result_int() < 400) {
            const auto& location = res.at(http::field::location);
            url uri{location};
            return get_with_redirect(uri.host, uri.port, uri.query);
        }
    } catch (std::exception const& ex) {
        fmt::print(stderr, "Received exception {} in processing response\n{}\n",
                   ex.what(), res);
        std::terminate();
    }

    // Write the message to standard out
    if (res.result_int() >= 200 && res.result_int() < 300)
        return res.body();

    // Gracefully close the socket
    boost::system::error_code ec;
    stream.shutdown(ec);
    if (ec == boost::asio::error::eof) {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec.assign(0, ec.category());
    }
    if (ec)
        throw boost::system::system_error{ec};
    throw http_error(res);
}

json get_citeproc(std::string_view doi)
{
    std::string result{get_with_redirect("doi.org", "443", doi)};
    return json::parse(result);
}

// Performs an HTTP GET and prints the response
void start(std::string_view /*appname*/, gsl::span<std::string> args)
{
    std::string target{args[0]};
    nlohmann::json result = get_citeproc(target);
    fmt::print("{}\n", result["title"]);
}
