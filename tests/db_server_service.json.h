#pragma once


#include <io/channel.h>
#include <net/server_exception.h>
#include <net/service.json.h>

#include <tests/db_server_module.json.h>


namespace tests {


template<
    typename collectionsImpl
>
struct db_server_service : private tyrtech::disallow_copy
{
    collections::module<collectionsImpl> collections;

    db_server_service(
        collectionsImpl* collections
    )
      : collections(collections)
    {
    }

    struct context : private tyrtech::disallow_copy
    {
        typename collectionsImpl::context collections_ctx;

        context(
            typename collectionsImpl::context&& collections_ctx
        )
          : collections_ctx(std::move(collections_ctx))
        {
        }
    };

    context create_context(const std::shared_ptr<tyrtech::io::channel>& remote)
    {
        return context(
            collections.create_context(remote)
        );
    }

    void process_message(const tyrtech::net::service::request_parser& service_request,
                         tyrtech::net::service::response_builder* service_response,
                         context* ctx)
    {
        switch (service_request.module())
        {
            case collections::id:
            {
                collections.process_message(service_request, service_response, &ctx->collections_ctx);

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
