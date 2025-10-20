#include "macOS.h"

#import <Cocoa/Cocoa.h>

@interface MenuHandler : NSObject
@property (nonatomic, assign) wire::macOS::MenuCallback callback;
- (instancetype)initWithCallback:(wire::macOS::MenuCallback)cb;
- (void)menuItemClicked:(id)sender;
@end

@implementation MenuHandler

- (instancetype)initWithCallback:(wire::macOS::MenuCallback)cb {
    self = [super init];
    if (self) {
        _callback = cb;
    }
    return self;
}

- (void)menuItemClicked:(id)sender {
    if (_callback) {
        _callback();
    }
}

@end

void wire::macOS::CreateMenuBar(uint64_t menuCount, const Menu* menus)
{
    NSMenu* mainMenu = [[NSMenu alloc] init];
    
    NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
    [mainMenu addItem:appMenuItem];
    
    NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@"App"];
    [appMenu addItemWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenuItem setSubmenu:appMenu];
    
    for (uint64_t i = 0; i < menuCount; i++)
    {
        const Menu& menu = menus[i];
        NSMenuItem* menuItem = [[NSMenuItem alloc] init];
        [mainMenu addItem:menuItem];
        
        NSMenu* nsmenu = [[NSMenu alloc] initWithTitle:[NSString stringWithCString:menu.Name encoding:NSUTF8StringEncoding]];
        
        for (uint64_t j = 0; j < menu.MenuItemCount; j++)
        {
            const MenuItem& item = menu.MenuItems[j];
            MenuCallback callback = item.Callback;
            MenuHandler* handler = [[MenuHandler alloc] initWithCallback:callback];
            
            NSMenuItem* nsitem = [[NSMenuItem alloc] initWithTitle:[NSString stringWithCString:item.Name encoding:NSUTF8StringEncoding] action:@selector(menuItemClicked:) keyEquivalent:@""];
            [nsitem setTarget:handler];
            [nsmenu addItem:nsitem];
            [menuItem setSubmenu:nsmenu];
        }
    }
    
    [NSApp setMainMenu:mainMenu];
}
