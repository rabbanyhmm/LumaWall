// src/platform/gnome/extension/extension.ts
import { Extension } from "resource:///org/gnome/shell/extensions/extension.js";
import * as Main2 from "resource:///org/gnome/shell/ui/main.js";
import * as Background from "resource:///org/gnome/shell/ui/background.js";
import * as Workspace from "resource:///org/gnome/shell/ui/workspace.js";
import * as WorkspaceThumbnail from "resource:///org/gnome/shell/ui/workspaceThumbnail.js";
import Gio2 from "gi://Gio";
import GLib2 from "gi://GLib";
import Meta from "gi://Meta";
import Shell2 from "gi://Shell";
import Clutter from "gi://Clutter";
import Graphene from "gi://Graphene";

// src/platform/gnome/extension/roundedCornersEffect.ts
import Cogl from "gi://Cogl";
import GLib from "gi://GLib";
import GObject from "gi://GObject";
import Shell from "gi://Shell";
var logger = {
  debug: (...args) => console.debug("[LumaWall:RoundedCornersEffect]", ...args)
};
var LOG_DEBOUNCE_MS = 250;
var fragmentShaderDeclarations = [
  "uniform vec4 bounds;           // x, y: top left; z, w: bottom right     \n",
  "uniform float clip_radius;                                               \n",
  "uniform vec2 pixel_step;                                                 \n",
  "uniform float border_stroke;                                             \n",
  "uniform vec4 border_color;                                               \n",
  "                                                                         \n",
  "float                                                                    \n",
  "rounded_rect_coverage (vec2 p)                                           \n",
  "{                                                                        \n",
  "  float center_left  = bounds.x + clip_radius;                           \n",
  "  float center_right = bounds.z - clip_radius;                           \n",
  "  float center_x;                                                        \n",
  "                                                                         \n",
  "  if (p.x < center_left)                                                 \n",
  "    center_x = center_left;                                              \n",
  "  else if (p.x > center_right)                                           \n",
  "    center_x = center_right;                                             \n",
  "  else                                                                   \n",
  "    return 1.0; // The vast majority of pixels exit early here           \n",
  "                                                                         \n",
  "  float center_top    = bounds.y + clip_radius;                          \n",
  "  float center_bottom = bounds.w - clip_radius;                          \n",
  "  float center_y;                                                        \n",
  "                                                                         \n",
  "  if (p.y < center_top)                                                  \n",
  "    center_y = center_top;                                               \n",
  "  else if (p.y > center_bottom)                                          \n",
  "    center_y = center_bottom;                                            \n",
  "  else                                                                   \n",
  "    return 1.0;                                                          \n",
  "                                                                         \n",
  "  vec2 delta = p - vec2 (center_x, center_y);                            \n",
  "  float dist_squared = dot (delta, delta);                               \n",
  "                                                                         \n",
  "  // Fully outside the circle                                            \n",
  "  float outer_radius = clip_radius + 0.5;                                \n",
  "  if (dist_squared >= (outer_radius * outer_radius))                     \n",
  "    return 0.0;                                                          \n",
  "                                                                         \n",
  "  // Fully inside the circle                                             \n",
  "  float inner_radius = clip_radius - 0.5;                                \n",
  "  if (dist_squared <= (inner_radius * inner_radius))                     \n",
  "    return 1.0;                                                          \n",
  "                                                                         \n",
  "  // Only pixels on the edge of the curve need expensive antialiasing    \n",
  "  return outer_radius - sqrt (dist_squared);                             \n",
  "}                                                                        \n"
].join("");
var fragmentShaderCode = [
  "vec2 texture_coord;                                                      \n",
  "                                                                         \n",
  "texture_coord = cogl_tex_coord0_in.xy / pixel_step;                      \n",
  "                                                                         \n",
  "bool inside = texture_coord.x >= bounds.x && texture_coord.x <= bounds.z \n",
  "           && texture_coord.y >= bounds.y && texture_coord.y <= bounds.w;\n",
  "                                                                         \n",
  "// border stroke for debug purposes                                      \n",
  "bool on_border = border_stroke > 0.0 && inside && (                      \n",
  "    texture_coord.x < bounds.x + border_stroke ||                        \n",
  "    texture_coord.x > bounds.z - border_stroke ||                        \n",
  "    texture_coord.y < bounds.y + border_stroke ||                        \n",
  "    texture_coord.y > bounds.w - border_stroke);                         \n",
  "                                                                         \n",
  "if (on_border)                                                           \n",
  "    cogl_color_out = border_color;                                       \n",
  "else if (clip_radius > 0.0 && !inside)                                   \n",
  "    cogl_color_out = vec4 (0.0);                                         \n",
  "else if (clip_radius > 0.0)                                              \n",
  "    cogl_color_out *= rounded_rect_coverage (texture_coord);             \n"
].join("");
function enlargeForEffects(x1, y1, width, height) {
  if (width <= 0 || height <= 0)
    return [x1, y1];
  const x2 = Math.ceil(x1 + width + 0.75);
  const y2 = Math.ceil(y1 + height + 0.75);
  return [x2 - Math.round(width) - 3, y2 - Math.round(height) - 3];
}
function getFboOffset(actor) {
  const volume = actor.get_paint_volume();
  if (volume) {
    const origin = volume.get_origin();
    const [x12, y12] = enlargeForEffects(
      origin.x,
      origin.y,
      volume.get_width(),
      volume.get_height()
    );
    return [Math.trunc(x12), Math.trunc(y12)];
  }
  const box = actor.get_allocation_box();
  const [x1, y1] = enlargeForEffects(
    box.x1,
    box.y1,
    box.x2 - box.x1,
    box.y2 - box.y1
  );
  return [Math.trunc(x1 - box.x1), Math.trunc(y1 - box.y1)];
}
var RoundedCornersEffect = GObject.registerClass(
  class RoundedCornersEffect2 extends Shell.GLSLEffect {
    // Pending debounced log timers, keyed by label.
    logTimeouts = /* @__PURE__ */ new Map();
    // Uniform values in actor-local logical pixels, as set by callers.
    bounds = [0, 0, 0, 0];
    clipRadius = 0;
    borderStroke = 0;
    uniformsDirty = true;
    // Actor-to-texture mapping the uniforms were last uploaded for.
    textureWidth = 0;
    textureHeight = 0;
    textureScale = 0;
    textureOffsetX = 0;
    textureOffsetY = 0;
    // Logs `label: ...args` only once the value stops changing for LOG_DEBOUNCE_MS.
    debugDebounced(label, ...args) {
      const pending = this.logTimeouts.get(label);
      if (pending)
        GLib.source_remove(pending);
      this.logTimeouts.set(
        label,
        GLib.timeout_add(GLib.PRIORITY_DEFAULT, LOG_DEBOUNCE_MS, () => {
          this.logTimeouts.delete(label);
          logger.debug(`${label}:`, ...args);
          return GLib.SOURCE_REMOVE;
        })
      );
    }
    vfunc_build_pipeline() {
      this.add_glsl_snippet(
        Cogl.SnippetHook.FRAGMENT,
        fragmentShaderDeclarations,
        fragmentShaderCode,
        false
      );
    }
    // The shader compares texture coordinates (converted to texture pixels
    // via pixel_step) against bounds/clip_radius/border_stroke, so those
    // uniforms must be expressed in pixels of the offscreen texture this
    // effect renders into. mutter sizes that texture from the actor's paint
    // box, not its allocation, and the relation between the two varies
    // across mutter versions (e.g. Clutter.Clone paint volumes changed in
    // 50.2, commit d26ac38b) and monitor scales (the texture uses the
    // ceiled resource scale). Instead of predicting the size, read the
    // actual texture at paint time and mirror mutter's actor-to-texture
    // mapping: texture_px = (logical_px - fbo_offset) * resource_scale.
    vfunc_paint_target(node, paintContext) {
      this.updateUniforms();
      super.vfunc_paint_target(node, paintContext);
    }
    updateUniforms() {
      const texture = this.get_texture();
      const actor = this.get_actor();
      if (!texture || !actor)
        return;
      const width = texture.get_width();
      const height = texture.get_height();
      const scale = actor.get_resource_scale();
      const [offsetX, offsetY] = getFboOffset(actor);
      if (!this.uniformsDirty && width === this.textureWidth && height === this.textureHeight && scale === this.textureScale && offsetX === this.textureOffsetX && offsetY === this.textureOffsetY)
        return;
      this.uniformsDirty = false;
      this.textureWidth = width;
      this.textureHeight = height;
      this.textureScale = scale;
      this.textureOffsetX = offsetX;
      this.textureOffsetY = offsetY;
      this.debugDebounced("textureMapping", width, height, scale, offsetX, offsetY);
      this.set_uniform_float(
        this.get_uniform_location("pixel_step"),
        2,
        [1 / width, 1 / height]
      );
      const [x1, y1, x2, y2] = this.bounds;
      this.set_uniform_float(
        this.get_uniform_location("bounds"),
        4,
        [
          (x1 - offsetX) * scale,
          (y1 - offsetY) * scale,
          (x2 - offsetX) * scale,
          (y2 - offsetY) * scale
        ]
      );
      this.set_uniform_float(
        this.get_uniform_location("clip_radius"),
        1,
        [this.clipRadius * scale]
      );
      this.set_uniform_float(
        this.get_uniform_location("border_stroke"),
        1,
        [this.borderStroke * scale]
      );
    }
    setBounds(bounds) {
      this.debugDebounced("bounds", ...bounds);
      this.bounds = bounds;
      this.uniformsDirty = true;
    }
    setClipRadius(clipRadius) {
      this.debugDebounced("clipRadius", clipRadius);
      this.clipRadius = clipRadius;
      this.uniformsDirty = true;
    }
    setBorderStroke(stroke) {
      this.debugDebounced("borderStroke", stroke);
      this.borderStroke = stroke;
      this.uniformsDirty = true;
    }
    setBorderColor(color) {
      this.debugDebounced("borderColor", ...color);
      this.set_uniform_float(
        this.get_uniform_location("border_color"),
        4,
        color
      );
    }
  }
);

// src/platform/gnome/extension/autoPause.ts
import Gio from "gi://Gio";
import * as Main from "resource:///org/gnome/shell/ui/main.js";
function _log(type, message) {
  console.debug(`[LumaWall:AutoPause] [${type}] ${message}`);
}
var DAEMON_BUS_NAME = "org.lumawall.Daemon";
var DAEMON_OBJECT_PATH = "/org/lumawall/Daemon";
var DAEMON_INTERFACE_NAME = "org.lumawall.Daemon";
var AutoPause = class {
  _workspaceManager = null;
  _activeWorkspace = null;
  _windows = [];
  _dbusProxy = null;
  _activeWorkspaceChangedId = null;
  _windowAddedId = null;
  _windowRemovedId = null;
  _overviewShowingId = null;
  _overviewHiddenId = null;
  _isPaused = false;
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
          _log("INFO", "Connected to LumaWall DBus Daemon for AutoPause");
        } catch (e) {
          _log("ERROR", `Failed to connect to DBus Daemon: ${e.message}`);
        }
      }
    );
  }
  enable() {
    this._overviewShowingId = Main.overview.connect("showing", () => this._update());
    this._overviewHiddenId = Main.overview.connect("hidden", () => this._update());
    this._workspaceManager = global.workspace_manager;
    this._activeWorkspace = this._workspaceManager.get_active_workspace();
    this._activeWorkspaceChangedId = this._workspaceManager.connect(
      "active-workspace-changed",
      (wm) => this._onActiveWorkspaceChanged(wm)
    );
    this._activeWorkspace.list_windows().forEach((w) => this._onWindowAdded(w, false));
    this._windowAddedId = this._activeWorkspace.connect("window-added", (_ws, window) => this._onWindowAdded(window));
    this._windowRemovedId = this._activeWorkspace.connect("window-removed", (_ws, window) => this._onWindowRemoved(window));
    this._update();
  }
  disable() {
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
    if (this._isPaused && this._dbusProxy) {
      this._dbusProxy.call("Play", null, Gio.DBusCallFlags.NONE, -1, null, null);
    }
  }
  _clearWindows() {
    for (const w of this._windows) {
      for (const sig of w.signals) {
        try {
          w.metaWindow.disconnect(sig);
        } catch (e) {
        }
      }
    }
    this._windows = [];
  }
  _onActiveWorkspaceChanged(wm) {
    this._clearWindows();
    if (this._windowAddedId && this._activeWorkspace) this._activeWorkspace.disconnect(this._windowAddedId);
    if (this._windowRemovedId && this._activeWorkspace) this._activeWorkspace.disconnect(this._windowRemovedId);
    this._activeWorkspace = wm.get_active_workspace();
    this._activeWorkspace.list_windows().forEach((w) => this._onWindowAdded(w, false));
    this._windowAddedId = this._activeWorkspace.connect("window-added", (_ws, window) => this._onWindowAdded(window));
    this._windowRemovedId = this._activeWorkspace.connect("window-removed", (_ws, window) => this._onWindowRemoved(window));
    this._update();
  }
  _onWindowAdded(metaWindow, doUpdate = true) {
    if (metaWindow.title?.includes("LumaWall") || metaWindow.skip_taskbar) return;
    const signals = [];
    signals.push(metaWindow.connect("notify::maximized-horizontally", () => this._update()));
    signals.push(metaWindow.connect("notify::maximized-vertically", () => this._update()));
    signals.push(metaWindow.connect("notify::fullscreen", () => this._update()));
    signals.push(metaWindow.connect("notify::minimized", () => this._update()));
    this._windows.push({ metaWindow, signals });
    if (doUpdate) this._update();
  }
  _onWindowRemoved(metaWindow) {
    if (metaWindow.title?.includes("LumaWall") || metaWindow.skip_taskbar) return;
    this._windows = this._windows.filter((w) => {
      if (w.metaWindow === metaWindow) {
        w.signals.forEach((sig) => {
          try {
            metaWindow.disconnect(sig);
          } catch (e) {
          }
        });
        return false;
      }
      return true;
    });
    this._update();
  }
  _update() {
    if (!this._dbusProxy) return;
    const inOverview = Main.overview.visible;
    const onLockScreen = Main.sessionMode.currentMode === "unlock-dialog";
    if (inOverview || onLockScreen) {
      this._setPaused(false);
      return;
    }
    const shouldPause = this._windows.some((w) => {
      const mw = w.metaWindow;
      return mw.showing_on_its_workspace() && (mw.is_maximized() || mw.fullscreen);
    });
    this._setPaused(shouldPause);
  }
  _setPaused(pause) {
    if (this._isPaused === pause) return;
    this._isPaused = pause;
    if (this._dbusProxy) {
      if (pause) {
        _log("INFO", "AutoPausing Live Wallpaper (Maximized/Fullscreen Window Detected)");
        this._dbusProxy.call("Pause", null, Gio.DBusCallFlags.NONE, -1, null, null);
      } else {
        _log("INFO", "AutoResuming Live Wallpaper");
        this._dbusProxy.call("Play", null, Gio.DBusCallFlags.NONE, -1, null, null);
      }
    }
  }
};

// src/platform/gnome/extension/extension.ts
var LUMAWALL_TITLE = "LumaWall-RendererWindow-X11";
var BACKGROUND_RELOAD_REFRESH_DELAY_MS = 500;
var LOG_FILE = "/tmp/lumawall-extension.log";
var _logFile = null;
function _openLog() {
  try {
    _logFile = Gio2.File.new_for_path(LOG_FILE).replace(null, false, Gio2.FileCreateFlags.NONE, null);
    _log2("INFO", "=== LumaWall Extension v5 ===");
  } catch (e) {
    console.error("[LumaWall] log open failed:", e.message);
  }
}
function _closeLog() {
  _log2("INFO", "=== Stopping ===");
  try {
    _logFile?.close(null);
  } catch (_) {
  }
  _logFile = null;
}
function _log2(level, msg) {
  const line = `[${(/* @__PURE__ */ new Date()).toISOString()}] [${level}] ${msg}
`;
  console.log(`[LumaWall] [${level}] ${msg}`);
  try {
    _logFile?.write(line, null);
    _logFile?.flush(null);
  } catch (_) {
  }
}
var DaemonProxy = Gio2.DBusProxy.makeProxyWrapper(`
<node>
  <interface name="org.lumawall.Daemon">
    <method name="SetDesktopState">
      <arg type="s" name="state" direction="in"/>
    </method>
  </interface>
</node>`);
function _isRenderer(actorOrWindow) {
  if (!actorOrWindow) return false;
  const title = actorOrWindow.get_title?.() ?? actorOrWindow.meta_window?.get_title?.() ?? actorOrWindow.title;
  return typeof title === "string" && title.includes(LUMAWALL_TITLE);
}
var _getAllWindowActors = () => global.get_window_actors();
var LiveWallpaper = class {
  _backgroundActor;
  _getAllWindowActors;
  _monitorIndex;
  _clone;
  _sourceDestroyId;
  _pollTimeoutId;
  _disposed;
  _container;
  _roundedCornersEffect;
  monitorWidth;
  monitorHeight;
  constructor(backgroundActor, getAllWindowActors) {
    this._backgroundActor = backgroundActor;
    this._getAllWindowActors = getAllWindowActors;
    this._monitorIndex = backgroundActor.monitor ?? 0;
    this._clone = null;
    this._sourceDestroyId = null;
    this._pollTimeoutId = 0;
    this._disposed = false;
    this._container = new St.Widget({
      layout_manager: new Clutter.BinLayout(),
      clip_to_allocation: true
    });
    this._container.add_constraint(new Clutter.BindConstraint({
      source: backgroundActor,
      coordinate: Clutter.BindCoordinate.SIZE
    }));
    backgroundActor.add_child(this._container);
    const monitor = Main2.layoutManager.monitors[this._monitorIndex] || { width: 1920, height: 1080 };
    this.monitorWidth = monitor.width;
    this.monitorHeight = monitor.height;
    this._roundedCornersEffect = new RoundedCornersEffect();
    this._backgroundActor.add_effect(this._roundedCornersEffect);
    this._roundedCornersEffect.setClipRadius(0);
    this._roundedCornersEffect.setBorderStroke(0);
    this._roundedCornersEffect.setBorderColor([1, 0, 0, 1]);
    this._roundedCornersEffect.setBounds([0, 0, this.monitorWidth, this.monitorHeight]);
    this._container.connect("notify::allocation", () => this._applyBounds());
    this._container.connect("destroy", () => {
      this._disposed = true;
      this._clearPoll();
      this._destroyClone();
    });
    _log2("INFO", `LiveWallpaper created for monitor ${this._monitorIndex}`);
    this._applyWallpaper();
  }
  _applyBounds() {
    const monitor = Main2.layoutManager.monitors[this._monitorIndex];
    if (!monitor) return;
    const workArea = Main2.layoutManager.getWorkAreaForMonitor(this._monitorIndex);
    if (!workArea) return;
    const panelOffset = (workArea.y - monitor.y) / monitor.height * this._backgroundActor.height;
    this._roundedCornersEffect.setBounds(
      [0, panelOffset, this._container.width, this._container.height]
    );
  }
  _clearPoll() {
    if (this._pollTimeoutId) {
      GLib2.source_remove(this._pollTimeoutId);
      this._pollTimeoutId = 0;
    }
  }
  _destroyClone() {
    if (this._clone) {
      if (this._sourceDestroyId && this._clone.source) {
        try {
          this._clone.source.disconnect(this._sourceDestroyId);
        } catch (_) {
        }
        this._sourceDestroyId = null;
      }
      this._clone.set_source(null);
      this._clone.destroy();
      this._clone = null;
    }
  }
  destroy() {
    this._disposed = true;
    this._clearPoll();
    this._destroyClone();
    try {
      this._container?.destroy();
    } catch (_) {
    }
  }
  _getRenderer() {
    const actors = this._getAllWindowActors();
    _log2("DEBUG", `_getRenderer: ${actors.length} unfiltered actors`);
    for (const actor of actors) {
      if (!_isRenderer(actor)) continue;
      const monitorIdx = actor.meta_window?.get_monitor?.() ?? -1;
      _log2("DEBUG", `  Found LumaWall actor on monitor ${monitorIdx}`);
      if (monitorIdx === this._monitorIndex) return actor;
    }
    return null;
  }
  _applyWallpaper() {
    if (this._disposed) return;
    const attempt = () => {
      if (this._disposed) return false;
      const renderer = this._getRenderer();
      if (renderer) {
        this._clone = new Clutter.Clone({
          source: renderer,
          pivot_point: new Graphene.Point({ x: 0.5, y: 0.5 }),
          x_expand: true,
          y_expand: true
        });
        this._clone.connect("destroy", () => {
          this._clone = null;
        });
        this._sourceDestroyId = renderer.connect("destroy", () => {
          _log2("WARN", `Renderer destroyed on monitor ${this._monitorIndex}, will retry`);
          this._destroyClone();
          if (!this._disposed) this._applyWallpaper();
        });
        this._container.add_child(this._clone);
        this._container.ease({
          opacity: 255,
          duration: 1e3,
          mode: Clutter.AnimationMode.EASE_OUT_QUAD
        });
        _log2("INFO", `\u2713 Clone attached for monitor ${this._monitorIndex}`);
        this._pollTimeoutId = 0;
        return false;
      }
      return true;
    };
    if (attempt()) {
      this._pollTimeoutId = GLib2.timeout_add(GLib2.PRIORITY_DEFAULT, 1e3, attempt);
    }
  }
};
var ManagedWindow = class {
  _window;
  _signals;
  _resyncId;
  _disposed;
  constructor(metaWindow) {
    this._window = metaWindow;
    this._signals = [];
    this._resyncId = 0;
    this._disposed = false;
    this._window.move_frame(true, 0, 0);
    this._window.lower();
    this._signals.push(
      metaWindow.connect_after("raised", () => {
        if (!this._disposed) this._window.lower();
      })
    );
    this._signals.push(
      metaWindow.connect("position-changed", () => {
        if (!this._disposed) this._window.move_frame(true, 0, 0);
      })
    );
    this._signals.push(
      metaWindow.connect("notify::above", () => {
        if (!this._disposed && this._window.above)
          this._window.unmake_above();
      })
    );
    this._scheduleResync();
    _log2("INFO", `ManagedWindow: window at (0,0), lowered`);
  }
  _scheduleResync() {
    if (this._resyncId) return;
    this._resyncId = GLib2.timeout_add(GLib2.PRIORITY_DEFAULT, 250, () => {
      this._resyncId = 0;
      if (this._disposed) return GLib2.SOURCE_REMOVE;
      this._window?.move_frame(true, 0, 0);
      this._window?.lower();
      return GLib2.SOURCE_REMOVE;
    });
  }
  destroy() {
    this._disposed = true;
    if (this._resyncId) {
      GLib2.source_remove(this._resyncId);
      this._resyncId = 0;
    }
    this._signals.forEach((id) => {
      try {
        this._window?.disconnect(id);
      } catch (_) {
      }
    });
    this._signals = [];
    this._window = null;
  }
};
var LumaWallIntegration = class extends Extension {
  _managedWindow = null;
  _wallpaperActors = /* @__PURE__ */ new Set();
  _overrides = {};
  _mapId = null;
  _daemonProxy = null;
  _lockedId = null;
  _autoPause = null;
  enable() {
    _openLog();
    _log2("INFO", "enable()");
    this._managedWindow = null;
    this._wallpaperActors = /* @__PURE__ */ new Set();
    this._overrides = {};
    this._mapId = null;
    try {
      this._daemonProxy = new DaemonProxy(
        Gio2.DBus.session,
        "org.lumawall.Daemon",
        "/org/lumawall/Daemon"
      );
      _log2("INFO", "D-Bus proxy OK");
    } catch (e) {
      _log2("ERROR", `D-Bus: ${e.message}`);
      this._daemonProxy = null;
    }
    try {
      this._lockedId = Main2.sessionMode.connect("updated", () => {
        const locked = Main2.sessionMode.isLocked;
        this._daemonProxy?.SetDesktopStateRemote(
          locked ? "locked" : "normal",
          (r, e) => e && _log2("ERROR", `SetDesktopState: ${e.message}`)
        );
      });
    } catch (e) {
      _log2("ERROR", `sessionMode: ${e.message}`);
      this._lockedId = null;
    }
    const ext = this;
    try {
      this._overrides.getWindowActors = Shell2.Global.prototype.get_window_actors;
      _getAllWindowActors = () => ext._overrides.getWindowActors.call(global);
      Shell2.Global.prototype.get_window_actors = function() {
        return ext._overrides.getWindowActors.call(this).filter(
          (a) => !_isRenderer(a)
        );
      };
      _log2("INFO", "Shell.Global.get_window_actors: overridden");
    } catch (e) {
      _log2("ERROR", `get_window_actors: ${e.message}`);
    }
    try {
      this._overrides.createBgActor = Background.BackgroundManager.prototype._createBackgroundActor;
      Background.BackgroundManager.prototype._createBackgroundActor = function() {
        const backgroundActor = ext._overrides.createBgActor.call(this);
        const bgActorWidget = backgroundActor;
        const isLockScreen = bgActorWidget?.get_parent?.()?.style_class?.includes("screen-shield-background");
        _log2("DEBUG", `_createBackgroundActor: monitor ${backgroundActor.monitor}, isLockScreen=${!!isLockScreen}`);
        const wp = new LiveWallpaper(backgroundActor, _getAllWindowActors);
        ext._wallpaperActors.add(wp);
        wp._container.connect("destroy", () => {
          ext._wallpaperActors.delete(wp);
        });
        this.wallpaperActor = wp;
        return backgroundActor;
      };
      _log2("INFO", "BackgroundManager._createBackgroundActor: overridden");
    } catch (e) {
      _log2("ERROR", `createBgActor: ${e.message}`);
    }
    try {
      this._overrides.workspaceIsOverview = Workspace.Workspace.prototype._isOverviewWindow;
      Workspace.Workspace.prototype._isOverviewWindow = function(window) {
        if (_isRenderer(window)) return false;
        return ext._overrides.workspaceIsOverview.call(this, window);
      };
      _log2("INFO", "Workspace._isOverviewWindow: overridden");
    } catch (e) {
      _log2("ERROR", `workspace: ${e.message}`);
    }
    try {
      this._overrides.thumbnailIsOverview = WorkspaceThumbnail.WorkspaceThumbnail.prototype._isOverviewWindow;
      WorkspaceThumbnail.WorkspaceThumbnail.prototype._isOverviewWindow = function(window) {
        if (_isRenderer(window)) return false;
        return ext._overrides.thumbnailIsOverview.call(this, window);
      };
      _log2("INFO", "WorkspaceThumbnail._isOverviewWindow: overridden");
    } catch (e) {
      _log2("ERROR", `thumbnail: ${e.message}`);
    }
    try {
      this._overrides.getTabList = Meta.Display.prototype.get_tab_list;
      Meta.Display.prototype.get_tab_list = function(type, workspace) {
        return ext._overrides.getTabList.call(this, type, workspace).filter((w) => !_isRenderer(w));
      };
      _log2("INFO", "Meta.Display.get_tab_list: overridden");
    } catch (e) {
      _log2("ERROR", `get_tab_list: ${e.message}`);
    }
    try {
      this._overrides.getWindowApp = Shell2.WindowTracker.prototype.get_window_app;
      Shell2.WindowTracker.prototype.get_window_app = function(window) {
        if (_isRenderer(window)) return null;
        return ext._overrides.getWindowApp.call(this, window);
      };
      _log2("INFO", "Shell.WindowTracker.get_window_app: overridden");
    } catch (e) {
      _log2("ERROR", `get_window_app: ${e.message}`);
    }
    try {
      this._overrides.appGetWindows = Shell2.App.prototype.get_windows;
      Shell2.App.prototype.get_windows = function() {
        return ext._overrides.appGetWindows.call(this).filter((w) => !_isRenderer(w));
      };
      _log2("INFO", "Shell.App.get_windows: overridden");
    } catch (e) {
      _log2("ERROR", `get_windows: ${e.message}`);
    }
    try {
      this._overrides.appGetNWindows = Shell2.App.prototype.get_n_windows;
      Shell2.App.prototype.get_n_windows = function() {
        return this.get_windows().length;
      };
      _log2("INFO", "Shell.App.get_n_windows: overridden");
    } catch (e) {
      _log2("ERROR", `get_n_windows: ${e.message}`);
    }
    try {
      this._overrides.appSystemGetRunning = Shell2.AppSystem.prototype.get_running;
      Shell2.AppSystem.prototype.get_running = function() {
        return ext._overrides.appSystemGetRunning.call(this).filter((app) => app.get_n_windows() > 0);
      };
      _log2("INFO", "Shell.AppSystem.get_running: overridden");
    } catch (e) {
      _log2("ERROR", `get_running: ${e.message}`);
    }
    try {
      this._mapId = global.window_manager.connect_after("map", (_wm, windowActor) => {
        const win = windowActor?.get_meta_window?.();
        if (!win || !_isRenderer(win)) return;
        _log2("INFO", "LumaWall window mapped \u2014 starting ManagedWindow");
        this._managedWindow?.destroy();
        this._managedWindow = new ManagedWindow(win);
        this._reloadBackgrounds();
      });
      _log2("INFO", "window_manager.map: listener connected");
    } catch (e) {
      _log2("ERROR", `map listener: ${e.message}`);
      this._mapId = null;
    }
    this._autoPause = new AutoPause();
    this._autoPause.enable();
    this._reloadBackgrounds();
  }
  _reloadBackgrounds() {
    _log2("INFO", "_reloadBackgrounds()");
    this._wallpaperActors?.forEach((wp) => {
      try {
        wp.destroy();
      } catch (_) {
      }
    });
    this._wallpaperActors?.clear();
    try {
      global.compositor.get_laters().add(Meta.LaterType.BEFORE_REDRAW, () => {
        try {
          Main2.layoutManager._updateBackgrounds?.();
        } catch (e) {
          _log2("ERROR", `_updateBackgrounds: ${e.message}`);
        }
        try {
          if (Main2.screenShield?._dialog?._updateBackgrounds != null)
            Main2.screenShield._dialog._updateBackgrounds();
        } catch (_) {
        }
        try {
          Main2.overview._overview._controls._workspacesDisplay._updateWorkspacesViews();
        } catch (_) {
        }
        return GLib2.SOURCE_REMOVE;
      });
    } catch (e) {
      _log2("ERROR", `get_laters: ${e.message}`);
    }
    GLib2.timeout_add(GLib2.PRIORITY_DEFAULT, BACKGROUND_RELOAD_REFRESH_DELAY_MS, () => {
      const extManager = Main2.extensionManager;
      if (extManager._enabledExtensions?.includes("blur-my-shell@aunetx")) {
        Main2.layoutManager.emit("monitors-changed");
      }
      try {
        global.display.emit("workareas-changed");
      } catch (_) {
      }
      return GLib2.SOURCE_REMOVE;
    });
  }
  disable() {
    _log2("INFO", "disable()");
    if (this._autoPause) {
      this._autoPause.disable();
      this._autoPause = null;
    }
    this._managedWindow?.destroy();
    this._managedWindow = null;
    if (this._mapId != null) {
      try {
        global.window_manager.disconnect(this._mapId);
      } catch (e) {
        _log2("ERROR", e.message);
      }
      this._mapId = null;
    }
    if (this._lockedId != null) {
      try {
        Main2.sessionMode.disconnect(this._lockedId);
      } catch (e) {
        _log2("ERROR", e.message);
      }
      this._lockedId = null;
    }
    this._daemonProxy = null;
    const o = this._overrides ?? {};
    const restore = (obj, method, original) => {
      if (!original) return;
      try {
        obj[method] = original;
        _log2("INFO", `Restored ${method}`);
      } catch (e) {
        _log2("ERROR", `Restore ${method}: ${e.message}`);
      }
    };
    restore(Shell2.Global.prototype, "get_window_actors", o.getWindowActors);
    restore(Background.BackgroundManager.prototype, "_createBackgroundActor", o.createBgActor);
    restore(Workspace.Workspace.prototype, "_isOverviewWindow", o.workspaceIsOverview);
    restore(WorkspaceThumbnail.WorkspaceThumbnail.prototype, "_isOverviewWindow", o.thumbnailIsOverview);
    restore(Meta.Display.prototype, "get_tab_list", o.getTabList);
    restore(Shell2.WindowTracker.prototype, "get_window_app", o.getWindowApp);
    restore(Shell2.App.prototype, "get_windows", o.appGetWindows);
    restore(Shell2.App.prototype, "get_n_windows", o.appGetNWindows);
    restore(Shell2.AppSystem.prototype, "get_running", o.appSystemGetRunning);
    this._overrides = {};
    _getAllWindowActors = () => global.get_window_actors();
    this._reloadBackgrounds();
    _closeLog();
  }
};
export {
  LumaWallIntegration as default
};
