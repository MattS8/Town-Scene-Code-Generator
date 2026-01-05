"""
MP3 Transfer UI component for copying files to MP3 player.
"""

from pathlib import Path
from typing import List, Dict, Optional
from PyQt6.QtWidgets import (
    QWidget, QVBoxLayout, QHBoxLayout, QLabel, QComboBox, QPushButton,
    QProgressBar, QCheckBox, QGroupBox, QLineEdit, QMessageBox, QFormLayout
)
from PyQt6.QtCore import Qt, QThread, pyqtSignal

from src.core.mp3_file_manager import MP3FileManager
from src.core.routine_manager import RoutineManager


class TransferThread(QThread):
    """Thread for file transfer operations."""
    
    progress = pyqtSignal(int, int, str)  # current, total, filename
    finished = pyqtSignal(dict)  # results dict
    
    def __init__(self, file_manager: MP3FileManager, files: List[Path],
                 drive_letter: str, destination_folder: str, overwrite: bool):
        super().__init__()
        self.file_manager = file_manager
        self.files = files
        self.drive_letter = drive_letter
        self.destination_folder = destination_folder
        self.overwrite = overwrite
    
    def run(self):
        """Run the transfer operation."""
        results = self.file_manager.copy_files_to_mp3(
            self.files,
            self.drive_letter,
            self.destination_folder,
            self.overwrite,
            self._progress_callback
        )
        self.finished.emit(results)
    
    def _progress_callback(self, current: int, total: int, filename: str):
        """Progress callback."""
        self.progress.emit(current, total, filename)


class MP3TransferWidget(QWidget):
    """Widget for MP3 player file transfer."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.file_manager = MP3FileManager()
        self.routine_manager: Optional[RoutineManager] = None
        self.transfer_thread: Optional[TransferThread] = None
        self.init_ui()
        self.refresh_drives()
    
    def init_ui(self):
        """Initialize the UI."""
        layout = QVBoxLayout()
        
        # Title
        title = QLabel("MP3 Player Settings")
        title.setStyleSheet("font-size: 14px; font-weight: bold;")
        layout.addWidget(title)
        
        # Options
        options_group = QGroupBox("Pin Options")
        options_layout = QVBoxLayout()
        
        # Pin configuration form
        pin_form = QFormLayout()
        self.play_pause_pin_edit = QLineEdit()
        self.play_pause_pin_edit.setPlaceholderText("e.g., A3 or D12")
        self.play_pause_pin_edit.setMaxLength(4)
        self.play_pause_pin_edit.setFixedWidth(100)
        pin_form.addRow("Play/Pause Pin:", self.play_pause_pin_edit)
        
        self.skip_pin_edit = QLineEdit()
        self.skip_pin_edit.setPlaceholderText("e.g., A3 or D12")
        self.skip_pin_edit.setMaxLength(4)
        self.skip_pin_edit.setFixedWidth(100)
        # alight the row label to the right
        pin_form.setLabelAlignment(Qt.AlignmentFlag.AlignRight)
        pin_form.addRow("Skip Pin:", self.skip_pin_edit)
        
        options_layout.addLayout(pin_form)
        options_group.setLayout(options_layout)
        layout.addWidget(options_group)
        
        # Drive selection
        drive_group = QGroupBox("MP3 Player Drive")
        drive_layout = QVBoxLayout()
        
        drive_select_layout = QHBoxLayout()
        drive_select_layout.addWidget(QLabel("Drive Letter:"))
        self.drive_combo = QComboBox()
        self.drive_combo.setEditable(True)
        self.drive_combo.setInsertPolicy(QComboBox.InsertPolicy.NoInsert)
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
        
        drive_group.setLayout(drive_layout)
        layout.addWidget(drive_group)
        
        # Progress
        self.progress_bar = QProgressBar()
        self.progress_bar.setVisible(False)
        layout.addWidget(self.progress_bar)
        
        self.status_label = QLabel("")
        layout.addWidget(self.status_label)
        
        # Transfer button
        self.transfer_button = QPushButton("Transfer Files")
        self.transfer_button.clicked.connect(self._on_transfer)
        layout.addWidget(self.transfer_button)
        
        layout.addStretch()
        self.setLayout(layout)
    
    def set_routine_manager(self, routine_manager: RoutineManager):
        """Set the routine manager."""
        self.routine_manager = routine_manager
    
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
    
    def _on_transfer(self):
        """Handle transfer button click."""
        # Get drive letter
        drive_letter = self.drive_combo.currentText().strip()
        if not drive_letter:
            QMessageBox.warning(self, "Error", "Please select a drive letter")
            return
        
        if not drive_letter.endswith(':'):
            drive_letter = f"{drive_letter}:"
        
        if not self.file_manager.validate_drive(drive_letter):
            QMessageBox.warning(self, "Error", "Invalid or inaccessible drive")
            return
        
        # Get all .wav files from routines
        files_to_transfer = []
        if self.routine_manager:
            routines = self.routine_manager.get_all_routines()
            for routine in routines:
                wav_file = routine.get('wav_file', '')
                if wav_file:
                    file_path = Path(wav_file)
                    if file_path.exists() and file_path.suffix.lower() == '.wav':
                        files_to_transfer.append(file_path)
        
        if not files_to_transfer:
            QMessageBox.warning(self, "Error", "No .wav files found in routines")
            return
        
        # Get destination folder
        destination_folder = self.folder_edit.currentText().strip()
        overwrite = self.overwrite_checkbox.isChecked()
        
        # Start transfer
        self.transfer_button.setEnabled(False)
        self.progress_bar.setVisible(True)
        self.progress_bar.setMaximum(len(files_to_transfer))
        self.progress_bar.setValue(0)
        self.status_label.setText("Starting transfer...")
        
        self.transfer_thread = TransferThread(
            self.file_manager,
            files_to_transfer,
            drive_letter,
            destination_folder,
            overwrite
        )
        self.transfer_thread.progress.connect(self._on_transfer_progress)
        self.transfer_thread.finished.connect(self._on_transfer_finished)
        self.transfer_thread.start()
    
    def _on_transfer_progress(self, current: int, total: int, filename: str):
        """Handle transfer progress."""
        self.progress_bar.setValue(current)
        self.status_label.setText(f"Transferring: {filename} ({current}/{total})")
    
    def _on_transfer_finished(self, results: Dict):
        """Handle transfer completion."""
        self.transfer_button.setEnabled(True)
        self.progress_bar.setVisible(False)
        
        # Count successes and failures
        success_count = sum(1 for success in results.values() if success)
        fail_count = len(results) - success_count
        
        if fail_count == 0:
            QMessageBox.information(
                self, "Success",
                f"Successfully transferred {success_count} file(s) to MP3 player"
            )
            self.status_label.setText(f"Transfer complete: {success_count} file(s)")
        else:
            QMessageBox.warning(
                self, "Transfer Complete",
                f"Transferred {success_count} file(s), {fail_count} failed"
            )
            self.status_label.setText(f"Transfer complete: {success_count} success, {fail_count} failed")

