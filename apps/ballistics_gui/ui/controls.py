"""Scenario controls for the native application."""

from __future__ import annotations

from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtWidgets import (
    QComboBox,
    QDoubleSpinBox,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QScrollArea,
    QSizePolicy,
    QToolButton,
    QVBoxLayout,
    QWidget,
)

from ..simulation.models import ScenarioConfig


class ScenarioControls(QWidget):
    """Compact primary controls with the full solver exposed on demand."""

    fire_requested = pyqtSignal()
    scope_requested = pyqtSignal()
    reset_requested = pyqtSignal()

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._fields: dict[str, QDoubleSpinBox] = {}
        outer = QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.setSpacing(10)

        scroll = QScrollArea()
        scroll.setObjectName("controlScroll")
        scroll.setWidgetResizable(True)
        scroll.setFrameShape(QScrollArea.Shape.NoFrame)
        panel = QWidget()
        panel.setObjectName("controlScrollContents")
        panel_layout = QVBoxLayout(panel)
        panel_layout.setContentsMargins(0, 0, 5, 0)
        panel_layout.setSpacing(10)

        intro = QLabel(
            "Adjust the fictional shot, then fire to run the deterministic C solver."
        )
        intro.setObjectName("controlHint")
        intro.setWordWrap(True)
        panel_layout.addWidget(intro)
        panel_layout.addWidget(self._build_primary_group())

        self.advanced_toggle = QToolButton()
        self.advanced_toggle.setObjectName("advancedToggle")
        self.advanced_toggle.setText("Advanced physics")
        self.advanced_toggle.setCheckable(True)
        self.advanced_toggle.setChecked(False)
        self.advanced_toggle.setArrowType(Qt.ArrowType.RightArrow)
        self.advanced_toggle.setToolButtonStyle(
            Qt.ToolButtonStyle.ToolButtonTextBesideIcon
        )
        self.advanced_toggle.toggled.connect(self._set_advanced_visible)
        panel_layout.addWidget(self.advanced_toggle)

        self.advanced_panel = self._build_advanced_panel()
        self.advanced_panel.hide()
        panel_layout.addWidget(self.advanced_panel)
        panel_layout.addStretch(1)
        scroll.setWidget(panel)
        outer.addWidget(scroll, 1)
        outer.addWidget(self._build_action_bar())

    def _build_primary_group(self) -> QGroupBox:
        group = QGroupBox("QUICK SHOT")
        form = QFormLayout(group)
        form.setHorizontalSpacing(12)
        form.setVerticalSpacing(9)
        self._add_field(
            form, "Target", "target_distance_m", 25.0, 1000.0, 150.0, 1, "m"
        )
        self._add_field(
            form, "Launch speed", "launch_speed_mps", 0.0, 1500.0, 310.0, 1, "m/s"
        )
        self._add_field(
            form, "Elevation", "elevation_deg", -10.0, 85.0, 0.2, 3, "deg"
        )
        self._add_field(
            form, "Azimuth", "azimuth_deg", -90.0, 90.0, 0.0, 3, "deg"
        )
        self._add_field(
            form, "Crosswind", "wind_y_mps", -200.0, 200.0, 2.0, 2, "m/s"
        )
        return group

    def _build_advanced_panel(self) -> QWidget:
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(10)

        numerical = QGroupBox("SOLVER")
        numerical_form = QFormLayout(numerical)
        self.integrator = QComboBox()
        self.integrator.addItems(["rk4.v1", "euler.v1"])
        numerical_form.addRow("Integrator", self.integrator)
        self._add_field(
            numerical_form,
            "Time step",
            "time_step_s",
            0.0001,
            0.05,
            0.002,
            4,
            "s",
        )
        self._add_field(
            numerical_form,
            "Maximum time",
            "maximum_time_s",
            0.05,
            30.0,
            5.0,
            2,
            "s",
        )
        self._add_field(
            numerical_form,
            "Maximum range",
            "maximum_distance_m",
            10.0,
            20000.0,
            5000.0,
            1,
            "m",
        )
        self._add_field(
            numerical_form,
            "Launch height",
            "initial_height_m",
            0.01,
            1000.0,
            1.5,
            2,
            "m",
        )
        layout.addWidget(numerical)

        projectile = QGroupBox("SYNTHETIC PROJECTILE")
        projectile_form = QFormLayout(projectile)
        self._add_field(
            projectile_form,
            "Mass",
            "projectile_mass_kg",
            0.0001,
            100.0,
            0.018,
            4,
            "kg",
        )
        self._add_field(
            projectile_form,
            "Diameter",
            "projectile_diameter_m",
            0.0,
            1.0,
            0.009,
            4,
            "m",
        )
        self._add_field(
            projectile_form,
            "Reference area",
            "reference_area_m2",
            1.0e-9,
            1.0,
            6.3617e-5,
            8,
            "m²",
        )
        self._add_field(
            projectile_form,
            "Drag coefficient",
            "drag_coefficient",
            0.0,
            5.0,
            0.29,
            3,
            "",
        )
        layout.addWidget(projectile)

        environment = QGroupBox("ENVIRONMENT")
        environment_form = QFormLayout(environment)
        self._add_field(
            environment_form,
            "Air density",
            "air_density_kgpm3",
            0.0,
            10.0,
            1.225,
            3,
            "kg/m³",
        )
        self._add_field(
            environment_form,
            "Headwind",
            "wind_x_mps",
            -200.0,
            200.0,
            0.0,
            2,
            "m/s",
        )
        self._add_field(
            environment_form,
            "Vertical wind",
            "wind_z_mps",
            -200.0,
            200.0,
            0.0,
            2,
            "m/s",
        )
        self._add_field(
            environment_form,
            "Gravity",
            "gravity_mps2",
            0.0,
            50.0,
            9.80665,
            5,
            "m/s²",
        )
        layout.addWidget(environment)
        return panel

    def _build_action_bar(self) -> QWidget:
        actions = QWidget()
        layout = QVBoxLayout(actions)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(7)

        self.fire_button = QPushButton("FIRE SHOT")
        self.fire_button.setObjectName("fireButton")
        self.fire_button.setMinimumHeight(46)
        self.fire_button.setSizePolicy(
            QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Fixed
        )
        layout.addWidget(self.fire_button)

        secondary = QHBoxLayout()
        self.scope_button = QPushButton("Toggle optic")
        self.scope_button.setObjectName("secondaryButton")
        self.reset_button = QPushButton("Reset range")
        self.reset_button.setObjectName("secondaryButton")
        secondary.addWidget(self.scope_button)
        secondary.addWidget(self.reset_button)
        layout.addLayout(secondary)

        self.fire_button.clicked.connect(self.fire_requested)
        self.scope_button.clicked.connect(self.scope_requested)
        self.reset_button.clicked.connect(self.reset_requested)
        return actions

    def _add_field(
        self,
        form: QFormLayout,
        label: str,
        name: str,
        minimum: float,
        maximum: float,
        value: float,
        decimals: int,
        suffix: str,
    ) -> None:
        field = QDoubleSpinBox()
        field.setRange(minimum, maximum)
        field.setDecimals(decimals)
        field.setValue(value)
        field.setKeyboardTracking(False)
        if suffix:
            field.setSuffix(f" {suffix}")
        form.addRow(label, field)
        self._fields[name] = field

    def _set_advanced_visible(self, visible: bool) -> None:
        self.advanced_panel.setVisible(visible)
        self.advanced_toggle.setArrowType(
            Qt.ArrowType.DownArrow if visible else Qt.ArrowType.RightArrow
        )

    def scenario(self) -> ScenarioConfig:
        values = {name: field.value() for name, field in self._fields.items()}
        config = ScenarioConfig(integrator=self.integrator.currentText(), **values)
        config.validate()
        return config

    def set_aim(self, elevation_deg: float, azimuth_deg: float) -> None:
        self._fields["elevation_deg"].setValue(elevation_deg)
        self._fields["azimuth_deg"].setValue(azimuth_deg)

    def set_busy(self, busy: bool) -> None:
        self.fire_button.setDisabled(busy)
        self.fire_button.setText("SIMULATING…" if busy else "FIRE SHOT")
