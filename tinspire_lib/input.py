import ti_system as ti

class Keys:
    UP = "up"
    DOWN = "down"
    LEFT = "left"
    RIGHT = "right"
    ENTER = "enter"
    ESC = "esc"
    TAB = "tab"
    DEL = "del"
    CLEAR = "clear"
    SPACE = "space"

class Input:
    @staticmethod
    def get_key():
        """Returns the current key being pressed."""
        return ti.get_key()
    
    @staticmethod
    def is_key_pressed(key):
        """Check if a specific key is pressed."""
        return ti.get_key() == key
