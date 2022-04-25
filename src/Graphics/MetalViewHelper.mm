/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Cocoa/Cocoa.h>

// Workaround for deadlock on metal https://github.com/bkaradzic/bgfx/issues/1773
void *cbSetupMetalLayer(void *wnd) {
    NSWindow *window = (NSWindow*)wnd;
    NSView *contentView = [window contentView];
    [contentView setWantsLayer:YES];
    CAMetalLayer *res = [CAMetalLayer layer];
    [contentView setLayer:res];
    return res;
}
