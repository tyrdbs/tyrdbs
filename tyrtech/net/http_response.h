#pragma once


#include <net/http_utils.h>

#include <string>
#include <unordered_map>

#include <cstring>


namespace tyrtech::http {


class response
{
public:
    template<typename BufferType, typename ReaderType>
    static response parse(BufferType* buffer, ReaderType* reader)
    {
        using reader_t =
                http::reader<BufferType, ReaderType>;

        reader_t r(buffer, reader);

        response response;
        std::cmatch m;

        auto line = r.load_line();

        if (line.data() == nullptr)
        {
            throw malformed_message_error("response line empty");
        }

        if (std::regex_match(line.begin(), line.end(), m, __response_line) == false)
        {
            throw malformed_message_error("unable to parse response line");
        }

        assert(likely(m.size() == 4));

        response.m_version = std::string_view(m[1].first, m[1].length());
        response.m_code = std::string_view(m[2].first, m[2].length());
        response.m_status = std::string_view(m[3].first, m[3].length());

        while (true)
        {
            line = r.load_line();

            if (line.data() == nullptr)
            {
                throw malformed_message_error("unexpected hader ending");
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
            auto value = std::string_view(m[2].first, m[3].length());

            response.m_headers.set(name, value);
        }

        return response;
    }

public:
    std::string_view version() const
    {
        return m_version;
    }

    std::string_view code() const
    {
        return m_code;
    }

    std::string_view status() const
    {
        return m_status;
    }

    const http::headers& headers() const
    {
        return m_headers;
    }

private:
    std::string_view m_version;
    std::string_view m_code;
    std::string_view m_status;

    http::headers m_headers;
};

}
