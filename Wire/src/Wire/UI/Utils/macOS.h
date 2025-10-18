#pragma once

#include <stdint.h>

namespace wire::macOS {

    typedef void (*MenuCallback)(void);

    struct MenuItem
    {
        const char* Name;
        MenuCallback Callback;
    };

    struct Menu
    {
        const char* Name;
        uint64_t MenuItemCount;
        const MenuItem* MenuItems;
    };

    void CreateMenuBar(uint64_t menuCount, const Menu* menus);

}
