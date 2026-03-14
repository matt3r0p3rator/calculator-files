class App:
    def __init__(self, width=318, height=212):
        self.width = width
        self.height = height
        self.running = False
        self.current_scene = None

    def set_scene(self, scene):
        self.current_scene = scene
        if self.current_scene:
            self.current_scene.app = self
            self.current_scene.setup()

    def run(self):
        import ti_draw as dr
        dr.clear()
        dr.set_window(0, self.width, self.height, 0)
        
        self.running = True
        
        while self.running:
            if self.current_scene:
                self.current_scene.handle_input()
                self.current_scene.update()
                
                dr.clear()
                self.current_scene.draw()
            else:
                self.running = False


class Scene:
    def __init__(self):
        self.app = None

    def setup(self):
        pass

    def handle_input(self):
        pass

    def update(self):
        pass

    def draw(self):
        pass
