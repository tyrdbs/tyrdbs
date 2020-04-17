#pragma once


#include <common/fmt.h>
#include <common/conv.h>

#include <regex>


namespace tyrtech::http {


DEFINE_EXCEPTION(runtime_error, error);
DEFINE_EXCEPTION(error, malformed_message_error);
DEFINE_EXCEPTION(error, method_not_allowed_error);
DEFINE_EXCEPTION(error, bad_request_error);
DEFINE_EXCEPTION(error, not_found_error);
DEFINE_EXCEPTION(error, internal_server_error);


#define BAD_REQUEST tyrtech::http::bad_request_error("400 Bad Request")
#define NOT_FOUND tyrtech::http::bad_request_error("404 Not Found")
#define METHOD_NOT_ALLOWED tyrtech::http::method_not_allowed_error("405 Method Not Allowed")
#define INTERNAL_SERVER_ERROR tyrtech::http::internal_server_error("500 Internal Server Error")


std::regex __request_line("^([A-Z]+)[ ]+([^ ]+)[ ]+HTTP/(\\d+\\.\\d+)\r\n$",
                          std::regex::optimize | std::regex::ECMAScript);

std::regex __response_line("^HTTP/(\\d+\\.\\d+)[ ]+(\\d+)[ ]+([^\r]+)\r\n$",
                           std::regex::optimize | std::regex::ECMAScript);

std::regex __header_line("^([^:]+):[ ]*([^\r]*)\r\n$",
                         std::regex::optimize | std::regex::ECMAScript);


template<typename BufferType, typename ReaderType>
class reader : private disallow_copy
{
public:
    std::string_view load_line()
    {
        uint32_t start = m_offset;

        while (true)
        {
            if (m_offset >= m_buffer->size())
            {
                break;
            }

            *(m_buffer->data() + m_offset) = m_reader->read();

            if (*(m_buffer->data() + m_offset) == '\n')
            {
                if (m_offset != start)
                {
                    if (*(m_buffer->data() + m_offset - 1) == '\r')
                    {
                        m_offset++;
                        break;
                    }
                }
            }

            m_offset++;
        }

        if (start == m_offset)
        {
            return std::string_view(nullptr, 0);
        }

        return std::string_view(m_buffer->data() + start, m_offset - start);
    }

public:
    reader(BufferType* buffer, ReaderType* reader)
      : m_buffer(buffer)
      , m_reader(reader)
    {
    }

private:
    BufferType* m_buffer{nullptr};
    ReaderType* m_reader{nullptr};

    uint32_t m_offset{0};
};

class headers : private disallow_copy
{
public:
    void set(const std::string_view& name, const std::string_view& value)
    {
        m_headers.push_back(header_t(name, value));
    }

    template<typename T>
    T get(const std::string_view& name) const
    {
        return conv::parse<T>(get(name));
    }

    decltype(auto) raw() const
    {
        return m_headers;
    }

private:
    using header_t =
            std::pair<std::string_view, std::string_view>;

    using headers_t =
            std::vector<header_t>;

private:
    headers_t m_headers;

private:
    std::string_view get(const std::string_view& name) const
    {
        auto it = std::find_if(m_headers.cbegin(),
                               m_headers.cend(),
                               [&name] (const header_t& h)
                               {
                                   return h.first.size() == name.size() &&
                                          strncasecmp(h.first.data(),
                                                      name.data(),
                                                      name.size()) == 0;
                               });

        if (it == m_headers.end())
        {
            return std::string_view(nullptr, 0);
        }

        return it->second;
    }
};

}
