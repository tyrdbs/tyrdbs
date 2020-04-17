#pragma once


#include <common/disallow_copy.h>
#include <common/disallow_move.h>
#include <common/buffered_reader.h>
#include <common/logger.h>
#include <io/channel_reader.h>
#include <net/service.json.h>
#include <net/server_exception.h>

#include <unordered_set>
#include <array>


namespace tyrtech::net {


template<uint16_t buffer_size, typename T>
class rpc_server : private disallow_copy, disallow_move
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
    rpc_server(std::shared_ptr<io::channel> channel, T* service)
      : m_channel(std::move(channel))
      , m_service(service)
    {
        gt::create_thread(&rpc_server::server_thread, this);
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

            gt::create_thread(&rpc_server::worker_thread, this, std::move(remote));
        }

        m_channel.reset();
    }

    void worker_thread(const channel_t& remote)
    {
        m_remotes.insert(remote);

        logger::debug("{}: connected", remote->uri());

        try
        {
            buffer_t recv_buffer;
            io::channel_reader channel_reader(remote.get());

            reader_t reader(&recv_buffer, &channel_reader);

            auto ctx = m_service->create_context(remote);

            while (true)
            {
                buffer_t message_buffer;
                uint16_t* message_size = reinterpret_cast<uint16_t*>(message_buffer.data());

                reader.read(message_size);

                if (unlikely(*message_size > buffer_size - sizeof(uint16_t)))
                {
                    logger::error("{}: message too big, disconnecting...", remote->uri());

                    break;
                }

                reader.read(message_buffer.data() + sizeof(uint16_t), *message_size);

                message::parser parser(message_buffer.data(),
                                       *message_size + sizeof(uint16_t));
                service::request_parser request(&parser, 0);

                buffer_t send_buffer;

                message::builder builder(send_buffer.data(), send_buffer.size());
                service::response_builder response(&builder);

                try
                {
                    m_service->process_message(request, &response, &ctx);
                }
                catch (server_error& e)
                {
                    auto error = response.add_error();

                    error.add_code(e.code());
                    error.add_message(e.what());
                }

                response.finalize();

                remote->send_all(send_buffer.data(), builder.size(), 0);
            }
        }
        catch (io::channel::disconnected_error&)
        {
            logger::debug("{}: disconnected", remote->uri());
        }
        catch (message::malformed_message_error&)
        {
            logger::error("{}: invalid message, disconnecting...", remote->uri());
        }

        m_remotes.erase(remote);
    }
};

}
