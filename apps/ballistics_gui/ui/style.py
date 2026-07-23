"""Application-local Qt stylesheet."""

APP_STYLE = """
QMainWindow, QWidget {
    background: #0b1014;
    color: #dbe3e6;
    font-family: "Noto Sans", "DejaVu Sans", sans-serif;
    font-size: 13px;
}
QWidget#controlPanel {
    background: #10171c;
    border-left: 1px solid #26333a;
}
QScrollArea#controlScroll,
QWidget#controlScrollContents,
QWidget#metricsGrid {
    background: transparent;
    border: 0;
}
QLabel#eyebrow {
    color: #8a9aa2;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 1px;
}
QLabel#panelTitle {
    color: #f2f4f3;
    font-size: 27px;
    font-weight: 750;
}
QLabel#panelSubtitle,
QLabel#controlHint {
    color: #91a0a7;
    font-size: 11px;
}
QFrame#metricCard {
    background: #151e24;
    border: 1px solid #27343c;
    border-radius: 7px;
}
QLabel#metricLabel {
    color: #778890;
    font-size: 9px;
    font-weight: 700;
}
QLabel#metricValue {
    color: #8fe3b4;
    font-size: 16px;
    font-weight: 700;
}
QGroupBox {
    background: #121a20;
    border: 1px solid #2a373f;
    border-radius: 7px;
    margin-top: 13px;
    padding: 13px 10px 9px 10px;
    font-weight: 700;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 9px;
    padding: 0 5px;
    color: #c4cfd3;
    font-size: 10px;
    letter-spacing: 1px;
}
QDoubleSpinBox, QComboBox {
    min-height: 27px;
    background: #0d1419;
    border: 1px solid #324149;
    border-radius: 5px;
    padding: 2px 7px;
    color: #e1e7e9;
    selection-background-color: #8b652d;
}
QDoubleSpinBox:focus, QComboBox:focus {
    border: 1px solid #cf9e50;
}
QComboBox::drop-down {
    border: 0;
    width: 22px;
}
QToolButton#advancedToggle,
QToolButton#logToggle {
    color: #9fadb3;
    background: transparent;
    border: 0;
    padding: 6px 2px;
    text-align: left;
    font-weight: 650;
}
QToolButton#advancedToggle:hover,
QToolButton#logToggle:hover {
    color: #e1b767;
}
QPushButton {
    background: #26333a;
    color: #dce4e7;
    border: 1px solid #34434b;
    border-radius: 6px;
    padding: 8px 12px;
    font-weight: 700;
}
QPushButton:hover {
    background: #314149;
    border-color: #495b64;
}
QPushButton#fireButton {
    background: #d4a252;
    color: #14181a;
    border: 0;
    font-size: 13px;
    letter-spacing: 1px;
}
QPushButton#fireButton:hover {
    background: #e2b66e;
}
QPushButton#fireButton:pressed {
    background: #bd8b3d;
}
QPushButton:disabled {
    background: #273137;
    color: #6f7b80;
    border-color: #303b40;
}
QPlainTextEdit#eventLog {
    background: #090d10;
    border: 1px solid #26333a;
    border-radius: 6px;
    color: #9bb0b9;
    font-family: "DejaVu Sans Mono", monospace;
    font-size: 10px;
    padding: 5px;
}
QFrame#rangeViewport {
    background: #070a0c;
    border: 0;
}
QSplitter#mainSplitter::handle {
    background: #26333a;
}
QStatusBar {
    background: #0b1014;
    color: #9ba9af;
    border-top: 1px solid #202b31;
    font-size: 11px;
}
QScrollBar:vertical {
    background: transparent;
    width: 8px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: #34434b;
    border-radius: 4px;
    min-height: 28px;
}
QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical,
QScrollBar::add-page:vertical,
QScrollBar::sub-page:vertical {
    background: transparent;
    height: 0;
}
"""
