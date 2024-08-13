import urwid
import subprocess
import threading
import os
import sys

# urwid themes for frames
palette = [
    ('focused', 'white', 'dark gray'),
    ('unfocused', 'light gray', 'black'),
    ('assistant_prompt', 'light green', 'black'),
    ('bash_prompt', 'light cyan', 'black'),
    ('focused_prompt', 'yellow', 'dark gray'),
]

class CustomFrame(urwid.WidgetWrap):
    def __init__(self, name, select_callback, loop):
        self.name = name
        self.is_focused = False
        self.commands_history = []
        self.current_command = ""
        self.continuing = False
        self.loading = False
        self.loop = loop

        self.focus_text = urwid.Edit((f"{name.lower()}_prompt", self.name.lower() + "> "))
        self.output = urwid.Text("")
        self.text_content = [self.output, self.focus_text]
        self.text = urwid.ListBox(urwid.SimpleFocusListWalker(self.text_content))

        self.body = urwid.AttrMap(urwid.LineBox(self.text), 'unfocused')
        super().__init__(self.body)

        # Initialize subprocess attributes
        self.process = None
        self.stdout_thread = None

    def execute_command(self):
        # Executes the command entered by the user
        if self.loading:
            return
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
        if self.name.lower() == 'assistant':
            self._display_loading(command)
        else:
            self._run_interactive_command(command)

    def _run_interactive_command(self, command):
        # Start an interactive subprocess
        if self.process and self.process.poll() is None:
            self.process.stdin.write(command + "\n")
            self.process.stdin.flush()
        else:
            self.process = subprocess.Popen(
                command, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
            )
            self.stdout_thread = threading.Thread(target=self._read_output)
            self.stdout_thread.start()
            self._display_interactive_output(command, "Started interactive session")

    def _read_output(self):
        # Reads output from the subprocess
        for line in iter(self.process.stdout.readline, ''):
            if line:
                urwid.async_poll(self._update_output_text, line)

    def _update_output_text(self, line):
        # Updates the output text widget with new lines from the process
        self.output.set_text(self.output.text + "\n" + line)

    def _display_interactive_output(self, command, message):
        # Displays initial message for interactive programs
        self.output.set_text(self.output.text + f"\n{self.name.lower()}> {command}\n{message}")

    def _display_loading(self, command):
        # Displays loading before assistant replies
        self.focus_text.set_caption(('assistant_prompt', ''))
        self.loading = True
        self.loading_dots = 0
        self.output.set_text(self.output.text + f"\n{self.name.lower()}> {command}\nGenerating answer ")
        self._update_loading()

    def _update_loading(self, loop=None, user_data=None):
        # Loop for loading
        if self.loading_dots < 24:
            iter = self.loading_dots // 3
            dots = self.loading_dots % 3
            self.loading_dots += 1
            if iter != 0 and dots == 0:
                self.output.set_text(self.output.text[:len(self.output.text) - 3] + "." * (dots + 1))
            else:
                self.output.set_text(self.output.text[:len(self.output.text) - dots] + "." * (dots + 1))
            self.loop.set_alarm_in(0.5, self._update_loading)
        else:
            self.loading_dots = 0
            self.loop.set_alarm_in(0, self._display_generated_answer)

    def _display_generated_answer(self, loop=None, user_data=None):
        # Displays generated answer by assistant
        self.output.set_text(self.output.text + """\n
Lorem ipsum odor amet, consectetuer adipiscing elit. Lobortis parturient auctor ac urna sollicitudin consectetur. Nam nulla tempor habitant penatibus potenti mollis facilisis. Elit turpis vestibulum neque, efficitur aptent porttitor. Maecenas tempor volutpat purus maximus magna nisl volutpat aliquet erat. Nibh cubilia quisque non; torquent imperdiet magna aenean. Nisl proin a sit; sem ornare nascetur at dictum. Pellentesque lobortis ante sit viverra praesent eget scelerisque pellentesque tempor.

Molestie senectus ullamcorper felis proin integer. Finibus dictumst sem viverra diam vel mollis. Eget phasellus suscipit magnis amet eu lectus phasellus venenatis. Sem ipsum mattis condimentum fusce lacus accumsan. Praesent ultrices iaculis ut porttitor aenean. Condimentum sem rutrum felis proin tempus inceptos penatibus aliquet. Platea fames mus primis interdum scelerisque luctus laoreet placerat torquent?

Viverra volutpat arcu adipiscing malesuada rhoncus faucibus. Libero nunc orci metus id quis vel. Conubia finibus consequat netus netus primis, feugiat vehicula potenti. Mi pulvinar interdum convallis ad id mauris. Feugiat risus tortor auctor felis interdum eget id fringilla mattis. Morbi aenean lectus integer dolor a diam magnis. Cubilia id ultrices augue vestibulum; facilisis convallis.""")
        self.loading = False
        current_palette = f"{self.name}_prompt"
        if self.is_focused:
            current_palette = "focused_prompt"
        self.focus_text.set_caption((current_palette, self.name.lower() + "> "))

class MainFrame(urwid.Frame):
    def __init__(self, loop):
        self.loop = loop
        self.frames = [CustomFrame("assistant", self.selectable_click, self.loop),
                       CustomFrame("bash", self.selectable_click, self.loop)]
        self.columns = urwid.Columns([])
        self._init_layout()
        super().__init__(self.columns)

    def _init_layout(self):
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
        return urwid.Pile([urwid.AttrMap(f.body, None, focus_map='focused') for f in frames])

    def handle_input(self, key):
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
        self.frames.append(CustomFrame(name, self.selectable_click, self.loop))
        self._init_layout()

    def remove_frame(self):
        if len(self.frames) > 1:
            idx = self.columns.focus_position * 2 + self.columns.contents[self.columns.focus_position][0].focus_position
            self.frames.pop(idx)
            self._init_layout()

    def execute_command(self):
        col = self.columns.focus_position
        row = self.columns.contents[col][0].focus_position
        self.frames[col * 2 + row].execute_command()

    def _change_focus_frame(self, direction):
        current_col = self.columns.focus_position
        current_row = self.columns.contents[current_col][0].base_widget.focus_position
        new_col, new_row = self._calculate_new_focus_position(current_col, current_row, direction)

        self.columns.focus_position = new_col
        self.columns.contents[new_col][0].base_widget.focus_position = new_row

        return self.frames[new_col * 2 + new_row]

    def _calculate_new_focus_position(self, col, row, direction):
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
        for frame in self.frames:
            frame.body.set_attr_map({None: 'unfocused'})
            frame.is_focused = False
            if not frame.loading:
                frame.focus_text.set_caption((f"{frame.name.lower()}_prompt", frame.name.lower() + "> "))
        focus_frame.body.set_attr_map({None: 'focused'})
        if not focus_frame.loading:
            focus_frame.focus_text.set_caption(('focused_prompt', focus_frame.name.lower() + "> "))
            focus_frame.is_focused = True

    def selectable_click(self, frame, key):
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
    loop = urwid.MainLoop(None, palette)
    frame = MainFrame(loop)
    loop.widget = frame
    loop.unhandled_input = frame.handle_input
    loop.run()

if __name__ == "__main__":
    main()
