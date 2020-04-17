#pragma once


#include <common/slab_list.h>

#include <functional>


namespace tyrtech::gt {


using function_t =
        std::function<void()>;


using context_t =
        struct context*;

using context_queue_t =
        slab_list<context_t, 1024>;


void initialize();
void terminate();

void run();
bool terminated();

bool yield(bool enqueue_ctx = true);
void enqueue(context_t ctx);

int32_t sleep(uint64_t msec);

context_t current_context();
context_t create_context(bool is_user_ctx, function_t thread_callback);

context_queue_t new_context_queue();

uint64_t user_contexts_waiting();
uint64_t user_contexts();

void _set_terminate_callback(context_t ctx, function_t terminate_callback);

template<typename... Arguments>
void set_terminate_callback(context_t ctx, Arguments&&... arguments)
{
    _set_terminate_callback(ctx, std::bind(std::forward<Arguments>(arguments)...));
}

template<typename... Arguments>
context_t create_thread(Arguments&&... arguments)
{
    return create_context(true, std::bind(std::forward<Arguments>(arguments)...));
}

template<typename... Arguments>
context_t create_system_thread(Arguments&&... arguments)
{
    return create_context(false, std::bind(std::forward<Arguments>(arguments)...));
}

}
