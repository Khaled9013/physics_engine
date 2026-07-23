"""Application-local Qt stylesheet."""

APP_STYLE = """
QMainWindow, QWidget { background: #10151b; color: #d9e4ee; }
QGroupBox { border: 1px solid #2a3743; border-radius: 8px; margin-top: 12px; padding-top: 10px; font-weight: 600; }
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #8fd3ff; }
QDoubleSpinBox, QComboBox { background: #18212a; border: 1px solid #344553; border-radius: 5px; padding: 5px; }
QPushButton { background: #1d6f8a; border: 0; border-radius: 6px; padding: 8px 14px; font-weight: 700; }
QPushButton:hover { background: #268bab; }
QPushButton:disabled { background: #34404a; color: #7d8993; }
QPlainTextEdit { background: #0b0f13; border: 1px solid #26333e; border-radius: 6px; color: #a9c1d3; }
QLabel#metricValue { color: #79e2b5; font-size: 17px; font-weight: 700; }
QFrame#viewportPlaceholder { background: #0a1117; border: 1px solid #30404e; border-radius: 8px; }
"""
