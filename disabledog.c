#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <errno.h>
#include <mach/mach_error.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

static int run_command(const char *path, char *const argv[])
{
    pid_t pid = 0;
    int error = posix_spawn(&pid, path, NULL, NULL, argv, environ);
    if (error != 0) {
        fprintf(stderr, "disabledog: failed to start %s: %s\n",
                path, strerror(error));
        return error;
    }

    int status = 0;
    do {
        if (waitpid(pid, &status, 0) == -1) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "disabledog: failed waiting for %s: %s\n",
                    path, strerror(errno));
            return errno;
        }
        break;
    } while (1);

    if (!WIFEXITED(status)) {
        fprintf(stderr, "disabledog: %s terminated abnormally\n", path);
        return 1;
    }

    if (WEXITSTATUS(status) != 0) {
        fprintf(stderr, "disabledog: %s exited with status %d\n",
                path, WEXITSTATUS(status));
        return WEXITSTATUS(status);
    }

    return 0;
}

static int disable_kernel_watchdog(void)
{
    CFMutableDictionaryRef matching = IOServiceMatching("IOWatchdog");
    if (matching == NULL) {
        fprintf(stderr, "disabledog: failed to create IOWatchdog matching dictionary\n");
        return 1;
    }

    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault,
                                                       matching);
    if (service == IO_OBJECT_NULL) {
        fprintf(stderr, "disabledog: IOWatchdog service was not found\n");
        return 1;
    }

    io_connect_t connection = IO_OBJECT_NULL;
    kern_return_t kr = IOServiceOpen(service, mach_task_self(), 1, &connection);
    IOObjectRelease(service);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "disabledog: IOServiceOpen failed: %s (0x%x)\n",
                mach_error_string(kr), kr);
        return 1;
    }

    kr = IOConnectCallScalarMethod(connection, 3, NULL, 0, NULL, NULL);
    IOServiceClose(connection);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr,
                "disabledog: failed to disable kernel watchdog: %s (0x%x)\n",
                mach_error_string(kr), kr);
        return 1;
    }

    return 0;
}

int main(void)
{
    if (geteuid() != 0) {
        fprintf(stderr, "disabledog: must be run as root\n");
        return 1;
    }

    fprintf(stderr, "disabledog: unloading watchdogd...\n");
    char *launchctl_argv[] = {
        "launchctl",
        "unload",
        "/System/Library/LaunchDaemons/com.apple.watchdogd.plist",
        NULL,
    };
    if (run_command("/bin/launchctl", launchctl_argv) != 0) {
        return 1;
    }

    fprintf(stderr, "disabledog: disabling kernel watchdog...\n");
    if (disable_kernel_watchdog() != 0) {
        fprintf(stderr, "disabledog: restoring watchdogd after failure...\n");
        char *restore_argv[] = {
            "launchctl",
            "load",
            "/System/Library/LaunchDaemons/com.apple.watchdogd.plist",
            NULL,
        };
        if (run_command("/bin/launchctl", restore_argv) != 0) {
            fprintf(stderr,
                    "disabledog: CRITICAL: failed to restore watchdogd; reboot the device\n");
        }
        return 1;
    }

    fprintf(stderr, "disabledog: restarting SpringBoard...\n");
    char *killall_argv[] = {"killall", "SpringBoard", NULL};
    if (run_command("/usr/bin/killall", killall_argv) != 0) {
        return 1;
    }

    fprintf(stderr, "disabledog: completed successfully\n");
    return 0;
}
