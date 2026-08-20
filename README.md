# WatchdogDisabler

This is a fork of Lars Fröder's (opa334) WatchdogDisabler. The original tweak
prevents SpringBoard's `FBSProcessWatchdog` from terminating applications that
take too long to launch. This is useful when tracing application startup with
tools such as Frida.

## disabledog

The package also installs `/usr/bin/disabledog`. Run it as root to:

1. Unload `/System/Library/LaunchDaemons/com.apple.watchdogd.plist`.
2. Open `IOWatchdogUserClient` and disable the kernel watchdog.
3. Restart SpringBoard.

If disabling the kernel watchdog fails, the tool attempts to reload `watchdogd`
before returning the error. The IOWatchdog operation is based on Zhuowei Zhang's
[`who_let_the_dogs_out`](https://github.com/zhuowei/iOS-run-macOS-executables-tools/tree/main/who_let_the_dogs_out).

> [!WARNING]
> Disabling the kernel watchdog removes an important recovery mechanism. If
> SpringBoard, BackBoard, or another userspace service deadlocks, the device may
> remain unresponsive and require SSH access or a forced reboot. A full reboot
> restores the normal watchdog configuration.

## Build

Build the installable Debian package with Theos:

```sh
make clean package FINALPACKAGE=1
```

The resulting package is written to the `packages` directory.

## Credits

- Lars Fröder (opa334): original WatchdogDisabler tweak.
- Zhuowei Zhang: public IOWatchdog disable implementation.
- Cuihaixu: `disabledog` command, entitlement support, rollback, and packaging.

This project retains the original MIT license and copyright notice.
