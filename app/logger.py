import logging
import os

class Logger:
    _instance = None

    @staticmethod
    def get_instance():
        if not Logger._instance:
            branch = branch = os.getenv("BRANCH", "NONE").upper()
            Logger._instance = Logger(branch)
        return Logger._instance.logger

    def __init__(self, branch):
        if not Logger._instance:
            self.logger = logging.getLogger("template_singe-service")
            filename = '/data/service.log'
            file_handler = logging.FileHandler(filename, 'a')
            formatter = logging.Formatter('\033[31m.svc.\033[0m%(asctime)s:%(filename)s:%(lineno)d:%(levelname)s: %(message)s')

            file_handler.setFormatter(formatter)
            self.logger.addHandler(file_handler)

            if branch == "PROD":
                self.logger.setLevel(logging.INFO)
            else:
                self.logger.setLevel(logging.DEBUG)

log = Logger.get_instance()
