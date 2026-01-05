"""
Main application window.
"""

from pathlib import Path
from typing import Optional
from PyQt6.QtWidgets import (
    QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QSplitter,
    QMenuBar, QStatusBar, QFileDialog, QMessageBox, QDockWidget
)
from PyQt6.QtCore import Qt

from src.gui.file_manager import FileManagerWidget
from src.gui.routine_editor import RoutineEditorWidget
from src.gui.settings_widget import SettingsWidget
from src.gui.console_widget import ConsoleWidget
from src.core.routine_manager import RoutineManager, create_default_light_config
from src.core.code_generator import CodeGenerator


class MainWindow(QMainWindow):
    """Main application window."""
    
    def __init__(self):
        super().__init__()
        self.routine_manager = RoutineManager()
        self.code_generator = CodeGenerator()
        self.current_routine_id: Optional[int] = None
        self.mp3_rx_pin = 16
        self.mp3_tx_pin = 17
        self.init_ui()
    
    def init_ui(self):
        """Initialize the UI."""
        self.setWindowTitle("Town Scene Code Generator")
        self.setGeometry(100, 100, 1200, 800)
        
        # Create central widget
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        
        # Main layout
        main_layout = QHBoxLayout()
        central_widget.setLayout(main_layout)
        
        # Create splitter for resizable panels
        splitter = QSplitter(Qt.Orientation.Horizontal)
        
        # Left panel: File Manager
        self.file_manager = FileManagerWidget()
        self.file_manager.file_added.connect(self._on_file_added)
        self.file_manager.file_removed.connect(self._on_file_removed)
        self.file_manager.file_list.itemSelectionChanged.connect(self._on_file_selection_changed)
        splitter.addWidget(self.file_manager)
        
        # Center panel: Routine Editor
        self.routine_editor = RoutineEditorWidget()
        self.routine_editor.routine_changed.connect(self._on_routine_changed)
        self.routine_editor.file_selection_changed.connect(self._on_routine_editor_file_selected)
        splitter.addWidget(self.routine_editor)
        
        # Right panel: Settings
        self.settings_widget = SettingsWidget()
        splitter.addWidget(self.settings_widget)
        
        # Set splitter sizes (slightly narrower file manager, wider settings)
        splitter.setSizes([250, 600, 350])
        main_layout.addWidget(splitter)
        
        # Create menu bar
        self.create_menu_bar()
        
        # Create status bar
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)
        self.status_bar.showMessage("Ready")
        
        # Create console dock widget
        self.create_console_dock()
    
    def create_menu_bar(self):
        """Create the menu bar."""
        menubar = self.menuBar()
        
        # File menu
        file_menu = menubar.addMenu("File")
        
        new_project_action = file_menu.addAction("New Project")
        new_project_action.triggered.connect(self._on_new_project)
        
        open_project_action = file_menu.addAction("Open Project...")
        open_project_action.triggered.connect(self._on_open_project)
        
        save_project_action = file_menu.addAction("Save Project...")
        save_project_action.triggered.connect(self._on_save_project)
        
        file_menu.addSeparator()
        
        export_code_action = file_menu.addAction("Export Arduino Code...")
        export_code_action.triggered.connect(self._on_export_code)
        
        file_menu.addSeparator()
        
        exit_action = file_menu.addAction("Exit")
        exit_action.triggered.connect(self.close)
        
        # Routine menu (removed - routines are now automatically tied to WAV files)
        
        # Settings menu (removed - settings are now in the right panel)
        
        # View menu
        view_menu = menubar.addMenu("View")
        
        self.console_toggle_action = view_menu.addAction("Show Console")
        self.console_toggle_action.setCheckable(True)
        self.console_toggle_action.setChecked(True)
        self.console_toggle_action.triggered.connect(self._on_toggle_console)
    
    def create_console_dock(self):
        """Create the console dock widget."""
        self.console_widget = ConsoleWidget()
        
        self.console_dock = QDockWidget("Console", self)
        self.console_dock.setWidget(self.console_widget)
        self.console_dock.setAllowedAreas(Qt.DockWidgetArea.BottomDockWidgetArea | Qt.DockWidgetArea.TopDockWidgetArea)
        self.addDockWidget(Qt.DockWidgetArea.BottomDockWidgetArea, self.console_dock)
        
        # Connect visibility changes to update menu action
        self.console_dock.visibilityChanged.connect(self._on_console_visibility_changed)
        
        # Connect floating state changes to adjust widget constraints
        self.console_dock.topLevelChanged.connect(self._on_console_floating_changed)
        
        # Stream redirection is automatically installed when ConsoleWidget is initialized
    
    def showEvent(self, event):
        """Handle window show event to set initial dock sizes."""
        super().showEvent(event)
        # Set initial height of the console dock (smaller default)
        self.resizeDocks([self.console_dock], [150], Qt.Orientation.Vertical)
    
    def _on_file_added(self, file_data: dict):
        """Handle file added event - automatically create routine for the file."""
        filepath = file_data.get('filepath', '')
        
        # Check if routine already exists for this file
        existing_routine = self.routine_manager.get_routine_by_filepath(filepath)
        if existing_routine:
            # Routine already exists, just update it
            self.current_routine_id = existing_routine['id']
            # Update routine editor with available files
            files = self.file_manager.get_all_files()
            self.routine_editor.set_available_files(files)
            self.routine_editor.load_routine(existing_routine)
            self.status_bar.showMessage(f"Routine already exists for {file_data.get('filename', 'file')}")
            return
        
        # Extract filename without .wav extension for routine name
        filename = file_data.get('filename', 'New Routine')
        if filename.endswith('.wav'):
            routine_name = filename[:-4]
        else:
            routine_name = filename
        
        # Convert WAV parser lights to light_config format
        from src.core.routine_manager import create_light_config
        lights = []
        for wav_light in file_data.get('lights', []):
            # Skip special tags (ALL_ON, ALL_OFF)
            if wav_light.get('is_special', False):
                continue
            
            light_name = wav_light.get('name', '')
            label = light_name  # Use light name as label
            variable_name = light_name.lower().replace(' ', '_')
            pin_str = wav_light.get('pin', '')
            
            lights.append(create_light_config(label, variable_name, pin_str))
        
        # Create light config with lights from WAV file
        light_config = create_default_light_config()
        light_config['lights'] = lights
        
        # Create new routine automatically tied to this WAV file
        routine = self.routine_manager.create_routine(
            name=routine_name,
            wav_file=filepath,
            markers=file_data.get('markers', []),
            light_config=light_config
        )
        
        self.current_routine_id = routine['id']
        
        # Update routine editor with available files
        files = self.file_manager.get_all_files()
        self.routine_editor.set_available_files(files)
        
        # Load the routine in the editor (displays all routine data)
        self.routine_editor.load_routine(routine)
        
        self.status_bar.showMessage(f"Created routine for {filename}")
    
    def _on_file_removed(self, filepath: str):
        """Handle file removed event - automatically delete corresponding routine."""
        # Find and delete the routine for this file
        routine = self.routine_manager.get_routine_by_filepath(filepath)
        if routine:
            self.routine_manager.delete_routine_by_filepath(filepath)
            if self.current_routine_id == routine['id']:
                # Clear the editor if we deleted the current routine
                self.current_routine_id = None
                self.routine_editor.load_routine({
                    'name': '',
                    'wav_file': '',
                    'markers': [],
                    'light_config': create_default_light_config()
                })
        
        # Update routine editor with available files
        files = self.file_manager.get_all_files()
        self.routine_editor.set_available_files(files)
        
        self.status_bar.showMessage(f"Removed file and its routine")
    
    def _on_file_selection_changed(self):
        """Handle file selection change in left panel - automatically load corresponding routine."""
        current_item = self.file_manager.file_list.currentItem()
        if current_item:
            filepath = current_item.data(Qt.ItemDataRole.UserRole)
            if filepath:
                self._load_routine_for_file(filepath)
    
    def _on_routine_editor_file_selected(self, filepath: str):
        """Handle file selection change in routine editor dropdown - load routine for selected file."""
        self._load_routine_for_file(filepath)
    
    def _load_routine_for_file(self, filepath: str):
        """Load routine for a given filepath - used by both file manager and dropdown selection."""
        # Update routine editor with available files
        files = self.file_manager.get_all_files()
        self.routine_editor.set_available_files(files)
        
        # Find routine for this file
        routine = self.routine_manager.get_routine_by_filepath(filepath)
        if routine:
            self.current_routine_id = routine['id']
            # Simply load and display the routine (no modifications)
            self.routine_editor.load_routine(routine)
            self.status_bar.showMessage(f"Loaded routine for {Path(filepath).name}")
        else:
            # No routine exists yet - this shouldn't happen if files are added properly
            self.current_routine_id = None
            self.routine_editor.load_routine({
                'name': '',
                'wav_file': '',
                'markers': [],
                'light_config': create_default_light_config()
            })
    
    def _on_routine_changed(self, routine: dict):
        """Handle routine change."""
        if self.current_routine_id is not None:
            self.routine_manager.update_routine(
                self.current_routine_id,
                name=routine.get('name'),
                wav_file=routine.get('wav_file'),
                markers=routine.get('markers'),
                light_config=routine.get('light_config')
            )
            self.status_bar.showMessage("Routine updated")
    
    def _on_new_project(self):
        """Create a new project."""
        reply = QMessageBox.question(
            self, "New Project",
            "Create a new project? All unsaved changes will be lost.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        
        if reply == QMessageBox.StandardButton.Yes:
            self.routine_manager.clear_routines()
            self.file_manager.clear_files()
            self.current_routine_id = None
            self.status_bar.showMessage("New project created")
    
    def _on_open_project(self):
        """Open a project file."""
        file_path, _ = QFileDialog.getOpenFileName(
            self, "Open Project", "", "JSON Files (*.json);;All Files (*)"
        )
        
        if file_path:
            try:
                self.routine_manager.load_project(Path(file_path))
                self.status_bar.showMessage(f"Project loaded: {Path(file_path).name}")
                
                # Update file manager with files from routines
                # This will automatically create routines via _on_file_added
                routines = self.routine_manager.get_all_routines()
                for routine in routines:
                    wav_file = routine.get('wav_file', '')
                    if wav_file:
                        file_path_obj = Path(wav_file)
                        # Check if file exists before adding
                        if file_path_obj.exists():
                            # Add file (this will trigger _on_file_added which will check for existing routine)
                            self.file_manager.add_file(file_path_obj)
                        else:
                            # File doesn't exist, but routine exists - keep the routine
                            # Just update the editor if this is the first routine
                            if self.current_routine_id is None:
                                self.current_routine_id = routine['id']
                                files = self.file_manager.get_all_files()
                                self.routine_editor.set_available_files(files)
                                self.routine_editor.load_routine(routine)
                
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to load project:\n{str(e)}")
    
    def _on_save_project(self):
        """Save the current project."""
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Save Project", "", "JSON Files (*.json);;All Files (*)"
        )
        
        if file_path:
            try:
                self.routine_manager.save_project(Path(file_path))
                self.status_bar.showMessage(f"Project saved: {Path(file_path).name}")
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to save project:\n{str(e)}")
    
    def _on_export_code(self):
        """Export Arduino code."""
        routines = self.routine_manager.get_all_routines()
        if not routines:
            QMessageBox.warning(self, "Error", "No routines to export")
            return
        
        file_path, _ = QFileDialog.getSaveFileName(
            self, "Export Arduino Code", "", "Arduino Files (*.ino);;All Files (*)"
        )
        
        if file_path:
            try:
                success = self.code_generator.generate_code(
                    routines,
                    Path(file_path),
                    mp3_rx_pin=self.mp3_rx_pin,
                    mp3_tx_pin=self.mp3_tx_pin
                )
                if success:
                    QMessageBox.information(
                        self, "Success",
                        f"Arduino code exported to:\n{file_path}"
                    )
                    self.status_bar.showMessage("Code exported successfully")
                else:
                    QMessageBox.critical(self, "Error", "Failed to generate code")
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to export code:\n{str(e)}")
    
    def _on_toggle_console(self, checked: bool):
        """Toggle console dock visibility."""
        self.console_dock.setVisible(checked)
        self._update_console_menu_text(checked)
    
    def _on_console_visibility_changed(self, visible: bool):
        """Handle console dock visibility change (e.g., from close button)."""
        self.console_toggle_action.setChecked(visible)
        self._update_console_menu_text(visible)
    
    def _update_console_menu_text(self, visible: bool):
        """Update the console menu action text."""
        self.console_toggle_action.setText("Show Console" if not visible else "Hide Console")
    
    def _on_console_floating_changed(self, floating: bool):
        """Handle console dock floating state change."""
        self.console_widget.set_floating(floating)

