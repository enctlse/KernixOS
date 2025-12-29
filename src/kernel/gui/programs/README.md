
This directory contains GUI applications for the DystopiaOS graphical interface.


- Command: `gui`
- Description: GUI terminal with text input/output
- Status: ✅ Implemented

- Command: `calc`
- Description: Graphical calculator application
- Status: 🚧 Placeholder (TODO)

- Command: `edit [filename]`
- Description: GUI text editor for file editing
- Status: 🚧 Placeholder (TODO)


1. Create new `.c` file in this directory
2. Implement `FHDR(cmd_name)` function
3. Add function declaration to `programs.h`
4. The build system will automatically include the new file


Each GUI program should:
- Include `<kernel/gui/gui.h>` for GUI functions
- Use `FHDR(cmd_name)` macro for command registration
- Handle its own window management and rendering
- Integrate with the main GUI event loop


- File Manager
- Image Viewer
- System Monitor
- Settings Panel
- Web Browser (when networking is implemented)










