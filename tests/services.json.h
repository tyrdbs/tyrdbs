#pragma once


#include <io/channel.h>
#include <net/server_exception.h>
#include <net/service.json.h>

#include <tests/modules.json.h>


namespace tests {


template<
    typename module1Impl,
    typename module2Impl
>
struct service1 : private tyrtech::disallow_copy
{
    module1::module<module1Impl> module1;
    module2::module<module2Impl> module2;

    service1(
        module1Impl* module1,
        module2Impl* module2
    )
      : module1(module1)
      , module2(module2)
    {
    }

    struct context : private tyrtech::disallow_copy
    {
        typename module1Impl::context module1_ctx;
        typename module2Impl::context module2_ctx;

        context(
            typename module1Impl::context&& module1_ctx,
            typename module2Impl::context&& module2_ctx
        )
          : module1_ctx(std::move(module1_ctx))
          , module2_ctx(std::move(module2_ctx))
        {
        }
    };

    context create_context(const std::shared_ptr<tyrtech::io::channel>& remote)
    {
        return context(
            module1.create_context(remote),
            module2.create_context(remote)
        );
    }

    void process_message(const tyrtech::net::service::request_parser& service_request,
                         tyrtech::net::service::response_builder* service_response,
                         context* ctx)
    {
        switch (service_request.module())
        {
            case module1::id:
            {
                module1.process_message(service_request, service_response, &ctx->module1_ctx);

                break;
            }
            case module2::id:
            {
                module2.process_message(service_request, service_response, &ctx->module2_ctx);

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
