#pragma once


namespace tyrtech {


class disallow_copy
{
public:
    disallow_copy() noexcept = default;

public:
    disallow_copy(const disallow_copy&) = delete;
    disallow_copy& operator=(const disallow_copy&) = delete;

public:
    disallow_copy(disallow_copy&&) noexcept = default;
    disallow_copy& operator=(disallow_copy&&) noexcept = default;
};

}
