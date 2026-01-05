"""
File Manager GUI component with drag-and-drop support for WAV files.
"""

from pathlib import Path
from typing import List, Dict, Optional, Callable
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QListWidget, QListWidgetItem,
    QPushButton, QMessageBox, QFileDialog
)
from PyQt6.QtCore import Qt, pyqtSignal
from PyQt6.QtGui import QDragEnterEvent, QDropEvent

from src.core.wav_parser import parse_wav_file


class FileManagerWidget(QWidget):
    """Widget for managing WAV files with drag-and-drop."""
    
    file_added = pyqtSignal(dict)  # Emits file data dict
    file_removed = pyqtSignal(str)  # Emits file path
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.files: Dict[str, Dict] = {}  # filepath -> file_data
        self.init_ui()
    
    def init_ui(self):
        """Initialize the UI."""
        layout = QVBoxLayout()
        
        # Title
        title = QLabel("WAV Files")
        title.setStyleSheet("font-size: 14px; font-weight: bold;")
        layout.addWidget(title)
        
        # File list
        self.file_list = QListWidget()
        self.file_list.setAcceptDrops(True)
        self.file_list.itemDoubleClicked.connect(self._on_item_double_clicked)
        layout.addWidget(self.file_list)
        
        # Buttons
        button_layout = QHBoxLayout()
        
        self.add_button = QPushButton("Add Files...")
        self.add_button.clicked.connect(self._on_add_files)
        button_layout.addWidget(self.add_button)
        
        self.remove_button = QPushButton("Remove Selected")
        self.remove_button.clicked.connect(self._on_remove_selected)
        button_layout.addWidget(self.remove_button)
        
        layout.addLayout(button_layout)
        
        self.setLayout(layout)
        
        # Enable drag and drop
        self.setAcceptDrops(True)
        self.file_list.setAcceptDrops(True)
    
    def dragEnterEvent(self, event: QDragEnterEvent):
        """Handle drag enter event."""
        if event.mimeData().hasUrls():
            event.acceptProposedAction()
    
    def dropEvent(self, event: QDropEvent):
        """Handle drop event."""
        urls = event.mimeData().urls()
        for url in urls:
            file_path = Path(url.toLocalFile())
            if file_path.suffix.lower() == '.wav':
                self.add_file(file_path)
        event.acceptProposedAction()
    
    def _on_add_files(self):
        """Handle add files button click."""
        file_paths, _ = QFileDialog.getOpenFileNames(
            self, "Select WAV Files", "", "WAV Files (*.wav);;All Files (*)"
        )
        for file_path in file_paths:
            self.add_file(Path(file_path))
    
    def _on_remove_selected(self):
        """Handle remove selected button click."""
        current_item = self.file_list.currentItem()
        if current_item:
            file_path = current_item.data(Qt.ItemDataRole.UserRole)
            self.remove_file(file_path)
    
    def _on_item_double_clicked(self, item: QListWidgetItem):
        """Handle double click on item."""
        file_path = item.data(Qt.ItemDataRole.UserRole)
        if file_path:
            self.file_added.emit(self.files[file_path])
    
    def add_file(self, file_path: Path) -> bool:
        """
        Add a WAV file to the list.
        
        Args:
            file_path: Path to WAV file
            
        Returns:
            True if successful, False otherwise
        """
        if not file_path.exists():
            QMessageBox.warning(self, "Error", f"File not found: {file_path}")
            return False
        
        file_path_str = str(file_path)
        
        # Check if already added
        if file_path_str in self.files:
            QMessageBox.information(self, "Info", "File already in list")
            return False
        
        try:
            # Parse WAV file
            print(f"Parsing WAV file: {file_path.name}")
            file_data = parse_wav_file(file_path)
            print(f"Successfully parsed {file_path.name}: {len(file_data['markers'])} markers, {len(file_data['lights'])} lights found")
            
            # Store file data
            self.files[file_path_str] = file_data
            
            # Add to list widget
            item = QListWidgetItem()
            item.setText(f"{file_path.name} ({len(file_data['markers'])} markers)")
            item.setData(Qt.ItemDataRole.UserRole, file_path_str)
            item.setToolTip(f"Duration: {file_data['duration_ms']/1000:.1f}s\n"
                          f"Markers: {len(file_data['markers'])}\n"
                          f"Sample Rate: {file_data['sample_rate']} Hz")
            self.file_list.addItem(item)

            # Emit signal
            self.file_added.emit(file_data)
            
            return True
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to parse WAV file:\n{str(e)}")
            return False
    
    def remove_file(self, file_path: str):
        """
        Remove a file from the list.
        
        Args:
            file_path: Path to file to remove
        """
        if file_path not in self.files:
            return
        
        # Remove from dict
        del self.files[file_path]
        
        # Remove from list widget
        for i in range(self.file_list.count()):
            item = self.file_list.item(i)
            if item.data(Qt.ItemDataRole.UserRole) == file_path:
                self.file_list.takeItem(i)
                break
        
        # Emit signal
        self.file_removed.emit(file_path)
    
    def get_file_data(self, file_path: str) -> Optional[Dict]:
        """Get file data by path."""
        return self.files.get(file_path)
    
    def get_all_files(self) -> List[Dict]:
        """Get all file data."""
        return list(self.files.values())
    
    def clear_files(self):
        """Clear all files."""
        self.files.clear()
        self.file_list.clear()

