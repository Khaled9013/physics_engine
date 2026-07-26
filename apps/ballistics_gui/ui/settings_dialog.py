"""Escape-key settings panel for display and aim controls.

The panel is deliberately small: it covers the two things a user cannot change any other way
while playing — how large the window is and how fast the view turns. Scenario and physics
values stay on the main control panel, because they are part of the shot rather than part of
the setup.

Every control applies immediately. There is no OK/Cancel pair, because aim speed can only be
judged by moving the view, which requires the change to already be in effect.
"""

from __future__ import annotations

from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtWidgets import (
    QComboBox,
    QDialog,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QSlider,
    QVBoxLayout,
)


WINDOW_SIZE_PRESETS: tuple[tuple[str, int, int], ...] = (
    ("1280 x 720", 1280, 720),
    ("1440 x 900", 1440, 900),
    ("1600 x 900", 1600, 900),
    ("1760 x 990", 1760, 990),
    ("1920 x 1080", 1920, 1080),
    ("2560 x 1440", 2560, 1440),
)

DISPLAY_MODE_WINDOWED = "Windowed"
DISPLAY_MODE_FULLSCREEN = "Fullscreen"


def describe_sensitivity(setting: float, degrees_per_count: float) -> str:
    """Return the readout shown beside the aim-speed slider."""

    if setting <= 0.5:
        pace = "slowest"
    elif setting < 25.0:
        pace = "very slow"
    elif setting < 45.0:
        pace = "slow"
    elif setting < 65.0:
        pace = "medium"
    elif setting < 85.0:
        pace = "fast"
    else:
        pace = "very fast"
    return f"{setting:.0f}  ·  {pace}  ·  {degrees_per_count:.4f}°/count"


class SettingsDialog(QDialog):
    """Modal settings panel raised by the escape key."""

    sensitivity_changed = pyqtSignal(float)
    display_mode_changed = pyqtSignal(str)
    window_size_changed = pyqtSignal(int, int)

    def __init__(
        self,
        *,
        sensitivity: float,
        fullscreen: bool,
        window_size: tuple[int, int],
        sensitivity_preview,
        parent=None,
    ) -> None:
        super().__init__(parent)
        self._sensitivity_preview = sensitivity_preview
        self.setObjectName("settingsDialog")
        self.setWindowTitle("Range settings")
        self.setModal(True)
        self.setMinimumWidth(430)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(20, 18, 20, 16)
        layout.setSpacing(14)

        heading = QLabel("Settings")
        heading.setObjectName("panelTitle")
        layout.addWidget(heading)
        hint = QLabel("Changes apply immediately. Press Escape or Resume to return.")
        hint.setObjectName("panelSubtitle")
        hint.setWordWrap(True)
        layout.addWidget(hint)

        layout.addWidget(self._build_display_group(fullscreen, window_size))
        layout.addWidget(self._build_control_group(sensitivity))
        layout.addStretch(1)

        resume = QPushButton("Resume")
        resume.setObjectName("fireButton")
        resume.clicked.connect(self.accept)
        layout.addWidget(resume)

    def _build_display_group(self, fullscreen: bool, window_size: tuple[int, int]) -> QGroupBox:
        group = QGroupBox("DISPLAY")
        form = QFormLayout(group)
        form.setContentsMargins(10, 6, 10, 4)
        form.setSpacing(9)

        self.mode_box = QComboBox()
        self.mode_box.addItems([DISPLAY_MODE_WINDOWED, DISPLAY_MODE_FULLSCREEN])
        self.mode_box.setCurrentText(
            DISPLAY_MODE_FULLSCREEN if fullscreen else DISPLAY_MODE_WINDOWED
        )
        self.mode_box.currentTextChanged.connect(self._mode_selected)
        form.addRow("Mode", self.mode_box)

        self.size_box = QComboBox()
        for label, width, height in WINDOW_SIZE_PRESETS:
            self.size_box.addItem(label, (width, height))
        self._select_nearest_size(window_size)
        self.size_box.currentIndexChanged.connect(self._size_selected)
        form.addRow("Window size", self.size_box)

        self.size_box.setEnabled(not fullscreen)
        return group

    def _build_control_group(self, sensitivity: float) -> QGroupBox:
        group = QGroupBox("CONTROLS")
        layout = QVBoxLayout(group)
        layout.setContentsMargins(10, 6, 10, 4)
        layout.setSpacing(7)

        caption = QHBoxLayout()
        caption.addWidget(QLabel("Aim speed"))
        caption.addStretch(1)
        self.sensitivity_readout = QLabel()
        self.sensitivity_readout.setObjectName("controlHint")
        caption.addWidget(self.sensitivity_readout)
        layout.addLayout(caption)

        self.sensitivity_slider = QSlider(Qt.Orientation.Horizontal)
        self.sensitivity_slider.setObjectName("sensitivitySlider")
        self.sensitivity_slider.setRange(0, 100)
        self.sensitivity_slider.setSingleStep(1)
        self.sensitivity_slider.setPageStep(5)
        self.sensitivity_slider.setValue(int(round(sensitivity)))
        self.sensitivity_slider.valueChanged.connect(self._sensitivity_selected)
        layout.addWidget(self.sensitivity_slider)

        note = QLabel(
            "Applies to hip aim. The optic turns proportionally slower so the magnified "
            "view stays controllable."
        )
        note.setObjectName("controlHint")
        note.setWordWrap(True)
        layout.addWidget(note)

        self._update_sensitivity_readout(self.sensitivity_slider.value())
        return group

    def _select_nearest_size(self, window_size: tuple[int, int]) -> None:
        """Preselect the preset closest to the window's present size."""

        width, height = window_size
        best_index = 0
        best_distance = None
        for index in range(self.size_box.count()):
            preset_width, preset_height = self.size_box.itemData(index)
            distance = abs(preset_width - width) + abs(preset_height - height)
            if best_distance is None or distance < best_distance:
                best_distance = distance
                best_index = index
        self.size_box.setCurrentIndex(best_index)

    def _update_sensitivity_readout(self, value: int) -> None:
        self.sensitivity_readout.setText(
            describe_sensitivity(float(value), self._sensitivity_preview(float(value)))
        )

    def _sensitivity_selected(self, value: int) -> None:
        self._update_sensitivity_readout(value)
        self.sensitivity_changed.emit(float(value))

    def _mode_selected(self, mode: str) -> None:
        self.size_box.setEnabled(mode == DISPLAY_MODE_WINDOWED)
        self.display_mode_changed.emit(mode)

    def _size_selected(self, index: int) -> None:
        data = self.size_box.itemData(index)
        if data is not None:
            self.window_size_changed.emit(int(data[0]), int(data[1]))
