import urwid
import subprocess

palette = [
    ('focused', 'white', 'dark gray'),
    ('unfocused', 'light gray', 'black'),
    ('assistant_prompt', 'light green', 'black'),
    ('bash_prompt', 'light cyan', 'black'),
    ('focused_prompt', 'yellow', 'dark gray'),
]

class CustomWidget(urwid.WidgetWrap):

    def __init__(self, name, select_callback):
        self.name = name
        self.commands_history = []
        self.current_command = ""
        self.continuing = False

        self.user = urwid.Edit((f"{name.lower()}_prompt", self.name.lower() + "> "))
        self.output = urwid.Text("")
        self.listbox_content = [self.output, self.user]
        self.listbox = urwid.ListBox(urwid.SimpleFocusListWalker(self.listbox_content))
        self.box = urwid.AttrMap(urwid.LineBox(self.listbox, title=name.capitalize()), 'unfocused')

        self.frame = urwid.AttrMap(urwid.LineBox(self.listbox), 'unfocused')
        self.frame.keypress = select_callback

        super().__init__(self.frame)

    def execute_command(self):
        command = self.user.edit_text.strip()
        self.commands_history.append(command)
        self.user.set_edit_text("")  # Clear the input field after reading the command

        if command.endswith('\\'):
            self.current_command += command
            self.continuing = True
            self.user.set_edit_text(self.current_command)
        else:
            if self.continuing:
                command = self.current_command + command
                self.continuing = False
            self.current_command = ""
            try:
                result = subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT, text=True)
            except subprocess.CalledProcessError as e:
                result = e.output

            self.display_result(command, result)

    def display_result(self, command, result):
        self.output.set_text(self.output.text + "\n" + self.name.lower() + "> " + command + "\n" + result)


class MainFrame(urwid.Frame):
    def __init__(self):
        self.frames = [CustomWidget("assistant", self.selectable_click), CustomWidget("bash", self.selectable_click)]
        self.columns = urwid.Columns([])
        self.init_layout()
        super().__init__(self.columns)

    def init_layout(self):
        columns = []
        column_frames = []

        # Group frames into columns
        for i, frame in enumerate(self.frames):
            if i % 2 == 0:
                column_frames = [frame]
            else:
                column_frames.append(frame)
                columns.append(urwid.Pile([urwid.AttrMap(f.box, None, focus_map='focused') for f in column_frames]))
                column_frames = []

        # If there's an odd frame out, it gets its own full-height column
        if column_frames:
            columns.append(urwid.Pile([urwid.AttrMap(f.box, None, focus_map='focused') for f in column_frames]))

        # Set the columns layout
        self.columns.contents = [(col, self.columns.options('weight', 1)) for col in columns]
        self.update_focus()

    def add_frame(self, name):
        self.frames.append(CustomWidget(name, self.selectable_click))
        self.init_layout()

    def remove_frame(self, idx):
        self.frames.pop(idx)
        self.init_layout()

    def handle_input(self, key):
        if key in ('shift tab', 'ctrl shift tab'):
            self.columns.focus_position = (self.columns.focus_position + (1 if key == 'shift tab' else -1)) % len(self.columns.contents)
            self.update_focus()
        elif key == 'enter':
            col = self.columns.focus_position
            row = self.columns.contents[col][0].focus_position
            self.frames[col * 2 + row].execute_command()
        elif key == 'ctrl t':
            self.add_frame('bash')
        elif key == 'ctrl x':
            if len(self.frames) > 1:
                self.remove_frame(self.columns.focus_position * 2 + self.columns.contents[self.columns.focus_position][0].focus_position)
        else:
            return False

    def update_focus(self):
        for frame in self.frames:
            frame.box.set_attr_map({None: 'unfocused'})
            frame.user.set_caption((f"{frame.name.lower()}_prompt", frame.name.lower() + "> "))

        col = self.columns.focus_position
        row = self.columns.contents[col][0].focus_position
        focused_frame = self.frames[col * 2 + row]

        focused_frame.box.set_attr_map({None: 'focused'})
        focused_frame.user.set_caption(('focused_prompt', focused_frame.name.lower() + "> "))

    def selectable_click(self, widget, size, key):
        idx = 0
        for i in range(len(self.frames)):
            if self.frames[i].frame == widget:
                idx = i
        if key == 'mouse press':
            col = idx // 2
            row = idx % 2
            self.columns.focus_position = col
            self.columns.contents[col][0].focus_position = row
            self.update_focus()
        else:
            return False

def main():
    frame = MainFrame()
    loop = urwid.MainLoop(frame, palette, unhandled_input=frame.handle_input)
    loop.run()

if __name__ == "__main__":
    main()
