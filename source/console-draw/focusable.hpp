#pragma once

class focusable
{
public:
    focusable(bool focused) : focused(focused) {}

    bool is_focused() const { return focused; }
    void set_focused(bool focused)
    {
        this->focused = focused;
        handle_focused();
    }

protected:
    virtual void handle_focused() = 0;

protected:
    bool focused;
};