"""
Routine Editor GUI component for configuring light routines.
"""

from pathlib import Path
from typing import Dict, List, Optional
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QComboBox, QPushButton,
    QTableWidget, QTableWidgetItem, QSpinBox, QLineEdit, QGroupBox,
    QFormLayout, QMessageBox, QHeaderView
)
from PyQt6.QtCore import Qt, pyqtSignal

from src.core.routine_manager import create_light_config, create_default_light_config


class RoutineEditorWidget(QWidget):
    """Widget for editing routine configurations."""
    
    routine_changed = pyqtSignal(dict)  # Emits updated routine dict
    file_selection_changed = pyqtSignal(str)  # Emits filepath when file is selected in dropdown
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.current_routine: Optional[Dict] = None
        self.available_files: List[Dict] = []
        self._loading_routine: bool = False  # Flag to prevent auto-save during loading
        self.init_ui()
    
    def init_ui(self):
        """Initialize the UI."""
        layout = QVBoxLayout()
        
        # Title
        title = QLabel("Routine Editor")
        title.setStyleSheet("font-size: 14px; font-weight: bold;")
        layout.addWidget(title)
        
        # Routine name
        name_layout = QHBoxLayout()
        name_layout.addWidget(QLabel("Routine Name:"))
        self.name_edit = QLineEdit()
        self.name_edit.setPlaceholderText("Enter routine name...")
        self.name_edit.textChanged.connect(self._on_name_changed)
        name_layout.addWidget(self.name_edit)
        layout.addLayout(name_layout)
        
        # WAV file selection
        file_layout = QHBoxLayout()
        file_layout.addWidget(QLabel("WAV File:"))
        self.file_combo = QComboBox()
        self.file_combo.currentIndexChanged.connect(self._on_file_changed)
        file_layout.addWidget(self.file_combo)
        layout.addLayout(file_layout)
        
        # Lights configuration
        lights_group = QGroupBox("Lights")
        lights_layout = QVBoxLayout()
        
        # Lights table
        self.lights_table = QTableWidget()
        self.lights_table.setColumnCount(3)
        self.lights_table.setHorizontalHeaderLabels(["Label", "Variable Name", "Pin"])
        self.lights_table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)
        self.lights_table.itemChanged.connect(self._on_light_changed)
        lights_layout.addWidget(self.lights_table)
        
        # Light buttons
        light_buttons = QHBoxLayout()
        self.add_light_btn = QPushButton("Add Light")
        self.add_light_btn.clicked.connect(self._on_add_light)
        self.remove_light_btn = QPushButton("Remove Light")
        self.remove_light_btn.clicked.connect(self._on_remove_light)
        light_buttons.addWidget(self.add_light_btn)
        light_buttons.addWidget(self.remove_light_btn)
        light_buttons.addStretch()
        lights_layout.addLayout(light_buttons)
        
        lights_group.setLayout(lights_layout)
        layout.addWidget(lights_group)
        
        # Marker mapping
        markers_group = QGroupBox("Marker Actions")
        markers_layout = QVBoxLayout()
        
        self.markers_table = QTableWidget()
        self.markers_table.setColumnCount(3)
        self.markers_table.setHorizontalHeaderLabels(["Marker", "Start (ms)", "End (ms)"])
        self.markers_table.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.Stretch)
        markers_layout.addWidget(self.markers_table)
        
        markers_group.setLayout(markers_layout)
        layout.addWidget(markers_group)
        
        layout.addStretch()
        self.setLayout(layout)
    
    def set_available_files(self, files: List[Dict]):
        """Set available WAV files."""
        self.available_files = files
        # Block signals to prevent triggering file selection change when repopulating
        self.file_combo.blockSignals(True)
        try:
            self.file_combo.clear()
            for file_data in files:
                self.file_combo.addItem(file_data['filename'], file_data)
        finally:
            self.file_combo.blockSignals(False)
    
    
    def load_routine(self, routine: Dict):
        """
        Load a routine for editing - simply displays the routine data.
        Does not modify or re-parse the routine.
        
        Args:
            routine: Routine dictionary to load
        """
        # Block auto-save during loading to prevent overwriting routine data
        self._loading_routine = True
        
        self.current_routine = routine
        
        # Set routine name (block signals to prevent auto-save)
        self.name_edit.blockSignals(True)
        try:
            self.name_edit.setText(routine.get('name', ''))
        finally:
            self.name_edit.blockSignals(False)
        
        # Load lights from routine's light_config
        light_config = routine.get('light_config', {})
        self._load_lights(light_config)
        
        # Set WAV file in dropdown (block signals to prevent recursion)
        wav_file = routine.get('wav_file', '')
        self.file_combo.blockSignals(True)
        try:
            for i in range(self.file_combo.count()):
                file_data = self.file_combo.itemData(i)
                if file_data and file_data.get('filepath') == wav_file:
                    self.file_combo.setCurrentIndex(i)
                    break
        finally:
            self.file_combo.blockSignals(False)
        
        # Load markers from routine
        self._load_markers(routine.get('markers', []))
        
        # Done loading, allow auto-save again
        self._loading_routine = False
    
    def _load_lights(self, light_config: Dict):
        """Load lights into table."""
        self.lights_table.setRowCount(0)
        lights = light_config.get('lights', [])
        
        for light in lights:
            row = self.lights_table.rowCount()
            self.lights_table.insertRow(row)
            
            # Label (human-readable name)
            label_item = QTableWidgetItem(light.get('label', ''))
            self.lights_table.setItem(row, 0, label_item)
            
            # Variable Name (maps to 'name' field)
            name_item = QTableWidgetItem(light.get('name', ''))
            self.lights_table.setItem(row, 1, name_item)
            
            # Pin (single value)
            pin = light.get('pin', light.get('pins', [''])[0] if light.get('pins') else '')
            pin_str = str(pin) if pin else ''
            pin_item = QTableWidgetItem(pin_str)
            self.lights_table.setItem(row, 2, pin_item)
    
    def _load_lights_from_wav(self, lights: List[Dict], clear_table: bool = False):
        """
        Load lights from WAV file data into the lights table.
        Converts lights from WAV parser format to light format.
        
        Args:
            lights: List of light dictionaries from WAV parser
            clear_table: If True, clear the table before loading. If False, only load if table is empty.
        """
        # Clear table if requested, or if table is empty and we have lights
        should_load = False
        if clear_table:
            self.lights_table.setRowCount(0)
            should_load = True
            print("Clearing table and loading lights from WAV file")
        elif self.lights_table.rowCount() == 0:
            should_load = True
            print("Loading lights from WAV file into the lights table")
        else:
            print("The table is not empty and the lights will not be loaded (use clear_table=True to overwrite)")
        
        if should_load:
            for light in lights:
                # Skip special tags (ALL_ON, ALL_OFF)
                if light.get('is_special', False):
                    continue
                
                # Label (human-readable name, use light name as default)
                light_name = light.get('name', '')
                label = light_name
                
                # Variable Name (use name as variable name, normalized to lowercase with underscores)
                variable_name = light_name.lower().replace(' ', '_')
                
                # Pin (single value from WAV parser, stored as string like "A2" or "3")
                pin_str = light.get('pin', '')
                
                # Add light using the _on_add_light method
                self._on_add_light(label, variable_name, pin_str)
            
            print(f"Loaded {self.lights_table.rowCount()} lights into the table")
    
    def _load_markers(self, markers: List[Dict]):
        """Load markers into table."""
        self.markers_table.setRowCount(0)
        
        for marker in markers:
            row = self.markers_table.rowCount()
            self.markers_table.insertRow(row)
            
            # Marker label
            label_item = QTableWidgetItem(marker.get('label', ''))
            label_item.setFlags(label_item.flags() & ~Qt.ItemFlag.ItemIsEditable)
            self.markers_table.setItem(row, 0, label_item)
            
            # Start time
            start_item = QTableWidgetItem(str(marker.get('start_ms', marker.get('position_ms', 0))))
            start_item.setFlags(start_item.flags() & ~Qt.ItemFlag.ItemIsEditable)
            self.markers_table.setItem(row, 1, start_item)
            
            # End time
            end_item = QTableWidgetItem(str(marker.get('end_ms', 0)))
            end_item.setFlags(end_item.flags() & ~Qt.ItemFlag.ItemIsEditable)
            self.markers_table.setItem(row, 2, end_item)
    
    def _on_name_changed(self, text: str):
        """Handle name change."""
        if self.current_routine:
            self.current_routine['name'] = text
            self._auto_save_routine()
    
    def _on_file_changed(self, index: int):
        """Handle file selection change in dropdown - emit signal to load routine for selected file."""
        if index >= 0:
            file_data = self.file_combo.itemData(index)
            if file_data:
                filepath = file_data.get('filepath', '')
                # Emit signal so main window can load the routine for this file
                self.file_selection_changed.emit(filepath)
    
    def _on_add_light(self, label: str = "New Light", variable_name: str = "", pin: str = ""):
        """
        Add a new light row.
        
        Args:
            label: Human-readable label for the light (default: "New Light")
            variable_name: Variable name for the light (default: auto-generated)
            pin: Pin number/identifier (default: empty string)
        """
        row = self.lights_table.rowCount()
        self.lights_table.insertRow(row)
        
        # Generate variable name if not provided
        if not variable_name:
            variable_name = f"light_{row}"
        
        self.lights_table.setItem(row, 0, QTableWidgetItem(label))  # Label
        self.lights_table.setItem(row, 1, QTableWidgetItem(variable_name))  # Variable Name
        self.lights_table.setItem(row, 2, QTableWidgetItem(pin))  # Pin
        
        # Auto-save after adding light
        self._auto_save_routine()
    
    def _on_remove_light(self):
        """Remove selected light."""
        current_row = self.lights_table.currentRow()
        if current_row >= 0:
            self.lights_table.removeRow(current_row)
            # Auto-save after removing light
            self._auto_save_routine()
    
    def _on_light_changed(self, item: QTableWidgetItem):
        """Handle light data change."""
        # Auto-save when light data is edited
        self._auto_save_routine()
    
    def _auto_save_routine(self):
        """Automatically save the current routine when changes are made."""
        if not self.current_routine:
            return
        
        # Don't auto-save while loading a routine (would overwrite with empty data)
        if self._loading_routine:
            return
        
        # Get name (don't validate - allow empty names during editing)
        name = self.name_edit.text().strip()
        
        # Collect lights
        lights = []
        for row in range(self.lights_table.rowCount()):
            label_item = self.lights_table.item(row, 0)
            name_item = self.lights_table.item(row, 1)
            pin_item = self.lights_table.item(row, 2)
            
            if name_item:
                label = label_item.text().strip() if label_item else ''
                variable_name = name_item.text().strip()
                pin_str = pin_item.text().strip() if pin_item else ''
                
                if variable_name:
                    lights.append(create_light_config(label, variable_name, pin_str))
        
        # Create light config
        light_config = create_default_light_config()
        light_config['lights'] = lights
        
        # Update routine
        self.current_routine['name'] = name
        self.current_routine['light_config'] = light_config
        
        # Emit signal to notify main window of changes
        self.routine_changed.emit(self.current_routine)

