import urwid
import subprocess

# urwid themes for frames
palette = [
    ('focused', 'white', 'dark gray'),
    ('unfocused', 'light gray', 'black'),
    ('assistant_prompt', 'light green', 'black'),
    ('bash_prompt', 'light cyan', 'black'),
    ('focused_prompt', 'yellow', 'dark gray'),
]

class CustomFrame(urwid.WidgetWrap):
    def __init__(self, name, select_callback):
        # Initializes the frame with its name and a callback for selection events
        self.name = name
        self.commands_history = []
        self.current_command = ""
        self.continuing = False

        self.focus_text = urwid.Edit((f"{name.lower()}_prompt", self.name.lower() + "> "))
        self.output = urwid.Text("")
        self.text_content = [self.output, self.focus_text]
        self.text = urwid.ListBox(urwid.SimpleFocusListWalker(self.text_content))

        self.body = urwid.AttrMap(urwid.LineBox(self.text), 'unfocused')
        # self.body.keypress = select_callback

        super().__init__(self.body)

    def execute_command(self):
        # Executes the command entered by the user
        command = self._read_command()
        if self._is_multiline_command(command):
            self._handle_multiline_command(command)
        else:
            self._handle_singleline_command(command)

    def _read_command(self):
        # Reads and clears the command from the focus text widget
        command = self.focus_text.edit_text.strip()
        self.focus_text.set_edit_text("")
        return command

    def _is_multiline_command(self, command):
        return command.endswith('\\')

    def _handle_multiline_command(self, command):
        # Handles multiline commands by appending and setting the text for continuation
        self.current_command += command
        self.continuing = True
        self.focus_text.set_edit_text(self.current_command + "\n")

    def _handle_singleline_command(self, command):
        # Handles single-line commands by executing and displaying the result
        if self.continuing:
            command = self.current_command + command
            self.continuing = False
        self.commands_history.append(command)
        self.current_command = ""
        result = self._run_command(command)
        self._display_result(command, result)

    def _run_command(self, command):
        # Executes the given command and returns the result
        try:
            return subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT, text=True)
        except subprocess.CalledProcessError as e:
            return e.output

    def _display_result(self, command, result):
        # Displays the result of the executed command in the output widget
        result_text = f"{self.name.lower()}> {command}\n{result}"
        self.output.set_text(self.output.text + "\n" + result_text)


class MainFrame(urwid.Frame):
    def __init__(self):
        # Initializes "assistant" and "bash" frames at the beginning and then the whole layout around those two frames
        self.frames = [CustomFrame("assistant", self.selectable_click), CustomFrame("bash", self.selectable_click)]
        self.columns = urwid.Columns([])
        self._init_layout()
        super().__init__(self.columns)

    def _init_layout(self):
        # Initializes layout in columns, with max two rows in one column
        columns = []
        column_frames = []
        for i, frame in enumerate(self.frames):
            if i % 2 == 0:
                column_frames = [frame]
            else:
                column_frames.append(frame)
                columns.append(self._create_column(column_frames))
                column_frames = []
        if column_frames:
            columns.append(self._create_column(column_frames))

        self.columns.contents = [(col, self.columns.options('weight', 1)) for col in columns]
        self.update_focus(self.frames[0])

    def _create_column(self, frames):
        # Creates a column with a list of frames
        return urwid.Pile([urwid.AttrMap(f.body, None, focus_map='focused') for f in frames])

    def handle_input(self, key):
        # Handles keyboard input for different shortcuts
        if key in ('shift tab', 'ctrl shift tab'):
            self.update_focus(self._change_focus_frame(1 if key == 'shift tab' else -1))
        elif key == 'enter':
            self.execute_command()
        elif key == 'ctrl t':
            self.add_frame('bash')
        elif key == 'ctrl x':
            self.remove_frame()
        else:
            return False

    def add_frame(self, name):
        # Adds a new frame to the layout and updates the layout
        self.frames.append(CustomFrame(name, self.selectable_click))
        self._init_layout()

    def remove_frame(self):
        # Removes the frame at the currently focused position
        if len(self.frames) > 1:
            idx = self.columns.focus_position * 2 + self.columns.contents[self.columns.focus_position][0].focus_position
            self.frames.pop(idx)
            self._init_layout()

    def execute_command(self):
        # Executes the command at the currently focused frame
        col = self.columns.focus_position
        row = self.columns.contents[col][0].focus_position
        self.frames[col * 2 + row].execute_command()

    def _change_focus_frame(self, direction):
            # Change the focus frame in the specified direction
            current_col = self.columns.focus_position
            current_row = self.columns.contents[current_col][0].base_widget.focus_position
            new_col, new_row = self._calculate_new_focus_position(current_col, current_row, direction)

            self.columns.focus_position = new_col
            self.columns.contents[new_col][0].base_widget.focus_position = new_row

            return self.frames[new_col * 2 + new_row]

    def _calculate_new_focus_position(self, col, row, direction):
        # Calculates new focus position based on direction
        new_row = row + direction
        new_col = col
        if new_row >= len(self.columns.contents[col][0].base_widget.contents):
            new_row = 0
            new_col = (col + 1) % len(self.columns.contents)
        elif new_row < 0:
            new_col = (col - 1) % len(self.columns.contents)
            new_row = len(self.columns.contents[new_col][0].base_widget.contents) - 1
        return new_col, new_row

    def update_focus(self, focus_frame):
        # Update the focus attributes for all frames and set the focused frame
        for frame in self.frames:
            frame.body.set_attr_map({None: 'unfocused'})
            frame.focus_text.set_caption((f"{frame.name.lower()}_prompt", frame.name.lower() + "> "))
        focus_frame.body.set_attr_map({None: 'focused'})
        focus_frame.focus_text.set_caption(('focused_prompt', focus_frame.name.lower() + "> "))

    def selectable_click(self, frame, key):
        # Callback function for mouse click on frame
        if key == 'mouse press':
            for i, frame in enumerate(self.frames):
                if frame.body == frame:
                    col = i // 2
                    row = i % 2
                    self.columns.focus_position = col
                    self.columns.contents[col][0].base_widget.focus_position = row
                    self.update_focus(self.frames[i])
                    break
        else:
            return False


def main():
    frame = MainFrame()
    loop = urwid.MainLoop(frame, palette, unhandled_input=frame.handle_input)
    loop.run()


if __name__ == "__main__":
    main()
