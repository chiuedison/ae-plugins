# This file is currently no longer being used for testing because we're using socket.io

import threading
import time

import zmq


class Sender:
    def __init__(self, ctx_: zmq.Context):
        self.ctx = ctx_
        self.socket = self.ctx.socket(zmq.PAIR)
        self.socket.connect("tcp://127.0.0.1:5556")

    def send(self):
        while True:
            print("Sending...")
            self.socket.send('pong'.encode())
            time.sleep(1)


class Listener:
    def __init__(self, ctx_: zmq.Context):
        self.ctx = ctx_
        self.socket = self.ctx.socket(zmq.PAIR)
        self.socket.connect("tcp://127.0.0.1:5555")

    def listen(self):
        while True:
            try:
                message: bytes = self.socket.recv()
                decoded_message = message.decode('utf-8')
                print(f"Receiving: {decoded_message}")
            except Exception as e:
                print("*dies*")
                print(e)


if __name__ == "__main__":
    ctx = zmq.Context()

    listener = Listener(ctx)
    sender = Sender(ctx)

    threading.Thread(target=listener.listen).start()
    threading.Thread(target=sender.send).start()
