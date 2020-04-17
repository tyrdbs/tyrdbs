#include <common/disallow_copy.h>
#include <common/disallow_move.h>
#include <gt/engine.h>
#include <extern/gtswitch.h>


namespace tyrtech::gt {


static constexpr uint32_t stack_size{0x10000U};


struct context : private disallow_copy, disallow_move
{
    enum class state
    {
        WAITING,
        RUNNING,
        SUSPENDED,
        TERMINATED
    };

    using registers_t =
            std::array<uint64_t, 8>;

    registers_t registers{{0}};

    struct engine* engine{nullptr};

    function_t thread_callback;
    function_t terminate_callback;

    state state{state::SUSPENDED};
    bool is_user_ctx{false};

    uint32_t stack{static_cast<uint32_t>(-1)};
    uint32_t ctx{static_cast<uint32_t>(-1)};

    void reset();
};

struct engine : private disallow_copy, disallow_move
{
    using stack_pool_t =
            slabs<std::array<char, stack_size>>;

    using context_pool_t =
            slabs<context>;

    context_queue_t::entry_pool_t queue_entry_pool;
    context_queue_t run_queue{&queue_entry_pool};

    stack_pool_t stack_pool;
    context_pool_t context_pool;

    uint64_t suspended_ctx{0};
    uint64_t user_ctx{0};
    uint64_t user_ctx_waiting{0};

    context idle_ctx;
    context_t current_ctx{&idle_ctx};

    bool terminated{false};

    void enqueue(context_t ctx);
    bool yield(bool enqueue_ctx);

    context_t create_context(bool is_user_ctx, function_t thread_callback);
    context_t current_context() const;

    context_queue_t new_context_queue();

    void switch_to_idle(bool enqueue_ctx);
    void switch_to_next(bool enqueue_ctx);

    uint32_t allocate_stack();
    void free_stack(uint32_t handle);
    char* get_stack(uint32_t handle);

    uint32_t allocate_context();
    void free_context(uint32_t handle);
    context_t get_context(uint32_t handle);
};


void __start_thread(context_t ctx)
{
    engine* engine = ctx->engine;

    ctx->thread_callback();

    ctx->state = context::state::TERMINATED;

    if (ctx->terminate_callback)
    {
        ctx->terminate_callback();
    }

    if (ctx->is_user_ctx == true)
    {
        assert(likely(engine->user_ctx != 0));
        engine->user_ctx--;
    }

    ctx->reset();

    engine->free_context(ctx->ctx);
    engine->free_stack(ctx->stack);

    engine->current_ctx = &engine->idle_ctx;
    gtjump(engine->idle_ctx.registers.data());
}

void context::reset()
{
    thread_callback = function_t();
    terminate_callback = function_t();
}

void engine::enqueue(context_t ctx)
{
    assert(likely(ctx->state == context::state::SUSPENDED));
    ctx->state = context::state::WAITING;

    assert(likely(suspended_ctx != 0));
    suspended_ctx--;

    if (ctx->is_user_ctx == true)
    {
        user_ctx_waiting++;
    }

    run_queue.push_back(ctx);
}

bool engine::yield(bool enqueue_ctx)
{
    if (run_queue.empty() == true)
    {
        if (current_ctx != &idle_ctx)
        {
            switch_to_idle(enqueue_ctx);

            return true;
        }

        if (suspended_ctx == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    switch_to_next(enqueue_ctx);

    return true;
}

context_t engine::create_context(bool is_user_ctx, function_t thread_callback)
{
    assert(likely((stack_size & 0x0fff) == 0));

    uint32_t _ctx = allocate_context();
    context_t ctx = get_context(_ctx);

    ctx->ctx = _ctx;
    ctx->stack = allocate_stack();

    ctx->engine = this;
    ctx->thread_callback = std::move(thread_callback);

    char* stack = get_stack(ctx->stack);

    *reinterpret_cast<uint64_t*>(&stack[stack_size - 16]) =
            reinterpret_cast<uint64_t>(&__start_thread);
    *reinterpret_cast<uint64_t*>(&stack[stack_size - 8]) = 0;

    ctx->registers[0] = reinterpret_cast<uint64_t>(stack) + stack_size - 16;
    ctx->registers[7] = reinterpret_cast<uint64_t>(ctx);

    ctx->state = context::state::SUSPENDED;
    ctx->is_user_ctx = is_user_ctx;

    if (is_user_ctx == true)
    {
        user_ctx++;
    }

    suspended_ctx++;

    enqueue(ctx);

    return ctx;
}

context_queue_t engine::new_context_queue()
{
    return context_queue_t(&queue_entry_pool);
}

void engine::switch_to_idle(bool enqueue_ctx)
{
    current_ctx->state = context::state::SUSPENDED;
    suspended_ctx++;

    if (enqueue_ctx == true)
    {
        enqueue(current_ctx);
    }

    context_t old_ctx = current_ctx;
    context_t new_ctx = &idle_ctx;

    current_ctx = new_ctx;

    gtswitch(old_ctx->registers.data(), new_ctx->registers.data());
}

void engine::switch_to_next(bool enqueue_ctx)
{
    if (current_ctx != &idle_ctx)
    {
        current_ctx->state = context::state::SUSPENDED;
        suspended_ctx++;

        if (enqueue_ctx == true)
        {
            enqueue(current_ctx);
        }
    }

    context_t old_ctx = current_ctx;
    context_t new_ctx = *run_queue.front_item();

    run_queue.pop_front();

    current_ctx = new_ctx;
    current_ctx->state = context::state::RUNNING;

    if (current_ctx->is_user_ctx == true)
    {
        assert(likely(user_ctx_waiting != 0));
        user_ctx_waiting--;
    }

    gtswitch(old_ctx->registers.data(), new_ctx->registers.data());
}

uint32_t engine::allocate_stack()
{
    if (unlikely(stack_pool.full() == true))
    {
        stack_pool.extend(std::make_unique<stack_pool_t::slab>(128));
    }

    return stack_pool.allocate();
}

void engine::free_stack(uint32_t handle)
{
    stack_pool.free(handle);
}

char* engine::get_stack(uint32_t handle)
{
    char* stack = stack_pool.get(handle).data();
    assert(likely((reinterpret_cast<uint64_t>(stack) & 0x0f) == 0));

    return stack;
}

uint32_t engine::allocate_context()
{
    if (unlikely(context_pool.full() == true))
    {
        context_pool.extend(std::make_unique<context_pool_t::slab>(128));
    }

    return context_pool.allocate();
}

void engine::free_context(uint32_t handle)
{
    context_pool.free(handle);
}

context_t engine::get_context(uint32_t handle)
{
    return &context_pool.get(handle);
}


thread_local std::unique_ptr<engine> __engine;


void initialize()
{
    __engine = std::make_unique<engine>();
}

bool terminated()
{
    return __engine->terminated;
}

void terminate()
{
    __engine->terminated = true;
}

bool yield(bool enqueue_ctx)
{
    return __engine->yield(enqueue_ctx);
}

void enqueue(context_t ctx)
{
    __engine->enqueue(ctx);
}

uint64_t user_contexts_waiting()
{
    return __engine->user_ctx_waiting;
}

uint64_t user_contexts()
{
    return __engine->user_ctx;
}

void run()
{
    while (true)
    {
        if (yield(false) == false)
        {
            break;
        }

        if (user_contexts() == 0)
        {
            terminate();
        }
    }
}

context_t current_context()
{
    return __engine->current_ctx;
}

context_queue_t new_context_queue()
{
    return __engine->new_context_queue();
}

context_t create_context(bool is_user_ctx, function_t thread_callback)
{
    return __engine->create_context(is_user_ctx, std::move(thread_callback));
}

void _set_terminate_callback(context_t ctx, function_t terminate_callback)
{
    ctx->terminate_callback = std::move(terminate_callback);
}

}
