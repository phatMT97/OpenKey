//
//  AppDelegate.m
//  ModernKey
//
//  Created by Tuyen on 1/18/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <ServiceManagement/ServiceManagement.h>
#import <objc/runtime.h>
#import "AppDelegate.h"
#import "ViewController.h"
#import "OpenKeyManager.h"
#import "MJAccessibilityUtils.h"

AppDelegate* appDelegate;
extern ViewController* viewController;
extern void OnTableCodeChange(void);
extern void OnInputMethodChanged(void);
extern void RequestNewSession(void);
extern void OnActiveAppChanged(void);

//see document in Engine.h
int vLanguage = 1;
int vInputType = 0;
int vFreeMark = 0;
int vCodeTable = 0;
int vCheckSpelling = 1;
int vUseModernOrthography = 1;
int vQuickTelex = 0;
#define DEFAULT_SWITCH_STATUS 0x7A000206 //default option + z
int vSwitchKeyStatus = DEFAULT_SWITCH_STATUS;
int vRestoreIfWrongSpelling = 0;
int vFixRecommendBrowser = 1;
int vUseMacro = 1;
int vUseMacroInEnglishMode = 1;
int vAutoCapsMacro = 0;
int vSendKeyStepByStep = 0;
int vUseSmartSwitchKey = 1;
int vUpperCaseFirstChar = 0;
int vTempOffSpelling = 0;
int vAllowConsonantZFWJ = 0;
int vQuickStartConsonant = 0;
int vQuickEndConsonant = 0;
int vRememberCode = 1; //new on version 2.0
int vOtherLanguage = 1; //new on version 2.0
int vTempOffOpenKey = 0; //new on version 2.0

int vShowIconOnDock = 0; //new on version 2.0

int vPerformLayoutCompat = 0;

//beta feature
int vFixChromiumBrowser = 0; //new on version 2.0

extern int convertToolHotKey;
extern bool convertToolDontAlertWhenCompleted;

@interface AppDelegate ()

@end


@implementation AppDelegate {
    NSWindowController *_mainWC;
    NSWindowController *_macroWC;
    NSWindowController *_convertWC;
    NSWindowController *_aboutWC;
    
    NSStatusItem *statusItem;
    NSMenu *theMenu;
    
    NSMenuItem* menuInputMethod;
    
    NSMenuItem* mnuTelex;
    NSMenuItem* mnuVNI;
    NSMenuItem* mnuSimpleTelex1;
    NSMenuItem* mnuSimpleTelex2;
    
    NSMenuItem* mnuUnicode;
    NSMenuItem* mnuTCVN;
    NSMenuItem* mnuVNIWindows;
    
    NSMenuItem* mnuUnicodeComposite;
    NSMenuItem* mnuVietnameseLocaleCP1258;
    
    NSMenuItem* mnuQuickConvert;
    
    // FIX 2: Retry counter to avoid TCC desync issues
    NSInteger _initRetryCount;
}

-(void)askPermission {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText: [NSString stringWithFormat:@"OpenKey cần bạn cấp quyền để có thể hoạt động!"]];
    [alert setInformativeText:@"Sau khi cấp quyền trong System Settings:\n"
                               "1. Đóng System Settings\n"
                               "2. Đợi 2-3 giây để hệ thống cập nhật\n"
                               "3. OpenKey sẽ tự động khởi động"];

    [alert addButtonWithTitle:@"Không"];
    [alert addButtonWithTitle:@"Cấp quyền"];

    [alert.window makeKeyAndOrderFront:nil];
    [alert.window setLevel:NSStatusWindowLevel];

    NSModalResponse res = [alert runModal];

    if (res == 1001) {
        // User clicked "Cấp quyền" - Mở System Settings
        NSLog(@"🔐 User requested to grant permission, opening System Settings...");
        MJAccessibilityOpenPanel();
        
        // CRITICAL FIX: Đợi user cấp quyền thay vì terminate ngay
        // TCC database có thể mất 2-5 giây để sync
        [self waitForPermissionAndInit];
    } else {
        // User clicked "Không" - Thoát luôn
        NSLog(@"❌ User declined permission request, terminating");
        [NSApp terminate:0];
    }
}

// FIX: Monitor permission status và auto-init khi được cấp
- (void)waitForPermissionAndInit {
    __block int checkCount = 0;
    const int maxChecks = 15; // Check tối đa 15 lần (15 giây)
    
    NSTimer *permissionCheckTimer = [NSTimer scheduledTimerWithTimeInterval:1.0
                                                                     target:self
                                                                   selector:@selector(checkPermissionStatus:)
                                                                   userInfo:@{@"checkCount": @(checkCount), @"maxChecks": @(maxChecks)}
                                                                    repeats:YES];
    
    // Store timer để có thể invalidate sau
    objc_setAssociatedObject(self, "permissionCheckTimer", permissionCheckTimer, OBJC_ASSOCIATION_RETAIN);
}

- (void)checkPermissionStatus:(NSTimer *)timer {
    NSDictionary *info = timer.userInfo;
    int checkCount = [info[@"checkCount"] intValue];
    int maxChecks = [info[@"maxChecks"] intValue];
    
    checkCount++;
    
    if (MJAccessibilityIsEnabled()) {
        // Permission đã được cấp!
        NSLog(@"✅ Permission granted after %d seconds, initializing...", checkCount);
        [timer invalidate];
        
        // Init app với retry mechanism
        vShowIconOnDock = (int)[[NSUserDefaults standardUserDefaults] integerForKey:@"vShowIconOnDock"];
        if (vShowIconOnDock)
            [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];
        
        if (vSwitchKeyStatus & 0x8000)
            NSBeep();
        
        [self createStatusBarMenu];
        [self setQuickConvertString]; // FIX: Ensure hotkey is shown in menu
        
        _initRetryCount = 0;
        dispatch_async(dispatch_get_main_queue(), ^{
            [self attemptInitEventTap];
        });
        
    } else if (checkCount >= maxChecks) {
        // Timeout - user chưa cấp quyền sau 15 giây
        NSLog(@"⏱️ Permission check timeout after %d seconds", checkCount);
        [timer invalidate];
        
        // CRITICAL FIX: KHÔNG terminate app, để user có thể dùng menu TCC Reset
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Chưa nhận được quyền"];
        [alert setInformativeText:@"OpenKey chưa nhận được quyền Accessibility.\n\n"
                                   "Nếu bạn đã cấp quyền, vui lòng:\n"
                                   "1. Khởi động lại OpenKey\n"
                                   "2. Hoặc thử menu '🔧 Sửa lỗi quyền (TCC Reset)'\n\n"
                                   "App sẽ KHÔNG thoát để bạn có thể dùng menu."];
        [alert addButtonWithTitle:@"OK"];
        [alert runModal];
        
        // KHÔNG terminate - để user có thể access menu!
        // [NSApp terminate:0];
    } else {
        // Vẫn đang đợi - update timer userInfo
        NSLog(@"⏳ Waiting for permission... (%d/%d)", checkCount, maxChecks);
        NSDictionary *newInfo = @{@"checkCount": @(checkCount), @"maxChecks": @(maxChecks)};
        
        // Re-schedule timer với count mới
        [timer invalidate];
        NSTimer *newTimer = [NSTimer scheduledTimerWithTimeInterval:1.0
                                                             target:self
                                                           selector:@selector(checkPermissionStatus:)
                                                           userInfo:newInfo
                                                            repeats:YES];
        objc_setAssociatedObject(self, "permissionCheckTimer", newTimer, OBJC_ASSOCIATION_RETAIN);
    }
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    appDelegate = self;
    
    [self registerSupportedNotification];
    
    //set quick tooltip
    [[NSUserDefaults standardUserDefaults] setObject: [NSNumber numberWithInt: 50]
                                              forKey: @"NSInitialToolTipDelay"];
    
    //check whether this app has been launched before that or not
    NSArray* runningApp = [[NSWorkspace sharedWorkspace] runningApplications];
    if ([runningApp containsObject:OPENKEY_BUNDLE]) { //if already running -> exit
        [NSApp terminate:nil];
        return;
    }
    
    // check if user granted Accessabilty permission
    if (!MJAccessibilityIsEnabled()) {
        [self askPermission];
        return;
    }
    
    vShowIconOnDock = (int)[[NSUserDefaults standardUserDefaults] integerForKey:@"vShowIconOnDock"];
    if (vShowIconOnDock)
        [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];
    
    if (vSwitchKeyStatus & 0x8000)
        NSBeep();

    [self createStatusBarMenu];
    
    // FIX 2: Init with retry mechanism to handle TCC desync
    _initRetryCount = 0;
    dispatch_async(dispatch_get_main_queue(), ^{
        [self attemptInitEventTap];
    });
    
    //load default config if is first launch
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"NonFirstTime"] == 0) {
        [self loadDefaultConfig];
    }
    [[NSUserDefaults standardUserDefaults] setInteger:1 forKey:@"NonFirstTime"];
    
    //check update if enable
    NSInteger dontCheckUpdate = [[NSUserDefaults standardUserDefaults] integerForKey:@"DontCheckUpdate"];
    if (!dontCheckUpdate)
        [OpenKeyManager checkNewVersion:nil callbackFunc:nil];
    
    //correct run on startup
    NSInteger val = [[NSUserDefaults standardUserDefaults] integerForKey:@"RunOnStartup"];
    [appDelegate setRunOnStartup:val];
}

// FIX 2: Retry mechanism to handle TCC desync/lag
- (void)attemptInitEventTap {
    if ([OpenKeyManager initEventTap]) {
        // Success!
        NSLog(@"✅ Event tap initialized successfully on attempt %ld", (long)(_initRetryCount + 1));
        _initRetryCount = 0;
        
        // Show UI if needed
        NSInteger showui = [[NSUserDefaults standardUserDefaults] integerForKey:@"ShowUIOnStartup"];
        if (showui == 1) {
            [self onControlPanelSelected];
        }
        
        [self setQuickConvertString];
    } else {
        // Failed - retry up to 3 times before showing error
        _initRetryCount++;
        
        if (_initRetryCount < 3) {
            NSLog(@"⚠️ Event tap init failed (attempt %ld/3), retrying in 1 second...", (long)_initRetryCount);
            
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1.0 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
                [self attemptInitEventTap];
            });
        } else {
            // After 3 retries, check if it's really a permission issue or TCC zombie
            NSLog(@"❌ Event tap init failed after 3 attempts");
            
            if (!MJAccessibilityIsEnabled()) {
                // Permission not granted - show permission dialog
                NSLog(@"❌ Permission not granted after retries");
                [self askPermission];
            } else {
                // Permission is OK but init still fails - likely TCC zombie state
                NSLog(@"❌ Permission granted but event tap init still fails - possible TCC zombie state");
                [self showTCCResetGuidance];
            }
        }
    }
}

// FIX 2: Show guidance for TCC zombie state
- (void)showTCCResetGuidance {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"OpenKey không thể khởi động dù đã được cấp quyền"];
    [alert setInformativeText:@"Đây có thể là lỗi của macOS (TCC Database desync).\n\n"
                               "Vui lòng thử một trong hai cách sau:\n\n"
                               "Cách 1 (Khuyên dùng):\n"
                               "1. Mở Terminal\n"
                               "2. Chạy lệnh: tccutil reset Accessibility org.tuyenmai.OpenKey\n"
                               "3. Khởi động lại OpenKey và cấp quyền lại\n\n"
                               "Cách 2:\n"
                               "Bấm 'Mở System Settings' và tắt/bật lại quyền Accessibility cho OpenKey."];
    
    [alert addButtonWithTitle:@"Mở System Settings"];
    [alert addButtonWithTitle:@"Đóng"];
    
    [alert.window makeKeyAndOrderFront:nil];
    [alert.window setLevel:NSStatusWindowLevel];
    
    NSModalResponse res = [alert runModal];
    
    if (res == 1000) {
        // Open System Settings
        [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility"]];
    }
    
    [NSApp terminate:0];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag {
    [self onControlPanelSelected];
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}

-(void) createStatusBarMenu {
    NSStatusBar *statusBar = [NSStatusBar systemStatusBar];
    statusItem = [statusBar statusItemWithLength:NSVariableStatusItemLength];
    statusItem.button.image = [NSImage imageNamed:@"Status"];
    statusItem.button.alternateImage = [NSImage imageNamed:@"StatusHighlighted"];
    
    theMenu = [[NSMenu alloc] initWithTitle:@""];
    [theMenu setAutoenablesItems:NO];
    
    menuInputMethod = [theMenu addItemWithTitle:@"Bật Tiếng Việt"
                                                     action:@selector(onInputMethodSelected)
                                              keyEquivalent:@""];
    [theMenu addItem:[NSMenuItem separatorItem]];
    NSMenuItem* menuInputType = [theMenu addItemWithTitle:@"Kiểu gõ" action:nil keyEquivalent:@""];
    
    [theMenu addItem:[NSMenuItem separatorItem]];
    
    mnuUnicode = [theMenu addItemWithTitle:@"Unicode dựng sẵn" action:@selector(onCodeSelected:) keyEquivalent:@""];
    mnuUnicode.tag = 0;
    mnuTCVN = [theMenu addItemWithTitle:@"TCVN3 (ABC)" action:@selector(onCodeSelected:) keyEquivalent:@""];
    mnuTCVN.tag = 1;
    mnuVNIWindows = [theMenu addItemWithTitle:@"VNI Windows" action:@selector(onCodeSelected:) keyEquivalent:@""];
    mnuVNIWindows.tag = 2;
    NSMenuItem* menuCode = [theMenu addItemWithTitle:@"Bảng mã khác" action:nil keyEquivalent:@""];
    
    [theMenu addItem:[NSMenuItem separatorItem]];
    
    [theMenu addItemWithTitle:@"Công cụ chuyển mã..." action:@selector(onConvertTool) keyEquivalent:@""];
    mnuQuickConvert = [theMenu addItemWithTitle:@"Chuyển mã nhanh" action:@selector(onQuickConvert) keyEquivalent:@""];
    
    [theMenu addItem:[NSMenuItem separatorItem]];
    
    [theMenu addItemWithTitle:@"Bảng điều khiển..." action:@selector(onControlPanelSelected) keyEquivalent:@""];
    [theMenu addItemWithTitle:@"Gõ tắt..." action:@selector(onMacroSelected) keyEquivalent:@""];
    [theMenu addItemWithTitle:@"Giới thiệu" action:@selector(onAboutSelected) keyEquivalent:@""];
    [theMenu addItem:[NSMenuItem separatorItem]];
    
    // FIX 5: TCC Reset helper
    [theMenu addItemWithTitle:@"🔧 Sửa lỗi quyền (TCC Reset)" action:@selector(onResetTCC) keyEquivalent:@""];
    [theMenu addItem:[NSMenuItem separatorItem]];
    
    [theMenu addItemWithTitle:@"Thoát" action:@selector(terminate:) keyEquivalent:@"q"];
    
    
    [self setInputTypeMenu:menuInputType];
    [self setCodeMenu:menuCode];
    
    //set menu
    [statusItem setMenu:theMenu];
    
    [self fillData];
}

-(void)setQuickConvertString {
    NSMutableString* hotKey = [NSMutableString stringWithString:@""];
    bool hasAdd = false;
    if (convertToolHotKey & 0x100) {
        [hotKey appendString:@"⌃"];
        hasAdd = true;
    }
    if (convertToolHotKey & 0x200) {
        if (hasAdd)
            [hotKey appendString:@" + "];
        [hotKey appendString:@"⌥"];
        hasAdd = true;
    }
    if (convertToolHotKey & 0x400) {
        if (hasAdd)
            [hotKey appendString:@" + "];
        [hotKey appendString:@"⌘"];
        hasAdd = true;
    }
    if (convertToolHotKey & 0x800) {
        if (hasAdd)
            [hotKey appendString:@" + "];
        [hotKey appendString:@"⇧"];
        hasAdd = true;
    }
    
    unsigned short k = ((convertToolHotKey>>24) & 0xFF);
    if (k != 0xFE) {
        if (hasAdd)
            [hotKey appendString:@" + "];
        if (k == kVK_Space)
            [hotKey appendFormat:@"%@", @"␣ "];
        else
            [hotKey appendFormat:@"%c", k];
    }
    [mnuQuickConvert setTitle: hasAdd ? [NSString stringWithFormat:@"Chuyển mã nhanh - [%@]", [hotKey uppercaseString]] : @"Chuyển mã nhanh"];
}

-(void)loadDefaultConfig {
    vLanguage = 1; [[NSUserDefaults standardUserDefaults] setInteger:vLanguage forKey:@"InputMethod"];
    vInputType = 0; [[NSUserDefaults standardUserDefaults] setInteger:vInputType forKey:@"InputType"];
    vFreeMark = 0; [[NSUserDefaults standardUserDefaults] setInteger:vFreeMark forKey:@"FreeMark"];
    vCheckSpelling = 1; [[NSUserDefaults standardUserDefaults] setInteger:vCheckSpelling forKey:@"Spelling"];
    vCodeTable = 0; [[NSUserDefaults standardUserDefaults] setInteger:vCodeTable forKey:@"CodeTable"];
    vSwitchKeyStatus = DEFAULT_SWITCH_STATUS; [[NSUserDefaults standardUserDefaults] setInteger:vCodeTable forKey:@"SwitchKeyStatus"];
    vQuickTelex = 0; [[NSUserDefaults standardUserDefaults] setInteger:vQuickTelex forKey:@"QuickTelex"];
    vUseModernOrthography = 0; [[NSUserDefaults standardUserDefaults] setInteger:vUseModernOrthography forKey:@"ModernOrthography"];
    vRestoreIfWrongSpelling = 0; [[NSUserDefaults standardUserDefaults] setInteger:vRestoreIfWrongSpelling forKey:@"RestoreIfInvalidWord"];
    vFixRecommendBrowser = 1; [[NSUserDefaults standardUserDefaults] setInteger:vFixRecommendBrowser forKey:@"FixRecommendBrowser"];
    vUseMacro = 1; [[NSUserDefaults standardUserDefaults] setInteger:vUseMacro forKey:@"UseMacro"];
    vUseMacroInEnglishMode = 0; [[NSUserDefaults standardUserDefaults] setInteger:vUseMacroInEnglishMode forKey:@"UseMacroInEnglishMode"];
    vSendKeyStepByStep = 0;[[NSUserDefaults standardUserDefaults] setInteger:vUseMacroInEnglishMode forKey:@"SendKeyStepByStep"];
    vUseSmartSwitchKey = 1;[[NSUserDefaults standardUserDefaults] setInteger:vUseSmartSwitchKey forKey:@"UseSmartSwitchKey"];
    vUpperCaseFirstChar = 0;[[NSUserDefaults standardUserDefaults] setInteger:vUpperCaseFirstChar forKey:@"UpperCaseFirstChar"];
    vTempOffSpelling = 0;[[NSUserDefaults standardUserDefaults] setInteger:vTempOffSpelling forKey:@"vTempOffSpelling"];
    vAllowConsonantZFWJ = 0;[[NSUserDefaults standardUserDefaults] setInteger:vAllowConsonantZFWJ forKey:@"vAllowConsonantZFWJ"];
    vQuickStartConsonant = 0;[[NSUserDefaults standardUserDefaults] setInteger:vQuickStartConsonant forKey:@"vQuickStartConsonant"];
    vQuickEndConsonant = 0;[[NSUserDefaults standardUserDefaults] setInteger:vQuickEndConsonant forKey:@"vQuickEndConsonant"];
    vRememberCode = 1;[[NSUserDefaults standardUserDefaults] setInteger:vRememberCode forKey:@"vRememberCode"];
    vOtherLanguage = 1;[[NSUserDefaults standardUserDefaults] setInteger:vOtherLanguage forKey:@"vOtherLanguage"];
    vTempOffOpenKey = 0;[[NSUserDefaults standardUserDefaults] setInteger:vTempOffOpenKey forKey:@"vTempOffOpenKey"];
    vShowIconOnDock = 0;[[NSUserDefaults standardUserDefaults] setInteger:vShowIconOnDock forKey:@"vShowIconOnDock"];
    vFixChromiumBrowser = 0;[[NSUserDefaults standardUserDefaults] setInteger:vFixChromiumBrowser forKey:@"vFixChromiumBrowser"];
    vPerformLayoutCompat = 0;[[NSUserDefaults standardUserDefaults] setInteger:vPerformLayoutCompat forKey:@"vPerformLayoutCompat"];

    [[NSUserDefaults standardUserDefaults] setInteger:1 forKey:@"GrayIcon"];
    [[NSUserDefaults standardUserDefaults] setInteger:1 forKey:@"RunOnStartup"];

    [self fillData];
    [viewController fillData];
}

-(void)setRunOnStartup:(BOOL)val {
    CFStringRef appId = (__bridge CFStringRef)@"com.tuyenmai.OpenKeyHelper";
    SMLoginItemSetEnabled(appId, val);
}

-(void)setGrayIcon:(BOOL)val {
    [self fillData];
}

-(void)showIconOnDock:(BOOL)val {
    [NSApp setActivationPolicy: val ? NSApplicationActivationPolicyRegular : NSApplicationActivationPolicyAccessory];
}

#pragma mark -StatusBar menu data

- (void)setInputTypeMenu:(NSMenuItem*) parent {
    //sub for Kieu Go
    NSMenu *sub = [[NSMenu alloc] initWithTitle:@""];
    [sub setAutoenablesItems:NO];
    mnuTelex = [sub addItemWithTitle:@"Telex" action:@selector(onInputTypeSelected:) keyEquivalent:@""];
    mnuTelex.tag = 0;
    mnuVNI = [sub addItemWithTitle:@"VNI" action:@selector(onInputTypeSelected:) keyEquivalent:@""];
    mnuVNI.tag = 1;
    mnuSimpleTelex1 = [sub addItemWithTitle:@"Simple Telex 1" action:@selector(onInputTypeSelected:) keyEquivalent:@""];
    mnuSimpleTelex1.tag = 2;
    mnuSimpleTelex2 = [sub addItemWithTitle:@"Simple Telex 2" action:@selector(onInputTypeSelected:) keyEquivalent:@""];
    mnuSimpleTelex2.tag = 3;
    [theMenu setSubmenu:sub forItem:parent];
}

- (void)setCodeMenu:(NSMenuItem*) parent {
    //sub for Code
    NSMenu *sub = [[NSMenu alloc] initWithTitle:@""];
    [sub setAutoenablesItems:NO];
    mnuUnicodeComposite = [sub addItemWithTitle:@"Unicode tổ hợp" action:@selector(onCodeSelected:) keyEquivalent:@""];
    mnuUnicodeComposite.tag = 3;
    mnuVietnameseLocaleCP1258 = [sub addItemWithTitle:@"Vietnamese Locale CP 1258" action:@selector(onCodeSelected:) keyEquivalent:@""];
    mnuVietnameseLocaleCP1258.tag = 4;
    
    [theMenu setSubmenu:sub forItem:parent];
}

- (void) fillData {
    //fill data
    NSInteger intInputMethod = [[NSUserDefaults standardUserDefaults] integerForKey:@"InputMethod"];
    NSInteger grayIcon = [[NSUserDefaults standardUserDefaults] integerForKey:@"GrayIcon"];
    if (intInputMethod == 1) {
        [menuInputMethod setState:NSControlStateValueOn];
        statusItem.button.image = [NSImage imageNamed:@"Status"];
        [statusItem.button.image setTemplate:(grayIcon ? YES : NO)];
        statusItem.button.alternateImage = [NSImage imageNamed:@"StatusHighlighted"];
    } else {
        [menuInputMethod setState:NSControlStateValueOff];
        statusItem.button.image = [NSImage imageNamed:@"StatusEng"];
        [statusItem.button.image setTemplate:(grayIcon ? YES : NO)];
        statusItem.button.alternateImage = [NSImage imageNamed:@"StatusHighlightedEng"];
    }
    vLanguage = (int)intInputMethod;
    
    NSInteger intInputType = [[NSUserDefaults standardUserDefaults] integerForKey:@"InputType"];
    [mnuTelex setState:NSControlStateValueOff];
    [mnuVNI setState:NSControlStateValueOff];
    [mnuSimpleTelex1 setState:NSControlStateValueOff];
    [mnuSimpleTelex2 setState:NSControlStateValueOff];
    if (intInputType == 0) {
        [mnuTelex setState:NSControlStateValueOn];
    } else if (intInputType == 1) {
        [mnuVNI setState:NSControlStateValueOn];
    } else if (intInputType == 2) {
        [mnuSimpleTelex1 setState:NSControlStateValueOn];
    } else if (intInputType == 3) {
        [mnuSimpleTelex2 setState:NSControlStateValueOn];
    }
    vInputType = (int)intInputType;
    
    NSInteger intSwitchKeyStatus = [[NSUserDefaults standardUserDefaults] integerForKey:@"SwitchKeyStatus"];
    vSwitchKeyStatus = (int)intSwitchKeyStatus;
    if (vSwitchKeyStatus == 0)
        vSwitchKeyStatus = DEFAULT_SWITCH_STATUS;
    
    NSInteger intCode = [[NSUserDefaults standardUserDefaults] integerForKey:@"CodeTable"];
    [mnuUnicode setState:NSControlStateValueOff];
    [mnuTCVN setState:NSControlStateValueOff];
    [mnuVNIWindows setState:NSControlStateValueOff];
    [mnuUnicodeComposite setState:NSControlStateValueOff];
    [mnuVietnameseLocaleCP1258 setState:NSControlStateValueOff];
    if (intCode == 0) {
        [mnuUnicode setState:NSControlStateValueOn];
    } else if (intCode == 1) {
        [mnuTCVN setState:NSControlStateValueOn];
    } else if (intCode == 2) {
        [mnuVNIWindows setState:NSControlStateValueOn];
    } else if (intCode == 3) {
        [mnuUnicodeComposite setState:NSControlStateValueOn];
    } else if (intCode == 4) {
        [mnuVietnameseLocaleCP1258 setState:NSControlStateValueOn];
    }
    vCodeTable = (int)intCode;
    
    //
    NSInteger intRunOnStartup = [[NSUserDefaults standardUserDefaults] integerForKey:@"RunOnStartup"];
    [self setRunOnStartup:intRunOnStartup ? YES : NO];

}

-(void)onImputMethodChanged:(BOOL)willNotify {
    NSInteger intInputMethod = [[NSUserDefaults standardUserDefaults] integerForKey:@"InputMethod"];
    if (intInputMethod == 0)
        intInputMethod = 1;
    else
        intInputMethod = 0;
    vLanguage = (int)intInputMethod;
    [[NSUserDefaults standardUserDefaults] setInteger:intInputMethod forKey:@"InputMethod"];

    [self fillData];
    [viewController fillData];
    
    if (willNotify)
        OnInputMethodChanged();
}

#pragma mark -StatusBar menu action
- (void)onInputMethodSelected {
    [self onImputMethodChanged:YES];
}

- (void)onInputTypeSelected:(id)sender {
    NSMenuItem *menuItem = (NSMenuItem*) sender;
    [self onInputTypeSelectedIndex:(int)menuItem.tag];
}

- (void)onInputTypeSelectedIndex:(int)index {
    [[NSUserDefaults standardUserDefaults] setInteger:index forKey:@"InputType"];
    vInputType = index;
    [self fillData];
    [viewController fillData];
}

- (void)onCodeTableChanged:(int)index {
    [[NSUserDefaults standardUserDefaults] setInteger:index forKey:@"CodeTable"];
    vCodeTable = index;
    [self fillData];
    [viewController fillData];
    OnTableCodeChange();
}

- (void)onCodeSelected:(id)sender {
    NSMenuItem *menuItem = (NSMenuItem*) sender;
    [self onCodeTableChanged:(int)menuItem.tag];
}

-(void)onConvertTool {
    if (_convertWC == nil) {
        _convertWC = [[NSStoryboard storyboardWithName:@"Main" bundle:nil] instantiateControllerWithIdentifier:@"ConvertWindow"];
    }
    //[OpenKeyManager showDockIcon:YES];
    if ([_convertWC.window isVisible])
        return;
    [_convertWC.window makeKeyAndOrderFront:nil];
    [_convertWC.window setLevel:NSFloatingWindowLevel];
}

-(void)onQuickConvert {
    if ([OpenKeyManager quickConvert]) {
        if (!convertToolDontAlertWhenCompleted) {
            [OpenKeyManager showMessage: nil message:@"Chuyển mã thành công!" subMsg:@"Kết quả đã được lưu trong clipboard."];
        }
    } else {
        [OpenKeyManager showMessage: nil message:@"Không có dữ liệu trong clipboard!" subMsg:@"Hãy sao chép một đoạn text để chuyển đổi!"];
    }
}

-(void) onControlPanelSelected {
    if (_mainWC == nil) {
        _mainWC = [[NSStoryboard storyboardWithName:@"Main" bundle:nil] instantiateControllerWithIdentifier:@"OpenKey"];
    }
    //[OpenKeyManager showDockIcon:YES];
    if ([_mainWC.window isVisible]) {
        return;
    }
    [_mainWC.window makeKeyAndOrderFront:nil];
    [_mainWC.window setLevel:NSFloatingWindowLevel];
}

-(void) onMacroSelected {
    if (_macroWC == nil) {
        _macroWC = [[NSStoryboard storyboardWithName:@"Main" bundle:nil] instantiateControllerWithIdentifier:@"MacroWindow"];
    }
    //[OpenKeyManager showDockIcon:YES];
    if ([_macroWC.window isVisible])
        return;
    
    [_macroWC.window makeKeyAndOrderFront:nil];
    [_macroWC.window setLevel:NSFloatingWindowLevel];
}

-(void) onAboutSelected {
    if (_aboutWC == nil) {
        _aboutWC = [[NSStoryboard storyboardWithName:@"Main" bundle:nil] instantiateControllerWithIdentifier:@"AboutWindow"];
    }
    //[OpenKeyManager showDockIcon:YES];
    if ([_aboutWC.window isVisible])
        return;

    [_aboutWC.window makeKeyAndOrderFront:nil];
    [_aboutWC.window setLevel:NSFloatingWindowLevel];
}

// FIX 5: TCC Reset helper for user self-service
-(void)onResetTCC {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Bạn có muốn reset quyền Accessibility không?"];
    [alert setInformativeText:@"Lệnh này sẽ reset quyền Accessibility của OpenKey trong TCC Database (macOS permission system).\n\n"
                               "Sau khi reset, bạn cần:\n"
                               "1. Khởi động lại OpenKey\n"
                               "2. Cấp quyền Accessibility lại\n\n"
                               "Lệnh sẽ chạy: tccutil reset Accessibility org.tuyenmai.OpenKey\n\n"
                               "Dùng cách này khi OpenKey không hoạt động dù đã cấp quyền (TCC zombie state)."];
    
    [alert addButtonWithTitle:@"Reset và Thoát"];
    [alert addButtonWithTitle:@"Hủy"];
    
    [alert.window makeKeyAndOrderFront:nil];
    [alert.window setLevel:NSStatusWindowLevel];
    
    NSModalResponse res = [alert runModal];
    
    if (res == 1000) {
        NSLog(@"🔧 User requested TCC reset, running tccutil...");
        
        // Run TCC reset command
        NSTask *task = [[NSTask alloc] init];
        [task setLaunchPath:@"/usr/bin/tccutil"];
        [task setArguments:@[@"reset", @"Accessibility", @"com.tuyenmai.openkey"]];
        
        [task launch];
        [task waitUntilExit];
        
        NSLog(@"✅ TCC reset completed, exiting app");
        
        // Exit app so user can restart and grant permission again
        [NSApp terminate:0];
    }
}

#pragma mark -Short key event
-(void)onSwitchLanguage {
    [self onInputMethodSelected];
    [viewController fillData];
}

#pragma mark Reset OpenKey after mac computer awake
// FIX 3: Properly restart event tap after wake
-(void)receiveWakeNote: (NSNotification*)note {
    NSLog(@"💤 System woke up from sleep, restarting event tap...");
    
    // CRITICAL: Must stop first to reset _isInited flag
    // Otherwise initEventTap will return immediately without actually restarting
    [OpenKeyManager stopEventTap];
    
    // Delay 0.5s to let system stabilize after wake
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.5 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        // Reset retry counter and attempt init with retry mechanism
        _initRetryCount = 0;
        [self attemptInitEventTap];
    });
}

-(void)receiveSleepNote: (NSNotification*)note {
    NSLog(@"💤 System going to sleep, stopping event tap...");
    [OpenKeyManager stopEventTap];
}

-(void)receiveActiveSpaceChanged: (NSNotification*)note {
    RequestNewSession();
}

-(void)activeAppChanged: (NSNotification*)note {
    if (vUseSmartSwitchKey && [OpenKeyManager isInited]) {
        OnActiveAppChanged();
    }
}

-(void)registerSupportedNotification {
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver: self
                                                           selector: @selector(receiveWakeNote:)
                                                               name: NSWorkspaceDidWakeNotification object: NULL];
    
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver: self
                                                           selector: @selector(receiveSleepNote:)
                                                               name: NSWorkspaceWillSleepNotification object: NULL];
    
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver: self
                                                           selector: @selector(receiveActiveSpaceChanged:)
                                                               name: NSWorkspaceActiveSpaceDidChangeNotification object: NULL];
    
    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver: self
                                                           selector: @selector(activeAppChanged:)
                                                               name: NSWorkspaceDidActivateApplicationNotification object: NULL];
}
@end
