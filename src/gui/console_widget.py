"""
Console widget for displaying stdout and stderr output.
"""

import sys
from PyQt6.QtWidgets import QWidget, QVBoxLayout, QPlainTextEdit, QPushButton, QHBoxLayout, QSizePolicy
from PyQt6.QtCore import QObject, pyqtSignal, Qt
from PyQt6.QtGui import QTextCharFormat, QColor, QFont


class ConsoleStream(QObject):
    """Custom stream that redirects stdout/stderr to console widget."""
    
    text_written = pyqtSignal(str, str)  # text, stream_type ('stdout' or 'stderr')
    
    def __init__(self, stream_type: str, parent=None):
        super().__init__(parent)
        self.stream_type = stream_type
        self.original_stream = sys.stdout if stream_type == 'stdout' else sys.stderr
    
    def write(self, text: str):
        """Write text to the console widget."""
        if text:  # Only emit non-empty text
            self.text_written.emit(text, self.stream_type)
        self.original_stream.write(text)  # Also write to original stream
    
    def flush(self):
        """Flush the stream."""
        self.original_stream.flush()
    
    def __getattr__(self, name):
        """Forward other attributes to original stream."""
        return getattr(self.original_stream, name)


class ConsoleWidget(QWidget):
    """Widget that displays console output from stdout and stderr."""
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self.init_ui()
        self.stdout_stream = None
        self.stderr_stream = None
    
    def init_ui(self):
        """Initialize the UI."""
        layout = QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        
        # Create button bar (at the top)
        button_layout = QHBoxLayout()
        button_layout.setContentsMargins(5, 5, 5, 5)
        
        self.clear_button = QPushButton("Clear")
        self.clear_button.clicked.connect(self.clear)
        button_layout.addWidget(self.clear_button)
        
        button_layout.addStretch()
        layout.addLayout(button_layout)
        
        # Create text edit for console output
        self.text_edit = QPlainTextEdit()
        self.text_edit.setReadOnly(True)
        self.text_edit.setFont(QFont("Consolas", 9) if sys.platform == "win32" else QFont("Monaco", 9))
        self.text_edit.setStyleSheet("""
            QPlainTextEdit {
                background-color: #1e1e1e;
                color: #d4d4d4;
                border: 1px solid #3e3e3e;
            }
        """)
        layout.addWidget(self.text_edit)
        
        # Set size policy to allow expansion
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        self.text_edit.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)
        
        # Set a minimum height to ensure the console doesn't get too small
        # When docked, it will expand with the main window vertically
        self.min_height = 100
        self.setMinimumHeight(self.min_height)
        
        self.setLayout(layout)
        
        # Set up text formats for different stream types
        self.stdout_format = QTextCharFormat()
        self.stdout_format.setForeground(QColor("#d4d4d4"))  # Light gray
        
        self.stderr_format = QTextCharFormat()
        self.stderr_format.setForeground(QColor("#f48771"))  # Light red/orange
        
        # Connect stream signals
        self.stdout_stream = ConsoleStream('stdout', self)
        self.stdout_stream.text_written.connect(self.append_text)
        
        self.stderr_stream = ConsoleStream('stderr', self)
        self.stderr_stream.text_written.connect(self.append_text)
        
        # Install stream redirection immediately so print statements are captured
        self.install_streams()

        # Print a message to the user that the console is ready
        print("Console is ready")
    
    def append_text(self, text: str, stream_type: str):
        """Append text to the console with appropriate formatting."""
        cursor = self.text_edit.textCursor()
        cursor.movePosition(cursor.MoveOperation.End)
        
        # Select format based on stream type
        if stream_type == 'stderr':
            cursor.setCharFormat(self.stderr_format)
        else:
            cursor.setCharFormat(self.stdout_format)
        
        cursor.insertText(text)
        
        # Auto-scroll to bottom
        self.text_edit.ensureCursorVisible()
    
    def clear(self):
        """Clear the console output."""
        self.text_edit.clear()
    
    def install_streams(self):
        """Install custom streams for stdout and stderr."""
        sys.stdout = self.stdout_stream
        sys.stderr = self.stderr_stream
    
    def restore_streams(self):
        """Restore original stdout and stderr."""
        if self.stdout_stream:
            sys.stdout = self.stdout_stream.original_stream
        if self.stderr_stream:
            sys.stderr = self.stderr_stream.original_stream
    
    def set_floating(self, floating: bool):
        """Adjust widget constraints based on whether the dock is floating."""
        # Both floating and docked states allow expansion
        # The minimum height ensures it doesn't get too small
        # No maximum height constraint - let it expand naturally
        pass

