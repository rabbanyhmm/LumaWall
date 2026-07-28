import GObject from 'gi://GObject';
import GLib from 'gi://GLib';
import Meta from 'gi://Meta';
import Gio from 'gi://Gio';

import * as Main from 'resource:///org/gnome/shell/ui/main.js';

function _log(type: string, message: string) {
    console.debug(`[LumaWall:AutoPause] [${type}] ${message}`);
}

const DAEMON_BUS_NAME = 'org.lumawall.Daemon';
const DAEMON_OBJECT_PATH = '/org/lumawall/Daemon';
const DAEMON_INTERFACE_NAME = 'org.lumawall.Daemon';

export class AutoPause {
    private _workspaceManager: Meta.WorkspaceManager | null = null;
    private _activeWorkspace: Meta.Workspace | null = null;
    private _windows: Array<{ metaWindow: Meta.Window; signals: number[] }> = [];
    private _dbusProxy: Gio.DBusProxy | null = null;

    private _activeWorkspaceChangedId: number | null = null;
    private _windowAddedId: number | null = null;
    private _windowRemovedId: number | null = null;
    private _overviewShowingId: number | null = null;
    private _overviewHiddenId: number | null = null;
    
    private _isPaused: boolean = false;

    constructor() {
        Gio.DBusProxy.new_for_bus(
            Gio.BusType.SESSION,
            Gio.DBusProxyFlags.NONE,
            null,
            DAEMON_BUS_NAME,
            DAEMON_OBJECT_PATH,
            DAEMON_INTERFACE_NAME,
            null,
            (sourceObj, res) => {
                try {
                    this._dbusProxy = Gio.DBusProxy.new_for_bus_finish(res);
                    _log('INFO', 'Connected to LumaWall DBus Daemon for AutoPause');
                } catch (e: any) {
                    _log('ERROR', `Failed to connect to DBus Daemon: ${e.message}`);
                }
            }
        );
    }

    public enable(): void {
        this._overviewShowingId = Main.overview.connect('showing', () => this._update());
        this._overviewHiddenId = Main.overview.connect('hidden', () => this._update());

        this._workspaceManager = global.workspace_manager;
        this._activeWorkspace = this._workspaceManager.get_active_workspace();
        
        this._activeWorkspaceChangedId = this._workspaceManager.connect(
            'active-workspace-changed',
            (wm: Meta.WorkspaceManager) => this._onActiveWorkspaceChanged(wm)
        );

        this._activeWorkspace.list_windows().forEach(w => this._onWindowAdded(w, false));
        this._windowAddedId = this._activeWorkspace.connect('window-added', (_ws, window: Meta.Window) => this._onWindowAdded(window));
        this._windowRemovedId = this._activeWorkspace.connect('window-removed', (_ws, window: Meta.Window) => this._onWindowRemoved(window));

        this._update();
    }

    public disable(): void {
        if (this._overviewShowingId) Main.overview.disconnect(this._overviewShowingId);
        if (this._overviewHiddenId) Main.overview.disconnect(this._overviewHiddenId);
        if (this._activeWorkspaceChangedId && this._workspaceManager) {
            this._workspaceManager.disconnect(this._activeWorkspaceChangedId);
        }
        if (this._windowAddedId && this._activeWorkspace) {
            this._activeWorkspace.disconnect(this._windowAddedId);
        }
        if (this._windowRemovedId && this._activeWorkspace) {
            this._activeWorkspace.disconnect(this._windowRemovedId);
        }
        this._clearWindows();
        
        // Resume on disable
        if (this._isPaused && this._dbusProxy) {
            this._dbusProxy.call('Play', null, Gio.DBusCallFlags.NONE, -1, null, null);
        }
    }

    private _clearWindows(): void {
        for (const w of this._windows) {
            for (const sig of w.signals) {
                try { w.metaWindow.disconnect(sig); } catch(e) {}
            }
        }
        this._windows = [];
    }

    private _onActiveWorkspaceChanged(wm: Meta.WorkspaceManager): void {
        this._clearWindows();
        if (this._windowAddedId && this._activeWorkspace) this._activeWorkspace.disconnect(this._windowAddedId);
        if (this._windowRemovedId && this._activeWorkspace) this._activeWorkspace.disconnect(this._windowRemovedId);

        this._activeWorkspace = wm.get_active_workspace();
        this._activeWorkspace.list_windows().forEach(w => this._onWindowAdded(w, false));
        
        this._windowAddedId = this._activeWorkspace.connect('window-added', (_ws, window: Meta.Window) => this._onWindowAdded(window));
        this._windowRemovedId = this._activeWorkspace.connect('window-removed', (_ws, window: Meta.Window) => this._onWindowRemoved(window));

        this._update();
    }

    private _onWindowAdded(metaWindow: Meta.Window, doUpdate = true): void {
        if (metaWindow.title?.includes('LumaWall') || metaWindow.skip_taskbar) return;

        const signals: number[] = [];
        signals.push(metaWindow.connect('notify::maximized-horizontally', () => this._update()));
        signals.push(metaWindow.connect('notify::maximized-vertically', () => this._update()));
        signals.push(metaWindow.connect('notify::fullscreen', () => this._update()));
        signals.push(metaWindow.connect('notify::minimized', () => this._update()));

        this._windows.push({ metaWindow, signals });
        if (doUpdate) this._update();
    }

    private _onWindowRemoved(metaWindow: Meta.Window): void {
        if (metaWindow.title?.includes('LumaWall') || metaWindow.skip_taskbar) return;

        this._windows = this._windows.filter(w => {
            if (w.metaWindow === metaWindow) {
                w.signals.forEach(sig => { try { metaWindow.disconnect(sig); } catch(e) {} });
                return false;
            }
            return true;
        });
        this._update();
    }

    private _update(): void {
        if (!this._dbusProxy) return;

        const inOverview = Main.overview.visible;
        const onLockScreen = Main.sessionMode.currentMode === 'unlock-dialog';
        
        if (inOverview || onLockScreen) {
            // When in overview, we can see the background, so we should play
            this._setPaused(false);
            return;
        }

        const shouldPause = this._windows.some(w => {
            const mw = w.metaWindow;
            return mw.showing_on_its_workspace() && (mw.is_maximized() || mw.fullscreen);
        });

        this._setPaused(shouldPause);
    }

    private _setPaused(pause: boolean): void {
        if (this._isPaused === pause) return;
        this._isPaused = pause;

        if (this._dbusProxy) {
            if (pause) {
                _log('INFO', 'AutoPausing Live Wallpaper (Maximized/Fullscreen Window Detected)');
                this._dbusProxy.call('Pause', null, Gio.DBusCallFlags.NONE, -1, null, null);
            } else {
                _log('INFO', 'AutoResuming Live Wallpaper');
                this._dbusProxy.call('Play', null, Gio.DBusCallFlags.NONE, -1, null, null);
            }
        }
    }
}
