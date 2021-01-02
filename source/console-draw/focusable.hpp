#pragma once

class focusable
{
public:
    focusable(bool focused) : focused(focused) {}

    bool is_focused() const { return focused; }
    void set_focused(bool focused)
    {
        bool was_focused = this->focused;
        this->focused = focused;
        handle_focused(was_focused);
    }

protected:
    virtual void handle_focused(bool was_focused) = 0;

protected:
    bool focused;
};