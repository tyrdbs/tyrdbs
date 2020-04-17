#pragma once


#include <io/channel.h>
#include <net/server_exception.h>
#include <net/service.json.h>

#include <tests/ping_module.json.h>


namespace tests {


template<
    typename pingImpl
>
struct ping_service : private tyrtech::disallow_copy
{
    ping::module<pingImpl> ping;

    ping_service(
        pingImpl* ping
    )
      : ping(ping)
    {
    }

    struct context : private tyrtech::disallow_copy
    {
        typename pingImpl::context ping_ctx;

        context(
            typename pingImpl::context&& ping_ctx
        )
          : ping_ctx(std::move(ping_ctx))
        {
        }
    };

    context create_context(const std::shared_ptr<tyrtech::io::channel>& remote)
    {
        return context(
            ping.create_context(remote)
        );
    }

    void process_message(const tyrtech::net::service::request_parser& service_request,
                         tyrtech::net::service::response_builder* service_response,
                         context* ctx)
    {
        switch (service_request.module())
        {
            case ping::id:
            {
                ping.process_message(service_request, service_response, &ctx->ping_ctx);

                break;
            }
            default:
            {
                throw tyrtech::net::unknown_module_error("#{}: unknown module", service_request.function());
            }
        }
    }
};

}
