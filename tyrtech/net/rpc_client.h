#pragma once


#include <common/buffered_reader.h>
#include <io/channel_reader.h>
#include <net/service.json.h>


namespace tyrtech::net {


template<uint16_t buffer_size>
class rpc_client : private disallow_copy
{
private:
    using buffer_t =
            std::array<char, buffer_size>;

public:
    template<typename Function>
    class remote_call_wrapper : private disallow_copy
    {
    public:
        decltype(auto) request()
        {
            return typename Function::request_builder_t(m_request.add_message());
        }

        decltype(auto) response()
        {
            return typename Function::response_parser_t(m_response.get_parser(),
                                                        m_response.message());
        }

        void execute()
        {
            m_request.finalize();

            m_client->m_channel->send_all(m_buffer.data(), m_builder.size(), 0);
        }

        void wait()
        {
            uint16_t* size = reinterpret_cast<uint16_t*>(m_buffer.data());

            m_client->m_reader.read(size);

            if (unlikely(*size > buffer_size - sizeof(uint16_t)))
            {
                throw runtime_error("{}: response message too big",
                                    m_client->m_channel->uri());
            }

            m_client->m_reader.read(m_buffer.data() + sizeof(uint16_t), *size);

            m_parser = message::parser(m_buffer.data(), *size + sizeof(uint16_t));
            m_response = service::response_parser(&m_parser, 0);

            if (unlikely(m_response.has_error() == true))
            {
                Function::throw_exception(m_response.error());
            }
        }

    private:
        remote_call_wrapper(rpc_client* client)
          : m_client(client)
        {
            m_request.set_module(Function::module_id);
            m_request.set_function(Function::id);
        }

    private:
        rpc_client* m_client{nullptr};

        buffer_t m_buffer;

        message::builder m_builder{m_buffer.data(),
                                   static_cast<uint16_t>(m_buffer.size())};
        service::request_builder m_request{&m_builder};

        message::parser m_parser;
        service::response_parser m_response;

    private:
        friend class rpc_client;
    };

public:
    template<typename Function>
    decltype(auto) remote_call()
    {
        return remote_call_wrapper<Function>(this);
    }

public:
    rpc_client(const std::shared_ptr<io::channel> channel)
      : m_channel(std::move(channel))
    {
    }

private:
    using channel_t =
            std::shared_ptr<io::channel>;

    using reader_t =
            buffered_reader<buffer_t, io::channel_reader>;

private:
    channel_t m_channel;

    buffer_t m_recv_buffer;

    io::channel_reader m_channel_reader{m_channel.get()};
    reader_t m_reader{&m_recv_buffer, &m_channel_reader};
};

}
