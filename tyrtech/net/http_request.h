#pragma once


#include <net/http_utils.h>

#include <string>
#include <unordered_map>

#include <cstring>


namespace tyrtech::http {


class request
{
public:
    template<typename BufferType, typename ReaderType>
    static request parse(BufferType* buffer, ReaderType* reader)
    {
        using reader_t =
                http::reader<BufferType, ReaderType>;

        reader_t r(buffer, reader);

        request request;
        std::cmatch m;

        auto line = r.load_line();

        if (line.data() == nullptr)
        {
            throw malformed_message_error("request line empty");
        }

        if (std::regex_match(line.begin(), line.end(), m, __request_line) == false)
        {
            throw malformed_message_error("unable to parse request line");
        }

        assert(likely(m.size() == 4));

        request.m_method = std::string_view(m[1].first, m[1].length());
        request.m_path = std::string_view(m[2].first, m[2].length());
        request.m_version = std::string_view(m[3].first, m[3].length());

        while (true)
        {
            line = r.load_line();

            if (line.data() == nullptr)
            {
                throw malformed_message_error("unexpected header ending");
            }

            if (line.compare("\r\n") == 0)
            {
                break;
            }

            if (std::regex_match(line.begin(), line.end(), m, __header_line) == false)
            {
                throw malformed_message_error("unable to parse header line");
            }

            assert(likely(m.size() == 3));

            auto name = std::string_view(m[1].first, m[1].length());
            auto value = std::string_view(m[2].first, m[2].length());

            request.m_headers.set(name, value);
        }

        return request;
    }

public:
    std::string_view method() const
    {
        return m_method;
    }

    std::string_view path() const
    {
        return m_path;
    }

    std::string_view version() const
    {
        return m_version;
    }

    const http::headers& headers() const
    {
        return m_headers;
    }

private:
    std::string_view m_method;
    std::string_view m_path;
    std::string_view m_version;

    http::headers m_headers;
};

}
