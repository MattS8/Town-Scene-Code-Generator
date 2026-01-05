"""
Settings dialog with collapsible sections for code generation options.
"""

from typing import Optional
from PyQt6.QtWidgets import (
    QDialog, QVBoxLayout, QScrollArea, QWidget, QFormLayout,
    QSpinBox, QDialogButtonBox, QGroupBox, QPushButton, QLabel, QHBoxLayout
)
from PyQt6.QtCore import Qt
from PyQt6.QtGui import QFont


class CollapsibleSection(QGroupBox):
    """A collapsible section widget with toggle button."""
    
    def __init__(self, title: str, parent=None):
        super().__init__(parent)
        self._expanded = True
        self._content_widget = QWidget()
        self._content_layout = QVBoxLayout()
        self._content_widget.setLayout(self._content_layout)
        
        # Create header with toggle button and title
        header_layout = QHBoxLayout()
        header_layout.setContentsMargins(0, 0, 0, 0)
        
        self._toggle_button = QPushButton("▼")
        self._toggle_button.setFixedWidth(20)
        self._toggle_button.setFlat(True)
        self._toggle_button.clicked.connect(self._toggle)
        
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
        self._toggle_button.setText("▼" if self._expanded else "▶")
    
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


class SettingsDialog(QDialog):
    """Settings dialog with collapsible sections."""
    
    def __init__(self, parent=None, mp3_rx_pin: int = 16, mp3_tx_pin: int = 17):
        super().__init__(parent)
        self.mp3_rx_pin = mp3_rx_pin
        self.mp3_tx_pin = mp3_tx_pin
        self.init_ui()
    
    def init_ui(self):
        """Initialize the UI."""
        self.setWindowTitle("Settings")
        self.setMinimumWidth(500)
        self.setMinimumHeight(400)
        
        # Main layout
        main_layout = QVBoxLayout()
        
        # Create scroll area for settings
        scroll_area = QScrollArea()
        scroll_area.setWidgetResizable(True)
        scroll_area.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAlwaysOff)
        
        # Content widget for scroll area
        content_widget = QWidget()
        content_layout = QVBoxLayout()
        content_widget.setLayout(content_layout)
        
        # MP3 Player Configuration section
        self.mp3_section = CollapsibleSection("MP3 Player Configuration")
        self.mp3_section.set_expanded(True)  # Expanded by default
        mp3_layout = QFormLayout()
        
        self.rx_spin = QSpinBox()
        self.rx_spin.setRange(0, 39)
        self.rx_spin.setValue(self.mp3_rx_pin)
        mp3_layout.addRow("RX Pin:", self.rx_spin)
        
        self.tx_spin = QSpinBox()
        self.tx_spin.setRange(0, 39)
        self.tx_spin.setValue(self.mp3_tx_pin)
        mp3_layout.addRow("TX Pin:", self.tx_spin)
        
        self.mp3_section.add_layout(mp3_layout)
        content_layout.addWidget(self.mp3_section)
        
        # Add stretch to push sections to top
        content_layout.addStretch()
        
        scroll_area.setWidget(content_widget)
        main_layout.addWidget(scroll_area)
        
        # Dialog buttons
        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        main_layout.addWidget(buttons)
        
        self.setLayout(main_layout)
    
    def get_mp3_pins(self):
        """Get MP3 pin configuration."""
        return self.rx_spin.value(), self.tx_spin.value()

