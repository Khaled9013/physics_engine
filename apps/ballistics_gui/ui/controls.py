"""Scenario controls for the native application."""

from __future__ import annotations

import math

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
from ..simulation.projectiles import (
    ProjectilePreset,
    circular_reference_area_m2,
    load_projectile_presets,
)


CUSTOM_PRESET_LABEL = "Custom"


class ScenarioControls(QWidget):
    """Compact primary controls with the full solver exposed on demand."""

    fire_requested = pyqtSignal()
    scope_requested = pyqtSignal()
    reset_requested = pyqtSignal()
    preset_applied = pyqtSignal(str)

    def __init__(self, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self._fields: dict[str, QDoubleSpinBox] = {}
        self._presets: tuple[ProjectilePreset, ...] = ()
        self._preset_error: str | None = None
        self._applying_preset = False
        try:
            self._presets = load_projectile_presets()
        except RuntimeError as error:
            # A missing or malformed preset file must not stop the user from firing a
            # manually configured shot, so it degrades to custom-only with the reason shown.
            self._preset_error = str(error)
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
        self.integrator = QComboBox()
        self.integrator.addItems(["rk4.v1", "euler.v1"])
        form.addRow("Integrator", self.integrator)

        self.preset_box = QComboBox()
        self.preset_box.addItem(CUSTOM_PRESET_LABEL, None)
        for preset in self._presets:
            self.preset_box.addItem(f"{preset.category} · {preset.name}", preset.identifier)
        self.preset_box.currentIndexChanged.connect(self._preset_selected)
        form.addRow("Projectile", self.preset_box)

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
        # The optic's zero: the distance at which the bore's line crosses the line of sight,
        # so point of aim equals point of impact there. Not part of ScenarioConfig, because
        # it describes the sight rather than the shot.
        self._add_field(
            form, "Zero at", "zero_distance_m", 25.0, 1000.0, 150.0, 1, "m"
        )
        self.zero_readout = QLabel("Optic not yet zeroed.")
        self.zero_readout.setObjectName("controlHint")
        self.zero_readout.setWordWrap(True)
        form.addRow(self.zero_readout)
        self.preset_hint = QLabel(
            self._preset_error
            if self._preset_error is not None
            else "Choose a projectile to fill mass, diameter, area, drag, and muzzle speed."
        )
        self.preset_hint.setObjectName("controlHint")
        self.preset_hint.setWordWrap(True)
        form.addRow(self.preset_hint)
        return group

    def _preset_selected(self, index: int) -> None:
        identifier = self.preset_box.itemData(index)
        if identifier is None:
            self.preset_hint.setText(
                "Custom projectile — edit mass, diameter, drag, and speed by hand."
            )
            return
        preset = next(p for p in self._presets if p.identifier == identifier)
        self.apply_preset(preset)

    def apply_preset(self, preset: ProjectilePreset) -> None:
        """Populate every projectile field from one preset.

        Guarded so the writes do not read back as manual edits, which would otherwise flip
        the selector to Custom halfway through applying it.
        """

        self._applying_preset = True
        try:
            self._fields["projectile_mass_kg"].setValue(preset.mass_kg)
            self._fields["projectile_diameter_m"].setValue(preset.diameter_m)
            self._fields["reference_area_m2"].setValue(preset.reference_area_m2)
            self._fields["drag_coefficient"].setValue(preset.drag_coefficient)
            self._fields["launch_speed_mps"].setValue(preset.muzzle_speed_mps)
        finally:
            self._applying_preset = False
        self.preset_hint.setText(
            f"{preset.description} Sectional density "
            f"{preset.sectional_density_kgpm2:.0f} kg/m². Drag coefficient fitted at "
            f"{preset.reference_distance_m:.0f} m."
        )
        self.preset_applied.emit(preset.name)

    def _build_advanced_panel(self) -> QWidget:
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(10)

        numerical = QGroupBox("SOLVER")
        numerical_form = QFormLayout(numerical)
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
            5,
            "kg",
        )
        self._add_field(
            projectile_form,
            "Diameter",
            "projectile_diameter_m",
            0.0001,
            1.0,
            0.009,
            5,
            "m",
        )
        self._add_field(
            projectile_form,
            "Reference area",
            "reference_area_m2",
            1.0e-9,
            1.0,
            6.3617e-5,
            10,
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
        # Diameter previously reached the solver's validator but no force calculation, so it
        # could disagree with reference area indefinitely while only the area had any effect.
        # It now drives the area, which is the value drag actually uses.
        self._fields["projectile_diameter_m"].valueChanged.connect(
            self._diameter_changed
        )
        area_hint = QLabel(
            "Reference area follows diameter as πd²/4. Editing it directly overrides that "
            "for a non-circular cross-section; only the area reaches the drag model."
        )
        area_hint.setObjectName("controlHint")
        area_hint.setWordWrap(True)
        projectile_form.addRow(area_hint)
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
        # Stored as headwind, converted to the solver's downrange wind component in
        # `scenario`. A positive headwind blows toward the shooter, which is -x, so the two
        # differ by a sign; passing this field straight through made a headwind behave as a
        # tailwind and lengthen the shot.
        self._add_field(
            environment_form,
            "Headwind",
            "headwind_mps",
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
        field.setSingleStep(min(1.0, 10.0 ** (1 - decimals)))
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

    def _diameter_changed(self, diameter_m: float) -> None:
        area = self._fields["reference_area_m2"]
        derived = circular_reference_area_m2(diameter_m)
        if math.isclose(area.value(), derived, rel_tol=1e-9, abs_tol=1e-12):
            return
        blocked = area.blockSignals(True)
        try:
            area.setValue(derived)
        finally:
            area.blockSignals(blocked)
        if not self._applying_preset:
            self._mark_custom()

    def _mark_custom(self) -> None:
        """Drop back to Custom once any projectile value is edited by hand."""

        if self.preset_box.currentData() is None:
            return
        blocked = self.preset_box.blockSignals(True)
        try:
            self.preset_box.setCurrentIndex(0)
        finally:
            self.preset_box.blockSignals(blocked)
        self.preset_hint.setText(
            "Custom projectile — edit mass, diameter, drag, and speed by hand."
        )

    def zero_distance_m(self) -> float:
        return self._fields["zero_distance_m"].value()

    def set_zero_readout(self, text: str) -> None:
        self.zero_readout.setText(text)

    def scenario(self) -> ScenarioConfig:
        values = {name: field.value() for name, field in self._fields.items()}
        # A positive headwind opposes the shot, so it is the negative downrange component.
        values["wind_x_mps"] = -values.pop("headwind_mps")
        # Sight properties are not part of the shot the solver runs.
        values.pop("zero_distance_m", None)
        config = ScenarioConfig(integrator=self.integrator.currentText(), **values)
        config.validate()
        return config

    def set_aim(self, elevation_deg: float, azimuth_deg: float) -> None:
        self._fields["elevation_deg"].setValue(elevation_deg)
        self._fields["azimuth_deg"].setValue(azimuth_deg)

    def set_busy(self, busy: bool) -> None:
        self.fire_button.setDisabled(busy)
        self.fire_button.setText("SIMULATING…" if busy else "FIRE SHOT")
