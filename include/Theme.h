#pragma once
#include <QString>

namespace Theme {

inline constexpr auto BG          = "#0d0d0e";
inline constexpr auto SURFACE     = "#141416";
inline constexpr auto SURFACE2    = "#1c1c1f";
inline constexpr auto SURFACE3    = "#242428";
inline constexpr auto BORDER      = "#2a2a2f";
inline constexpr auto BORDER2     = "#3a3a40";
inline constexpr auto ACCENT      = "#e2f542";
inline constexpr auto ACCENT_DIM  = "#1a1f06";
inline constexpr auto ACCENT2     = "#4da8ff";
inline constexpr auto TEXT        = "#e6e6ea";
inline constexpr auto TEXT_DIM    = "#80808c";
inline constexpr auto TEXT_DIMMER = "#4a4a56";
inline constexpr auto DANGER      = "#ff4455";

inline constexpr int TITLEBAR_H  = 38;
inline constexpr int TABBAR_H    = 38;
inline constexpr int TOOLBAR_H   = 52;   // taller toolbar
inline constexpr int SIDEBAR_W   = 250;
inline constexpr int STATUSBAR_H = 28;

inline QString globalStyleSheet()
{
    return QStringLiteral(R"(
QWidget {
    background-color: #0d0d0e;
    color: #e6e6ea;
    font-family: "Noto Sans Mono", "DejaVu Sans Mono", "Liberation Mono", monospace;
    font-size: 13px;
}
QScrollBar:vertical {
    background: #0d0d0e; width: 9px; border: none;
}
QScrollBar::handle:vertical {
    background: #3a3a40; border-radius: 4px; min-height: 28px;
}
QScrollBar::handle:vertical:hover { background: #555560; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal {
    background: #0d0d0e; height: 9px; border: none;
}
QScrollBar::handle:horizontal {
    background: #3a3a40; border-radius: 4px; min-width: 28px;
}
QScrollBar::handle:horizontal:hover { background: #555560; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QScrollBar::corner { background: #0d0d0e; }
QToolTip {
    background: #1c1c1f; color: #e6e6ea;
    border: 1px solid #3a3a40; border-radius: 4px;
    padding: 5px 10px; font-size: 12px;
}
QMenu {
    background: #1c1c1f; border: 1px solid #3a3a40;
    border-radius: 5px; padding: 4px 0;
}
QMenu::item { padding: 7px 28px 7px 14px; color: #80808c; font-size: 13px; }
QMenu::item:selected { background: #242428; color: #e6e6ea; }
QMenu::item:disabled { color: #4a4a56; }
QMenu::separator { height: 1px; background: #2a2a2f; margin: 3px 0; }
QMessageBox { background: #141416; }
QMessageBox QPushButton {
    background: #1c1c1f; border: 1px solid #3a3a40;
    border-radius: 4px; padding: 7px 18px; color: #e6e6ea;
    min-width: 80px; font-size: 13px;
}
QMessageBox QPushButton:hover { background: #242428; border-color: #e2f542; color: #e2f542; }
QFileDialog { background: #141416; font-size: 13px; }
QLineEdit {
    font-size: 13px;
}
)");
}

} // namespace Theme
