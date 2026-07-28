// LumaWall GNOME Shell Extension — v5
//
// Every override and technique used for secure injection:
//
// GnomeShellOverride equivalent:
//   1. BackgroundManager._createBackgroundActor  → inject LiveWallpaper clone
//   2. Shell.Global.get_window_actors            → filter LumaWall out, save unfiltered
//   3. Workspace.Workspace._isOverviewWindow     → hide from Overview
//   4. WorkspaceThumbnail._isOverviewWindow      → hide from thumbnails
//   5. Meta.Display.get_tab_list                 → hide from Alt-Tab
//   6. Shell.WindowTracker.get_window_app        → exclude from app tracker
//   7. Shell.App.get_windows                     → filter renderer
//   8. Shell.App.get_n_windows                   → correct count after filter
//   9. Shell.AppSystem.get_running               → filter apps with 0 windows
//
// WindowManager equivalent:
//   - Listen for window-manager 'map' event
//   - Immediately keepAtBottom + keepPosition (0,0) — do NOT minimize!
//   - Minimizing kills the compositor actor; this override keeps it visible but hidden via API
//
// Log: /tmp/lumawall-extension.log

import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';
import * as Main from 'resource:///org/gnome/shell/ui/main.js';
import * as Background from 'resource:///org/gnome/shell/ui/background.js';
import * as Workspace from 'resource:///org/gnome/shell/ui/workspace.js';
import * as WorkspaceThumbnail from 'resource:///org/gnome/shell/ui/workspaceThumbnail.js';
import Gio from 'gi://Gio';
import GLib from 'gi://GLib';
import Meta from 'gi://Meta';
import Shell from 'gi://Shell';
import Clutter from 'gi://Clutter';
import Graphene from 'gi://Graphene';
import { RoundedCornersEffect } from './roundedCornersEffect.js';
import { AutoPause } from './autoPause.js';

// The title our C++ daemon sets on its window
const LUMAWALL_TITLE = 'LumaWall-RendererWindow-X11';

// Delay before forcing display/workarea refresh after reloading backgrounds
const BACKGROUND_RELOAD_REFRESH_DELAY_MS = 500;

// ─── Logger ───────────────────────────────────────────────────────────────────
const LOG_FILE = '/tmp/lumawall-extension.log';
let _logFile: Gio.FileStream | null = null;

function _openLog(): void {
    try {
        _logFile = Gio.File.new_for_path(LOG_FILE).replace(null, false, Gio.FileCreateFlags.NONE, null);
        _log('INFO', '=== LumaWall Extension v5 ===');
    } catch (e: any) { console.error('[LumaWall] log open failed:', e.message); }
}
function _closeLog(): void {
    _log('INFO', '=== Stopping ===');
    try { _logFile?.close(null); } catch (_) {}
    _logFile = null;
}
function _log(level: string, msg: string): void {
    const line = `[${new Date().toISOString()}] [${level}] ${msg}\n`;
    console.log(`[LumaWall] [${level}] ${msg}`);
    try { _logFile?.write(line, null); _logFile?.flush(null); } catch (_) {}
}

// ─── D-Bus proxy to pause/resume on lock screen ───────────────────────────────
const DaemonProxy = Gio.DBusProxy.makeProxyWrapper(`
<node>
  <interface name="org.lumawall.Daemon">
    <method name="SetDesktopState">
      <arg type="s" name="state" direction="in"/>
    </method>
  </interface>
</node>`);

// ─── Helper: is this actor the LumaWall renderer? ─────────────────────────────
function _isRenderer(actorOrWindow: any): boolean {
    if (!actorOrWindow) return false;
    // Works for both Meta.Window and Meta.WindowActor
    const title = actorOrWindow.get_title?.() ?? actorOrWindow.meta_window?.get_title?.() ?? actorOrWindow.title;
    return typeof title === 'string' && title.includes(LUMAWALL_TITLE);
}

// ─── Unfiltered window actor accessor ─────────────────────────────────────────
// The get_window_actors override hides our renderer from GNOME.
// LiveWallpaper uses this to still find it.
let _getAllWindowActors: () => Meta.WindowActor[] = () => global.get_window_actors();

// ─── LiveWallpaper ────────────────────────────────────────────────────────────
class LiveWallpaper {
    private _backgroundActor: any;
    private _getAllWindowActors: () => Meta.WindowActor[];
    private _monitorIndex: number;
    private _clone: Clutter.Clone | null;
    private _sourceDestroyId: number | null;
    private _pollTimeoutId: number;
    private _disposed: boolean;
    public _container: Clutter.Actor;
    
    private _roundedCornersEffect: RoundedCornersEffect;
    private monitorWidth: number;
    private monitorHeight: number;

    constructor(backgroundActor: any, getAllWindowActors: () => Meta.WindowActor[]) {
        this._backgroundActor = backgroundActor;
        this._getAllWindowActors = getAllWindowActors;
        this._monitorIndex = backgroundActor.monitor ?? 0;
        this._clone = null;
        this._sourceDestroyId = null;
        this._pollTimeoutId = 0;
        this._disposed = false;

        this._container = new St.Widget({
            layout_manager: new Clutter.BinLayout(),
            clip_to_allocation: true,
        });
        
        this._container.add_constraint(new Clutter.BindConstraint({
            source: backgroundActor,
            coordinate: Clutter.BindCoordinate.SIZE,
        }));
        
        backgroundActor.add_child(this._container);

        const monitor = Main.layoutManager.monitors[this._monitorIndex] || { width: 1920, height: 1080 };
        this.monitorWidth = monitor.width;
        this.monitorHeight = monitor.height;

        this._roundedCornersEffect = new RoundedCornersEffect();
        this._backgroundActor.add_effect(this._roundedCornersEffect);
        this._roundedCornersEffect.setClipRadius(0.0);
        this._roundedCornersEffect.setBorderStroke(0);
        this._roundedCornersEffect.setBorderColor([1.0, 0.0, 0.0, 1.0]);
        this._roundedCornersEffect.setBounds([0, 0, this.monitorWidth, this.monitorHeight]);

        this._container.connect('notify::allocation', () => this._applyBounds());

        this._container.connect('destroy', () => {
            this._disposed = true;
            this._clearPoll();
            this._destroyClone();
        });

        _log('INFO', `LiveWallpaper created for monitor ${this._monitorIndex}`);
        this._applyWallpaper();
    }

    private _applyBounds(): void {
        const monitor = Main.layoutManager.monitors[this._monitorIndex];
        if (!monitor) return;
        const workArea = Main.layoutManager.getWorkAreaForMonitor(this._monitorIndex);
        if (!workArea) return;
        const panelOffset = (workArea.y - monitor.y) / monitor.height * this._backgroundActor.height;
        this._roundedCornersEffect.setBounds(
            [0, panelOffset, this._container.width, this._container.height]
        );
    }

    private _clearPoll(): void {
        if (this._pollTimeoutId) {
            GLib.source_remove(this._pollTimeoutId);
            this._pollTimeoutId = 0;
        }
    }

    private _destroyClone(): void {
        if (this._clone) {
            if (this._sourceDestroyId && this._clone.source) {
                try { this._clone.source.disconnect(this._sourceDestroyId); } catch (_) {}
                this._sourceDestroyId = null;
            }
            this._clone.set_source(null);
            this._clone.destroy();
            this._clone = null;
        }
    }

    public destroy(): void {
        this._disposed = true;
        this._clearPoll();
        this._destroyClone();
        try { this._container?.destroy(); } catch (_) {}
    }

    private _getRenderer(): Meta.WindowActor | null {
        const actors = this._getAllWindowActors();
        _log('DEBUG', `_getRenderer: ${actors.length} unfiltered actors`);

        for (const actor of actors) {
            if (!_isRenderer(actor)) continue;
            const monitorIdx = actor.meta_window?.get_monitor?.() ?? -1;
            _log('DEBUG', `  Found LumaWall actor on monitor ${monitorIdx}`);
            if (monitorIdx === this._monitorIndex) return actor;
        }
        return null;
    }

    private _applyWallpaper(): void {
        if (this._disposed) return;

        const attempt = (): boolean => {
            if (this._disposed) return false;

            const renderer = this._getRenderer();
            if (renderer) {
                this._clone = new Clutter.Clone({
                    source: renderer as Clutter.Actor,
                    pivot_point: new Graphene.Point({ x: 0.5, y: 0.5 }),
                    x_expand: true,
                    y_expand: true,
                });
                this._clone.connect('destroy', () => { this._clone = null; });
                this._sourceDestroyId = renderer.connect('destroy', () => {
                    _log('WARN', `Renderer destroyed on monitor ${this._monitorIndex}, will retry`);
                    this._destroyClone();
                    if (!this._disposed) this._applyWallpaper();
                });
                this._container.add_child(this._clone);

                this._container.ease({
                    opacity: 255,
                    duration: 1000,
                    mode: Clutter.AnimationMode.EASE_OUT_QUAD,
                });

                _log('INFO', `✓ Clone attached for monitor ${this._monitorIndex}`);
                this._pollTimeoutId = 0;
                return false; // stop polling
            }
            return true; // continue polling
        };

        if (attempt()) {
            this._pollTimeoutId = GLib.timeout_add(GLib.PRIORITY_DEFAULT, 1000, attempt);
        }
    }
}

// ─── ManagedWindow ────────────────────────────────────────────────────────────
class ManagedWindow {
    private _window: Meta.Window;
    private _signals: number[];
    private _resyncId: number;
    private _disposed: boolean;

    constructor(metaWindow: Meta.Window) {
        this._window = metaWindow;
        this._signals = [];
        this._resyncId = 0;
        this._disposed = false;

        this._window.move_frame(true, 0, 0);
        this._window.lower();

        this._signals.push(
            metaWindow.connect_after('raised', () => {
                if (!this._disposed) this._window.lower();
            })
        );

        this._signals.push(
            metaWindow.connect('position-changed', () => {
                if (!this._disposed) this._window.move_frame(true, 0, 0);
            })
        );

        this._signals.push(
            metaWindow.connect('notify::above', () => {
                if (!this._disposed && this._window.above)
                    this._window.unmake_above();
            })
        );

        this._scheduleResync();
        _log('INFO', `ManagedWindow: window at (0,0), lowered`);
    }

    private _scheduleResync(): void {
        if (this._resyncId) return;
        this._resyncId = GLib.timeout_add(GLib.PRIORITY_DEFAULT, 250, () => {
            this._resyncId = 0;
            if (this._disposed) return GLib.SOURCE_REMOVE;
            this._window?.move_frame(true, 0, 0);
            this._window?.lower();
            return GLib.SOURCE_REMOVE;
        });
    }

    public destroy(): void {
        this._disposed = true;
        if (this._resyncId) {
            GLib.source_remove(this._resyncId);
            this._resyncId = 0;
        }
        this._signals.forEach(id => {
            try { this._window?.disconnect(id); } catch (_) {}
        });
        this._signals = [];
        (this._window as any) = null;
    }
}

// ─── Extension ────────────────────────────────────────────────────────────────
export default class LumaWallIntegration extends Extension {
    private _managedWindow: ManagedWindow | null = null;
    private _wallpaperActors: Set<LiveWallpaper> = new Set();
    private _overrides: Record<string, any> = {};
    private _mapId: number | null = null;
    private _daemonProxy: any = null;
    private _lockedId: number | null = null;
    private _autoPause: AutoPause | null = null;

    enable(): void {
        _openLog();
        _log('INFO', 'enable()');

        this._managedWindow = null;
        this._wallpaperActors = new Set();
        this._overrides = {};
        this._mapId = null;

        try {
            this._daemonProxy = new DaemonProxy(
                Gio.DBus.session, 'org.lumawall.Daemon', '/org/lumawall/Daemon'
            );
            _log('INFO', 'D-Bus proxy OK');
        } catch (e: any) { _log('ERROR', `D-Bus: ${e.message}`); this._daemonProxy = null; }

        try {
            this._lockedId = Main.sessionMode.connect('updated', () => {
                const locked = Main.sessionMode.isLocked;
                this._daemonProxy?.SetDesktopStateRemote(
                    locked ? 'locked' : 'normal',
                    (r: any, e: any) => e && _log('ERROR', `SetDesktopState: ${e.message}`)
                );
            });
        } catch (e: any) { _log('ERROR', `sessionMode: ${e.message}`); this._lockedId = null; }

        const ext = this;

        // ── 1. Shell.Global.get_window_actors ─────────────────────────────────
        try {
            this._overrides.getWindowActors = Shell.Global.prototype.get_window_actors;
            _getAllWindowActors = () => ext._overrides.getWindowActors.call(global);

            Shell.Global.prototype.get_window_actors = function (this: Shell.Global) {
                return ext._overrides.getWindowActors.call(this).filter(
                    (a: any) => !_isRenderer(a)
                );
            };
            _log('INFO', 'Shell.Global.get_window_actors: overridden');
        } catch (e: any) { _log('ERROR', `get_window_actors: ${e.message}`); }

        // ── 2. BackgroundManager._createBackgroundActor ────────────────────────
        try {
            this._overrides.createBgActor = Background.BackgroundManager.prototype._createBackgroundActor;

            Background.BackgroundManager.prototype._createBackgroundActor = function (this: any) {
                const backgroundActor = ext._overrides.createBgActor.call(this);
                const bgActorWidget = backgroundActor as (Clutter.Actor & { style_class?: string });
                
                // Allow injection everywhere, including the screen-shield (lock screen)
                const isLockScreen = bgActorWidget?.get_parent?.()?.style_class?.includes('screen-shield-background');
                _log('DEBUG', `_createBackgroundActor: monitor ${backgroundActor.monitor}, isLockScreen=${!!isLockScreen}`);
                
                const wp = new LiveWallpaper(backgroundActor, _getAllWindowActors);
                ext._wallpaperActors.add(wp);

                wp._container.connect('destroy', () => {
                    ext._wallpaperActors.delete(wp);
                });

                this.wallpaperActor = wp;
                return backgroundActor;
            };
            _log('INFO', 'BackgroundManager._createBackgroundActor: overridden');
        } catch (e: any) { _log('ERROR', `createBgActor: ${e.message}`); }

        // ── 3. Workspace._isOverviewWindow ────────────────────────────────────
        try {
            this._overrides.workspaceIsOverview = Workspace.Workspace.prototype._isOverviewWindow;
            Workspace.Workspace.prototype._isOverviewWindow = function (this: any, window: any) {
                if (_isRenderer(window)) return false;
                return ext._overrides.workspaceIsOverview.call(this, window);
            };
            _log('INFO', 'Workspace._isOverviewWindow: overridden');
        } catch (e: any) { _log('ERROR', `workspace: ${e.message}`); }

        // ── 4. WorkspaceThumbnail._isOverviewWindow ───────────────────────────
        try {
            this._overrides.thumbnailIsOverview = WorkspaceThumbnail.WorkspaceThumbnail.prototype._isOverviewWindow;
            WorkspaceThumbnail.WorkspaceThumbnail.prototype._isOverviewWindow = function (this: any, window: any) {
                if (_isRenderer(window)) return false;
                return ext._overrides.thumbnailIsOverview.call(this, window);
            };
            _log('INFO', 'WorkspaceThumbnail._isOverviewWindow: overridden');
        } catch (e: any) { _log('ERROR', `thumbnail: ${e.message}`); }

        // ── 5. Meta.Display.get_tab_list ──────────────────────────────────────
        try {
            this._overrides.getTabList = Meta.Display.prototype.get_tab_list;
            Meta.Display.prototype.get_tab_list = function (this: any, type: any, workspace: any) {
                return ext._overrides.getTabList.call(this, type, workspace)
                    .filter((w: any) => !_isRenderer(w));
            };
            _log('INFO', 'Meta.Display.get_tab_list: overridden');
        } catch (e: any) { _log('ERROR', `get_tab_list: ${e.message}`); }

        // ── 6. Shell.WindowTracker.get_window_app ─────────────────────────────
        try {
            this._overrides.getWindowApp = Shell.WindowTracker.prototype.get_window_app;
            Shell.WindowTracker.prototype.get_window_app = function (this: any, window: any) {
                if (_isRenderer(window)) return null as any;
                return ext._overrides.getWindowApp.call(this, window);
            };
            _log('INFO', 'Shell.WindowTracker.get_window_app: overridden');
        } catch (e: any) { _log('ERROR', `get_window_app: ${e.message}`); }

        // ── 7. Shell.App.get_windows ──────────────────────────────────────────
        try {
            this._overrides.appGetWindows = Shell.App.prototype.get_windows;
            Shell.App.prototype.get_windows = function (this: any) {
                return ext._overrides.appGetWindows.call(this).filter((w: any) => !_isRenderer(w));
            };
            _log('INFO', 'Shell.App.get_windows: overridden');
        } catch (e: any) { _log('ERROR', `get_windows: ${e.message}`); }

        // ── 8. Shell.App.get_n_windows ────────────────────────────────────────
        try {
            this._overrides.appGetNWindows = Shell.App.prototype.get_n_windows;
            Shell.App.prototype.get_n_windows = function (this: any) {
                return this.get_windows().length;
            };
            _log('INFO', 'Shell.App.get_n_windows: overridden');
        } catch (e: any) { _log('ERROR', `get_n_windows: ${e.message}`); }

        // ── 9. Shell.AppSystem.get_running ────────────────────────────────────
        try {
            this._overrides.appSystemGetRunning = Shell.AppSystem.prototype.get_running;
            Shell.AppSystem.prototype.get_running = function (this: any) {
                return ext._overrides.appSystemGetRunning.call(this)
                    .filter((app: any) => app.get_n_windows() > 0);
            };
            _log('INFO', 'Shell.AppSystem.get_running: overridden');
        } catch (e: any) { _log('ERROR', `get_running: ${e.message}`); }

        // ── WindowManager: catch LumaWall the moment it maps ─────────────────
        try {
            this._mapId = global.window_manager.connect_after('map', (_wm: any, windowActor: any) => {
                const win = windowActor?.get_meta_window?.();
                if (!win || !_isRenderer(win)) return;

                _log('INFO', 'LumaWall window mapped — starting ManagedWindow');
                this._managedWindow?.destroy();
                this._managedWindow = new ManagedWindow(win);

                this._reloadBackgrounds();
            });
            _log('INFO', 'window_manager.map: listener connected');
        } catch (e: any) { _log('ERROR', `map listener: ${e.message}`); this._mapId = null; }

        this._autoPause = new AutoPause();
        this._autoPause.enable();

        this._reloadBackgrounds();
    }

    private _reloadBackgrounds(): void {
        _log('INFO', '_reloadBackgrounds()');

        this._wallpaperActors?.forEach(wp => { try { wp.destroy(); } catch (_) {} });
        this._wallpaperActors?.clear();

        try {
            global.compositor.get_laters().add(Meta.LaterType.BEFORE_REDRAW, () => {
                try { (Main.layoutManager as any)._updateBackgrounds?.(); } catch (e: any) { _log('ERROR', `_updateBackgrounds: ${e.message}`); }

                try {
                    if ((Main.screenShield as any)?._dialog?._updateBackgrounds != null)
                        (Main.screenShield as any)._dialog._updateBackgrounds();
                } catch (_) {}

                try {
                    (Main.overview as any)._overview._controls._workspacesDisplay._updateWorkspacesViews();
                } catch (_) {}

                return GLib.SOURCE_REMOVE;
            });
        } catch (e: any) { _log('ERROR', `get_laters: ${e.message}`); }

        GLib.timeout_add(GLib.PRIORITY_DEFAULT, BACKGROUND_RELOAD_REFRESH_DELAY_MS, () => {
            const extManager = Main.extensionManager as any;
            if (extManager._enabledExtensions?.includes('blur-my-shell@aunetx')) {
                (Main.layoutManager as any).emit('monitors-changed');
            }

            try { global.display.emit('workareas-changed'); } catch (_) {}
            return GLib.SOURCE_REMOVE;
        });
    }

    disable(): void {
        _log('INFO', 'disable()');

        if (this._autoPause) {
            this._autoPause.disable();
            this._autoPause = null;
        }

        this._managedWindow?.destroy();
        this._managedWindow = null;

        if (this._mapId != null) {
            try { global.window_manager.disconnect(this._mapId); } catch (e: any) { _log('ERROR', e.message); }
            this._mapId = null;
        }

        if (this._lockedId != null) {
            try { Main.sessionMode.disconnect(this._lockedId); } catch (e: any) { _log('ERROR', e.message); }
            this._lockedId = null;
        }
        this._daemonProxy = null;

        const o = this._overrides ?? {};
        const restore = (obj: any, method: string, original: any) => {
            if (!original) return;
            try { obj[method] = original; _log('INFO', `Restored ${method}`); }
            catch (e: any) { _log('ERROR', `Restore ${method}: ${e.message}`); }
        };
        restore(Shell.Global.prototype,          'get_window_actors',     o.getWindowActors);
        restore(Background.BackgroundManager.prototype, '_createBackgroundActor', o.createBgActor);
        restore(Workspace.Workspace.prototype,   '_isOverviewWindow',     o.workspaceIsOverview);
        restore(WorkspaceThumbnail.WorkspaceThumbnail.prototype, '_isOverviewWindow', o.thumbnailIsOverview);
        restore(Meta.Display.prototype,          'get_tab_list',          o.getTabList);
        restore(Shell.WindowTracker.prototype,   'get_window_app',        o.getWindowApp);
        restore(Shell.App.prototype,             'get_windows',           o.appGetWindows);
        restore(Shell.App.prototype,             'get_n_windows',         o.appGetNWindows);
        restore(Shell.AppSystem.prototype,       'get_running',           o.appSystemGetRunning);
        this._overrides = {};

        _getAllWindowActors = () => global.get_window_actors();

        this._reloadBackgrounds();
        _closeLog();
    }
}
