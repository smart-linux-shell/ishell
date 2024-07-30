import urwid
import subprocess

# Define colors
palette = [
    ('focused', 'black', 'light gray'),
    ('unfocused', 'light gray', 'black')
]

class CustomWidget(urwid.Edit):

    def __init__(self, name, select_callback):
        super().__init__()

        self.name = name
        self.commands_history = []
        self.current_command = ""
        self.continuing = False

        self.user = urwid.Edit(self.name.lower() + "> ")
        self.frame = urwid.AttrMap(urwid.LineBox(self.user), 'unfocused')
        self.box = urwid.AttrMap(urwid.LineBox(self.user, title=name.capitalize()), 'unfocused')

        self.selectable_icon = urwid.SelectableIcon('', 0)
        self.frame.keypress = select_callback

    def execute_command(self):
        command = self.user.edit_text.strip()

        if command.endswith('\\'):
            self.current_command += command
            self.continuing = True
            self.user.set_edit_text(self.current_command + "\n")
        else:
            if self.continuing:
                command = self.current_command + command
                self.continuing = False
            self.current_command = ""
            try:
                result = subprocess.check_output(command, shell=True, stderr=subprocess.STDOUT, text=True)
            except subprocess.CalledProcessError as e:
                result = e.output

            self.display_result(result)

    def display_result(self, result):
        self.user.set_edit_text(self.user.edit_text + "\n" + result + "\n" + self.user.edit_text[:self.user.edit_pos])


class MainFrame(urwid.Frame):
    def __init__(self):
        self.frames = [CustomWidget("assistant", self.selectable_click), CustomWidget("bash", self.selectable_click)]
        self.pile = urwid.Pile([])
        self.init_layout()
        super().__init__(self.pile)

    def init_layout(self):
        self.pile.contents = [(frame.box, self.pile.options('weight', 1)) for frame in self.frames]
        self.update_focus()

    def add_frame(self, name):
        self.frames.append(CustomWidget(name, self.selectable_click))
        self.init_layout()

    def remove_frame(self, idx):
        self.frames.pop(idx)
        self.init_layout()

    def handle_input(self, key):
        if key in ('shift tab', 'ctrl shift tab'):
            self.pile.focus_position = (self.pile.focus_position + (1 if key == 'shift tab' else -1)) % len(self.frames)
            self.update_focus()
        elif key == 'enter':
            self.frames[self.pile.focus_position].execute_command()
        elif key == 'ctrl t':
            self.add_frame('bash')
        elif key == 'ctrl x':
            if len(self.frames) > 1:
                self.remove_frame(self.pile.focus_position)
        else:
            return False

    def update_focus(self):
        for frame in self.frames:
            frame.frame.set_attr_map({None: 'unfocused'})
        self.frames[self.pile.focus_position].frame.set_attr_map({None: 'focused'})

    def selectable_click(self, widget, size, key):
        idx = 0
        for i in range(len(self.frames)):
            if self.frames[i].frame == widget:
                idx = i
        if key == 'mouse press':
            self.pile.focus_position = idx
            self.update_focus()
        else:
            return False

def main():
    frame = MainFrame()
    loop = urwid.MainLoop(frame, palette, unhandled_input=frame.handle_input)
    loop.run()

if __name__ == "__main__":
    main()
