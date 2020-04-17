#pragma once


namespace tyrtech {


class disallow_move
{
public:
    disallow_move() noexcept = default;

public:
    disallow_move(const disallow_move&) noexcept = default;
    disallow_move& operator=(const disallow_move&) noexcept = default;

public:
    disallow_move(disallow_move&&) noexcept = delete;
    disallow_move& operator=(disallow_move&&) noexcept = delete;
};

}
