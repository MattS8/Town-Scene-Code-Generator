"""
Settings widget with collapsible sections for code generation options.
"""

from typing import Optional
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QScrollArea, QFormLayout,
    QSpinBox, QGroupBox, QPushButton, QLabel, QHBoxLayout, QLineEdit,
    QComboBox, QCheckBox, QStyle
)
from PyQt6.QtCore import Qt, QSize
from PyQt6.QtGui import QFont, QIcon
from pathlib import Path

from src.core.mp3_file_manager import MP3FileManager


class CollapsibleSection(QGroupBox):
    """A collapsible section widget with toggle button."""
    
    def __init__(self, title: str, parent=None):
        super().__init__(parent)
        self._expanded = True
        self._content_widget = QWidget()
        self._content_layout = QVBoxLayout()
        self._content_widget.setLayout(self._content_layout)
        
        # Load icons
        images_dir = Path(__file__).parent / "images"
        self._expand_icon = QIcon(str(images_dir / "expand_nobg.png"))
        self._collapse_icon = QIcon(str(images_dir / "collapse_nobg.png"))
        
        # Create header with toggle button and title
        header_layout = QHBoxLayout()
        header_layout.setContentsMargins(0, 0, 0, 0)
        header_layout.setSpacing(8)
        
        self._toggle_button = QPushButton()
        self._toggle_button.setFixedSize(12, 12)
        self._toggle_button.setIconSize(QSize(12, 12))
        self._toggle_button.setFlat(True)
        self._toggle_button.setStyleSheet("""
            QPushButton {
                border: none;
                background-color: transparent;
            }
            QPushButton:hover {
                background-color: rgba(0, 0, 0, 0.08);
            }
            QPushButton:pressed {
                background-color: rgba(0, 0, 0, 0.12);
            }
        """)
        self._toggle_button.clicked.connect(self._toggle)
        self._update_button_icon()
        
        self._title_label = QLabel(title)
        font = QFont()
        font.setBold(True)
        self._title_label.setFont(font)
        
        header_layout.addWidget(self._toggle_button)
        header_layout.addWidget(self._title_label)
        header_layout.addStretch()
        
        # Main layout - hide QGroupBox title, use our custom header
        self.setTitle("")  # Hide default title
        main_layout = QVBoxLayout()
        main_layout.setContentsMargins(10, 5, 10, 5)
        main_layout.addLayout(header_layout)
        main_layout.addWidget(self._content_widget)
        self.setLayout(main_layout)
    
    def _toggle(self):
        """Toggle expanded state."""
        self._expanded = not self._expanded
        self._content_widget.setVisible(self._expanded)
        self._update_button_icon()
    
    def _update_button_icon(self):
        """Update the button icon based on expanded state."""
        if self._expanded:
            # Show collapse icon (down arrow) when expanded
            self._toggle_button.setIcon(self._collapse_icon)
        else:
            # Show expand icon (right arrow) when collapsed
            self._toggle_button.setIcon(self._expand_icon)
    
    def add_layout(self, layout):
        """Add a layout to the section content."""
        self._content_layout.addLayout(layout)
    
    def add_widget(self, widget):
        """Add a widget to the section content."""
        self._content_layout.addWidget(widget)
    
    def set_expanded(self, expanded: bool):
        """Set the expanded state."""
        if self._expanded != expanded:
            self._toggle()


class SettingsWidget(QWidget):
    """Settings widget with collapsible sections."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.file_manager = MP3FileManager()
        self.init_ui()
        self.refresh_drives()
    
    def init_ui(self):
        """Initialize the UI."""
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        
        # Title
        title = QLabel("Settings")
        title.setStyleSheet("font-size: 14px; font-weight: bold;")
        layout.addWidget(title)
        
        # Create scroll area for settings
        scroll_area = QScrollArea()
        scroll_area.setWidgetResizable(True)
        scroll_area.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        
        # Content widget for scroll area
        content_widget = QWidget()
        content_layout = QVBoxLayout()
        content_widget.setLayout(content_layout)
        
        # MP3 Pin Options section
        self.mp3_pin_section = CollapsibleSection("MP3 Pins")
        self.mp3_pin_section.set_expanded(True)  # Expanded by default
        mp3_pin_layout = QFormLayout()
        mp3_pin_layout.setLabelAlignment(Qt.AlignmentFlag.AlignRight)
        
        self.play_pause_pin_edit = QLineEdit()
        self.play_pause_pin_edit.setPlaceholderText("e.g., A3 or D12")
        self.play_pause_pin_edit.setFixedWidth(100)
        mp3_pin_layout.addRow("Play/Pause Pin:", self.play_pause_pin_edit)
        
        self.skip_pin_edit = QLineEdit()
        self.skip_pin_edit.setPlaceholderText("e.g., A3 or D12")
        self.skip_pin_edit.setFixedWidth(100)
        mp3_pin_layout.addRow("Skip Pin:", self.skip_pin_edit)
        
        self.mp3_pin_section.add_layout(mp3_pin_layout)
        content_layout.addWidget(self.mp3_pin_section)
        
        # Train Control section
        self.train_section = CollapsibleSection("Train Control")
        self.train_section.set_expanded(False)  # Collapsed by default
        train_layout = QFormLayout()
        train_layout.setLabelAlignment(Qt.AlignmentFlag.AlignRight)
        
        self.train_pin_left_right_spin = QLineEdit()
        self.train_pin_left_right_spin.setPlaceholderText("None")
        self.train_pin_left_right_spin.setFixedWidth(100)
        train_layout.addRow("Train Pin (Left to Right):", self.train_pin_left_right_spin)
        
        self.train_pin_right_left_spin = QLineEdit()
        self.train_pin_right_left_spin.setPlaceholderText("None")
        self.train_pin_right_left_spin.setFixedWidth(100)
        train_layout.addRow("Train Pin (Right to Left):", self.train_pin_right_left_spin)
        
        self.motor_voltage_pin_spin = QLineEdit()
        self.motor_voltage_pin_spin.setPlaceholderText("None")
        self.motor_voltage_pin_spin.setFixedWidth(100)
        train_layout.addRow("Motor Voltage Pin:", self.motor_voltage_pin_spin)
        
        self.train_init_duration_spin = QSpinBox()
        self.train_init_duration_spin.setRange(0, 100000)
        self.train_init_duration_spin.setValue(0)
        self.train_init_duration_spin.setSuffix(" ms")
        train_layout.addRow("Initialization Duration:", self.train_init_duration_spin)
        
        self.train_section.add_layout(train_layout)
        content_layout.addWidget(self.train_section)
        
        # Routine Randomization section
        self.randomization_section = CollapsibleSection("Routine Randomization")
        self.randomization_section.set_expanded(False)  # Collapsed by default
        randomization_layout = QFormLayout()
        randomization_layout.setLabelAlignment(Qt.AlignmentFlag.AlignRight)
        
        self.randomize_routine_order_checkbox = QCheckBox("Disabled")
        self.randomize_routine_order_checkbox.stateChanged.connect(self._update_randomize_checkbox_text)
        randomization_layout.addRow("Randomize Order:", self.randomize_routine_order_checkbox)
        
        self.seed_randomizer_pin_spin = QLineEdit()
        self.seed_randomizer_pin_spin.setPlaceholderText("None")
        self.seed_randomizer_pin_spin.setFixedWidth(100)
        randomization_layout.addRow("Seed Pin:", self.seed_randomizer_pin_spin)
        
        self.randomization_section.add_layout(randomization_layout)
        content_layout.addWidget(self.randomization_section)
        
        # Debug Options section
        self.debug_section = CollapsibleSection("Debug Options")
        self.debug_section.set_expanded(False)  # Collapsed by default
        debug_layout = QVBoxLayout()
        
        self.enable_train_debug_checkbox = QCheckBox("Enable Train Debug")
        debug_layout.addWidget(self.enable_train_debug_checkbox)
        
        self.enable_lights_debug_checkbox = QCheckBox("Enable Lights Debug")
        debug_layout.addWidget(self.enable_lights_debug_checkbox)
        
        self.enable_routine_skip_debug_checkbox = QCheckBox("Enable Routine Skip Debug")
        debug_layout.addWidget(self.enable_routine_skip_debug_checkbox)
        
        self.debug_section.add_layout(debug_layout)
        content_layout.addWidget(self.debug_section)
        
        # ESP32/WiFi section
        self.esp32_section = CollapsibleSection("ESP32/WiFi")
        self.esp32_section.set_expanded(False)  # Collapsed by default
        esp32_layout = QFormLayout()
        esp32_layout.setLabelAlignment(Qt.AlignmentFlag.AlignRight)
        
        self.wifi_ssid_edit = QLineEdit()
        self.wifi_ssid_edit.setPlaceholderText("Enter WiFi SSID")
        esp32_layout.addRow("WiFi SSID:", self.wifi_ssid_edit)
        
        self.wifi_password_edit = QLineEdit()
        self.wifi_password_edit.setPlaceholderText("Enter WiFi password")
        self.wifi_password_edit.setEchoMode(QLineEdit.EchoMode.Password)
        esp32_layout.addRow("WiFi Password:", self.wifi_password_edit)
        
        self.esp32_section.add_layout(esp32_layout)
        content_layout.addWidget(self.esp32_section)
        
        # General Options section
        self.general_section = CollapsibleSection("General Options")
        self.general_section.set_expanded(False)  # Collapsed by default
        general_layout = QFormLayout()
        general_layout.setLabelAlignment(Qt.AlignmentFlag.AlignRight)
        
        self.swap_on_off_values_checkbox = QCheckBox("Disabled")
        self.swap_on_off_values_checkbox.stateChanged.connect(self._update_swap_on_off_checkbox_text)
        general_layout.addRow("Swap On/Off Values:", self.swap_on_off_values_checkbox)
        
        self.halloween_town_scene_checkbox = QCheckBox("Disabled")
        self.halloween_town_scene_checkbox.stateChanged.connect(self._update_halloween_checkbox_text)
        general_layout.addRow("Halloween Town Scene:", self.halloween_town_scene_checkbox)
        
        self.motion_sensor_pin_spin = QLineEdit()
        self.motion_sensor_pin_spin.setPlaceholderText("None")
        self.motion_sensor_pin_spin.setFixedWidth(100)
        general_layout.addRow("Motion Sensor Pin:", self.motion_sensor_pin_spin)
        
        self.general_section.add_layout(general_layout)
        content_layout.addWidget(self.general_section)

                # MP3 Player Drive section
        self.mp3_drive_section = CollapsibleSection("MP3 Player")
        self.mp3_drive_section.set_expanded(False)
        drive_layout = QVBoxLayout()
        
        drive_select_layout = QHBoxLayout()
        drive_select_layout.addWidget(QLabel("Drive Letter:"))
        self.drive_combo = QComboBox()
        self.drive_combo.setEditable(True)
        self.drive_combo.setInsertPolicy(QComboBox.InsertPolicy.NoInsert)
        self.drive_combo.currentTextChanged.connect(self._update_drive_info)
        drive_select_layout.addWidget(self.drive_combo)
        
        self.refresh_drives_btn = QPushButton("Refresh")
        self.refresh_drives_btn.clicked.connect(self.refresh_drives)
        drive_select_layout.addWidget(self.refresh_drives_btn)
        
        drive_layout.addLayout(drive_select_layout)
        
        # Drive info
        self.drive_info_label = QLabel("No drive selected")
        drive_layout.addWidget(self.drive_info_label)
        
        # Destination folder
        folder_layout = QHBoxLayout()
        folder_layout.addWidget(QLabel("Destination Folder (optional):"))
        self.folder_edit = QComboBox()
        self.folder_edit.setEditable(True)
        self.folder_edit.addItems(["", "music", "songs", "audio"])
        folder_layout.addWidget(self.folder_edit)
        drive_layout.addLayout(folder_layout)
        
        # Overwrite checkbox
        self.overwrite_checkbox = QCheckBox("Overwrite existing files")
        self.overwrite_checkbox.setChecked(True)
        drive_layout.addWidget(self.overwrite_checkbox)
        
        self.mp3_drive_section.add_layout(drive_layout)
        content_layout.addWidget(self.mp3_drive_section)
        
        # Add stretch to push sections to top
        content_layout.addStretch()
        
        scroll_area.setWidget(content_widget)
        layout.addWidget(scroll_area)
        
        self.setLayout(layout)
    
    def refresh_drives(self):
        """Refresh available drives."""
        self.drive_combo.clear()
        drives = self.file_manager.get_available_drives()
        self.drive_combo.addItems(drives)
        
        if drives:
            self.drive_combo.setCurrentIndex(0)
            self._update_drive_info()
    
    def _update_drive_info(self):
        """Update drive information display."""
        drive_letter = self.drive_combo.currentText().strip()
        if not drive_letter:
            self.drive_info_label.setText("No drive selected")
            return
        
        if not drive_letter.endswith(':'):
            drive_letter = f"{drive_letter}:"
        
        if self.file_manager.validate_drive(drive_letter):
            space_info = self.file_manager.get_drive_space(drive_letter)
            if space_info:
                free_gb = space_info['free'] / (1024 ** 3)
                total_gb = space_info['total'] / (1024 ** 3)
                self.drive_info_label.setText(
                    f"Drive valid | Free: {free_gb:.2f} GB / Total: {total_gb:.2f} GB"
                )
            else:
                self.drive_info_label.setText("Drive valid")
        else:
            self.drive_info_label.setText("Drive invalid or not accessible")
    
    def _update_randomize_checkbox_text(self, state):
        """Update the randomize routine order checkbox text based on state."""
        if self.randomize_routine_order_checkbox.isChecked():
            self.randomize_routine_order_checkbox.setText("Enabled")
        else:
            self.randomize_routine_order_checkbox.setText("Disabled")
    
    def _update_swap_on_off_checkbox_text(self, state):
        """Update the swap on/off values checkbox text based on state."""
        if self.swap_on_off_values_checkbox.isChecked():
            self.swap_on_off_values_checkbox.setText("Enabled")
        else:
            self.swap_on_off_values_checkbox.setText("Disabled")
    
    def _update_halloween_checkbox_text(self, state):
        """Update the halloween town scene checkbox text based on state."""
        if self.halloween_town_scene_checkbox.isChecked():
            self.halloween_town_scene_checkbox.setText("Enabled")
        else:
            self.halloween_town_scene_checkbox.setText("Disabled")

