/*
 * Adds a guardian toggle icon to the SteamVR dashboard bar.
 * Tapping the icon hides or shows the guardian walls, flashes a notice,
 * and returns the dashboard to the Steam window.
 */
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "openvr_capi.h"

S_API intptr_t VR_InitInternal(EVRInitError *peError, EVRApplicationType eType);
S_API void VR_ShutdownInternal(void);
S_API void *VR_GetGenericInterface(const char *pchInterfaceVersion, EVRInitError *peError);

#define SECTION "collisionBounds"
#define KEY "CollisionBoundsFadeDistance"
#define DEFAULT_FADE 0.7f
#define STEAM_DASHBOARD "valve.steam.gamepadui.main"
#define NOTICE_SECONDS 1.5

static struct VR_IVRSystem_FnTable *vr_system;
static struct VR_IVROverlay_FnTable *vr_overlay;
static struct VR_IVRSettings_FnTable *vr_settings;

static char asset_dir[512];
static char state_file[512];
static char remove_entry[512];
static volatile sig_atomic_t stop_requested;

static void request_stop(int sig)
{
    (void)sig;
    stop_requested = 1;
}

/* App list entry that removes the icon; it exists only while this runs. */
static void write_remove_entry(void)
{
    const char *data_home = getenv("XDG_DATA_HOME");
    const char *home = getenv("HOME");
    FILE *f;

    if (data_home && *data_home)
        snprintf(remove_entry, sizeof remove_entry,
                 "%s/applications/guardian-toggle-remove.desktop", data_home);
    else
        snprintf(remove_entry, sizeof remove_entry,
                 "%s/.local/share/applications/guardian-toggle-remove.desktop", home ? home : "");

    f = fopen(remove_entry, "w");
    if (!f) {
        remove_entry[0] = '\0';
        return;
    }
    fprintf(f,
            "[Desktop Entry]\n"
            "Type=Application\n"
            "Name=Remove Guardian Icon From Bar\n"
            "Comment=Hide the guardian toggle icon until SteamVR restarts\n"
            "Exec=systemctl --user stop guardian-toggle.service\n"
            "Icon=%s/icon.png\n"
            "Terminal=false\n"
            "Categories=Utility;\n"
            "StartupNotify=false\n",
            asset_dir);
    fclose(f);
}

static void remove_remove_entry(void)
{
    if (remove_entry[0])
        unlink(remove_entry);
}

static void *get_interface(const char *version)
{
    char name[128];
    EVRInitError err = EVRInitError_VRInitError_None;
    snprintf(name, sizeof name, "FnTable:%s", version);
    return VR_GetGenericInterface(name, &err);
}

static double now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static void asset_path(char *out, size_t size, const char *name)
{
    snprintf(out, size, "%s/%s", asset_dir, name);
}

/* State file shared with toggle.sh: holds the fade distance to restore. */
static void init_state_path(void)
{
    const char *state_home = getenv("XDG_STATE_HOME");
    const char *home = getenv("HOME");
    char dir[480];

    if (state_home && *state_home)
        snprintf(dir, sizeof dir, "%s/guardian-toggle", state_home);
    else
        snprintf(dir, sizeof dir, "%s/.local/state/guardian-toggle", home ? home : "");
    snprintf(state_file, sizeof state_file, "%s/fade-distance", dir);
}

static void save_fade(float value)
{
    char dir[512];
    FILE *f;

    /* Create every missing directory above the state file. */
    snprintf(dir, sizeof dir, "%s", state_file);
    for (char *p = dir + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(dir, 0755);
            *p = '/';
        }
    }
    f = fopen(state_file, "w");
    if (!f)
        return;
    fprintf(f, "%f\n", value);
    fclose(f);
}

static float load_fade(void)
{
    float value = 0;
    FILE *f = fopen(state_file, "r");

    if (f) {
        if (fscanf(f, "%f", &value) != 1)
            value = 0;
        fclose(f);
    }
    return value > 0 ? value : DEFAULT_FADE;
}

/* Flips the walls and returns true if they are now hidden. */
static bool toggle_walls(void)
{
    EVRSettingsError err = EVRSettingsError_VRSettingsError_None;
    float current = vr_settings->GetFloat(SECTION, KEY, &err);

    if (current > 0) {
        save_fade(current);
        vr_settings->SetFloat(SECTION, KEY, 0, &err);
        return true;
    }
    vr_settings->SetFloat(SECTION, KEY, load_fade(), &err);
    return false;
}

static bool walls_hidden(void)
{
    EVRSettingsError err = EVRSettingsError_VRSettingsError_None;
    return vr_settings->GetFloat(SECTION, KEY, &err) <= 0;
}

static void set_image(VROverlayHandle_t handle, const char *name)
{
    char path[600];
    asset_path(path, sizeof path, name);
    vr_overlay->SetOverlayFromFile(handle, path);
}

static const char *notice_image(bool hidden)
{
    return hidden ? "notice-hidden.png" : "notice-shown.png";
}

int main(int argc, char **argv)
{
    EVRInitError init_err = EVRInitError_VRInitError_None;
    VROverlayHandle_t panel = 0, thumb = 0, notice = 0;
    struct VREvent_t ev;
    double notice_until = 0;
    bool hidden;

    if (argc > 1) {
        snprintf(asset_dir, sizeof asset_dir, "%s", argv[1]);
    } else {
        /* Assets live next to the executable by default. */
        ssize_t n = readlink("/proc/self/exe", asset_dir, sizeof asset_dir - 1);
        if (n <= 0) {
            fprintf(stderr, "cannot locate assets\n");
            return 1;
        }
        asset_dir[n] = '\0';
        char *slash = strrchr(asset_dir, '/');
        if (slash)
            *slash = '\0';
    }
    init_state_path();

    VR_InitInternal(&init_err, EVRApplicationType_VRApplication_Overlay);
    if (init_err != EVRInitError_VRInitError_None) {
        fprintf(stderr, "VR init failed: %d\n", init_err);
        return 1;
    }
    vr_system = get_interface(IVRSystem_Version);
    vr_overlay = get_interface(IVROverlay_Version);
    vr_settings = get_interface(IVRSettings_Version);
    if (!vr_system || !vr_overlay || !vr_settings) {
        fprintf(stderr, "missing OpenVR interface\n");
        VR_ShutdownInternal();
        return 1;
    }

    /* Bar icon plus the panel the dashboard switches to when it is tapped. */
    if (vr_overlay->CreateDashboardOverlay("guardian-toggle", "Guardian Walls", &panel, &thumb)
        != EVROverlayError_VROverlayError_None) {
        fprintf(stderr, "cannot create dashboard overlay\n");
        VR_ShutdownInternal();
        return 1;
    }
    /* Walls always start shown; hiding them lasts one SteamVR session. */
    if (walls_hidden())
        toggle_walls();
    hidden = false;
    set_image(thumb, "icon.png");
    set_image(panel, notice_image(hidden));
    vr_overlay->SetOverlayWidthInMeters(panel, 1.0f);

    /* Floating notice shown in front of the headset after each toggle. */
    vr_overlay->CreateOverlay("guardian-toggle.notice", "Guardian Walls Notice", &notice);
    vr_overlay->SetOverlayWidthInMeters(notice, 0.4f);
    vr_overlay->SetOverlayFlag(notice, VROverlayFlags_VisibleInDashboard, true);
    vr_overlay->SetOverlaySortOrder(notice, 100);
    struct HmdMatrix34_t in_front = { { { 1, 0, 0, 0 }, { 0, 1, 0, -0.1f }, { 0, 0, 1, -1.0f } } };
    vr_overlay->SetOverlayTransformTrackedDeviceRelative(notice, k_unTrackedDeviceIndex_Hmd, &in_front);

    signal(SIGTERM, request_stop);
    signal(SIGINT, request_stop);
    write_remove_entry();

    for (;;) {
        if (stop_requested) {
            /* Removed from the bar: don't leave the walls hidden with no icon to restore them. */
            if (walls_hidden())
                toggle_walls();
            remove_remove_entry();
            VR_ShutdownInternal();
            return 0;
        }

        while (vr_system->PollNextEvent(&ev, sizeof ev)) {
            if (ev.eventType == EVREventType_VREvent_Quit) {
                vr_system->AcknowledgeQuit_Exiting();
                remove_remove_entry();
                VR_ShutdownInternal();
                return 0;
            }
        }

        while (vr_overlay->PollNextOverlayEvent(panel, &ev, sizeof ev)) {
            if (ev.eventType != EVREventType_VREvent_OverlayShown)
                continue;
            hidden = toggle_walls();
            set_image(panel, notice_image(hidden));
            set_image(notice, notice_image(hidden));
            vr_overlay->ShowOverlay(notice);
            notice_until = now() + NOTICE_SECONDS;
            vr_overlay->ShowDashboard(STEAM_DASHBOARD);
            /* Otherwise reopening the dashboard returns to this panel and toggles again. */
            EVRSettingsError err = EVRSettingsError_VRSettingsError_None;
            vr_settings->SetString("dashboard", "lastAccessedExternalOverlayKey", "", &err);
        }

        if (notice_until && now() >= notice_until) {
            vr_overlay->HideOverlay(notice);
            notice_until = 0;
        }

        /* Taps only happen with the dashboard open, so poll slowly otherwise. */
        if (notice_until || vr_overlay->IsDashboardVisible())
            usleep(100 * 1000);
        else
            usleep(1000 * 1000);
    }
}
