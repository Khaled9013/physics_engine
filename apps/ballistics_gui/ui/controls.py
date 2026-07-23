"""Scenario controls for the native application."""

from __future__ import annotations

from PyQt6.QtCore import pyqtSignal
from PyQt6.QtWidgets import (
    QComboBox,
    QDoubleSpinBox,
    QFormLayout,
    QGroupBox,
    QHBoxLayout,
    QPushButton,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)

from ..simulation.models import ScenarioConfig


class ScenarioControls(QWidget):
    fire_requested = pyqtSignal()
    scope_requested = pyqtSignal()
    reset_requested = pyqtSignal()

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._fields: dict[str, QDoubleSpinBox] = {}
        outer = QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        panel = QWidget()
        panel_layout = QVBoxLayout(panel)

        numerical = QGroupBox("Numerics")
        numerical_form = QFormLayout(numerical)
        self.integrator = QComboBox()
        self.integrator.addItems(["rk4.v1", "euler.v1"])
        numerical_form.addRow("Integrator", self.integrator)
        self._add_field(numerical_form, "Time step (s)", "time_step_s", 0.0001, 0.05, 0.002, 4)
        self._add_field(numerical_form, "Max time (s)", "maximum_time_s", 0.05, 30.0, 5.0, 2)
        self._add_field(numerical_form, "Max distance (m)", "maximum_distance_m", 10.0, 20000.0, 5000.0, 1)
        panel_layout.addWidget(numerical)

        launch = QGroupBox("Fictional launch")
        launch_form = QFormLayout(launch)
        self._add_field(launch_form, "Speed (m/s)", "launch_speed_mps", 0.0, 1500.0, 310.0, 1)
        self._add_field(launch_form, "Elevation (deg)", "elevation_deg", -10.0, 85.0, 0.2, 3)
        self._add_field(launch_form, "Azimuth (deg)", "azimuth_deg", -90.0, 90.0, 0.0, 3)
        self._add_field(launch_form, "Height (m)", "initial_height_m", 0.01, 1000.0, 1.5, 2)
        self._add_field(launch_form, "Target distance (m)", "target_distance_m", 25.0, 1000.0, 150.0, 1)
        panel_layout.addWidget(launch)

        projectile = QGroupBox("Synthetic projectile")
        projectile_form = QFormLayout(projectile)
        self._add_field(projectile_form, "Mass (kg)", "projectile_mass_kg", 0.0001, 100.0, 0.018, 4)
        self._add_field(projectile_form, "Diameter (m)", "projectile_diameter_m", 0.0, 1.0, 0.009, 4)
        self._add_field(projectile_form, "Reference area (m²)", "reference_area_m2", 1.0e-9, 1.0, 6.3617e-5, 8)
        self._add_field(projectile_form, "Drag coefficient", "drag_coefficient", 0.0, 5.0, 0.29, 3)
        panel_layout.addWidget(projectile)

        environment = QGroupBox("Environment")
        environment_form = QFormLayout(environment)
        self._add_field(environment_form, "Air density (kg/m³)", "air_density_kgpm3", 0.0, 10.0, 1.225, 3)
        self._add_field(environment_form, "Wind X (m/s)", "wind_x_mps", -200.0, 200.0, 0.0, 2)
        self._add_field(environment_form, "Wind Y (m/s)", "wind_y_mps", -200.0, 200.0, 2.0, 2)
        self._add_field(environment_form, "Wind Z (m/s)", "wind_z_mps", -200.0, 200.0, 0.0, 2)
        self._add_field(environment_form, "Gravity (m/s²)", "gravity_mps2", 0.0, 50.0, 9.80665, 5)
        panel_layout.addWidget(environment)
        panel_layout.addStretch(1)
        scroll.setWidget(panel)
        outer.addWidget(scroll, 1)

        buttons = QHBoxLayout()
        self.fire_button = QPushButton("Fire")
        self.scope_button = QPushButton("Scope")
        self.reset_button = QPushButton("Reset")
        buttons.addWidget(self.fire_button)
        buttons.addWidget(self.scope_button)
        buttons.addWidget(self.reset_button)
        outer.addLayout(buttons)
        self.fire_button.clicked.connect(self.fire_requested)
        self.scope_button.clicked.connect(self.scope_requested)
        self.reset_button.clicked.connect(self.reset_requested)

    def _add_field(
        self,
        form: QFormLayout,
        label: str,
        name: str,
        minimum: float,
        maximum: float,
        value: float,
        decimals: int,
    ) -> None:
        field = QDoubleSpinBox()
        field.setRange(minimum, maximum)
        field.setDecimals(decimals)
        field.setValue(value)
        field.setKeyboardTracking(False)
        form.addRow(label, field)
        self._fields[name] = field

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
        self.fire_button.setText("Simulating…" if busy else "Fire")
