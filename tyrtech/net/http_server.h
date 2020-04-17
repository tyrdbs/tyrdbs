#pragma once


#include <common/disallow_copy.h>
#include <common/disallow_move.h>
#include <common/buffered_reader.h>
#include <common/buffered_writer.h>
#include <common/logger.h>
#include <io/channel_reader.h>
#include <io/channel_writer.h>
#include <net/http_request.h>
#include <net/server_exception.h>

#include <unordered_set>
#include <array>


namespace tyrtech::net {


template<uint16_t buffer_size, typename T>
class http_server : private disallow_copy, disallow_move
{
public:
    void terminate()
    {
        m_channel->disconnect();

        for (auto&& remote : m_remotes)
        {
            remote->disconnect();
        }
    }

    std::string_view uri() const
    {
        return m_channel->uri();
    }

public:
    http_server(const std::shared_ptr<io::channel>& channel, T* service)
      : m_channel(channel)
      , m_service(service)
    {
        gt::create_thread(&http_server::server_thread, this);
    }

private:
    using channel_t =
            std::shared_ptr<io::channel>;

    using remotes_t =
            std::unordered_set<channel_t>;

    using buffer_t =
            std::array<char, buffer_size>;

    using reader_t =
            buffered_reader<buffer_t, io::channel_reader>;

    using writer_t =
            buffered_writer<buffer_t, io::channel_writer>;

private:
    channel_t m_channel;
    remotes_t m_remotes;

    T* m_service{nullptr};

private:
    void server_thread()
    {
        logger::debug("{}: server started", m_channel->uri());

        while (true)
        {
            channel_t remote;

            try
            {
                remote = m_channel->accept();
            }
            catch (io::channel::disconnected_error&)
            {
                logger::debug("{}: server shutdown", m_channel->uri());

                break;
            }

            gt::create_thread(&http_server::worker_thread, this, std::move(remote));
        }

        m_channel.reset();
    }

    void worker_thread(const channel_t& remote)
    {
        m_remotes.insert(remote);

        logger::debug("{}: connected", remote->uri());

        buffer_t recv_buffer;
        buffer_t send_buffer;

        io::channel_reader channel_reader(remote.get());
        io::channel_writer channel_writer(remote.get());

        reader_t reader(&recv_buffer, &channel_reader);
        writer_t writer(&send_buffer, &channel_writer);

        try
        {
            auto ctx = m_service->create_context(remote);

            while (true)
            {
                buffer_t http_buffer;

                try
                {
                    auto request = http::request::parse(&http_buffer, &reader);
                    m_service->process_request(request, &ctx, &reader, &writer);
                }
                catch (http::malformed_message_error&)
                {
                    throw BAD_REQUEST;
                }

                writer.flush();
            }
        }
        catch (http::error& e)
        {
            logger::error("{}: {}, disconnecting...", remote->uri(), e.what());

            char buff[512];
            auto response = format_to(buff, sizeof(buff),
                                      "HTTP/1.1 {}\r\n"
                                      "Content-Length: 0\r\n"
                                      "Connection-Type: Close\r\n"
                                      "\r\n", e.what());

            writer.write(response.data(), response.size());
            writer.flush();
        }
        catch (io::channel::disconnected_error&)
        {
            logger::debug("{}: disconnected", remote->uri());
        }

        m_remotes.erase(remote);
    }
};

}
