import signal
import socket
import time

import redis

r = redis.Redis(host="broker", decode_responses=True)
me = socket.gethostname()

running = True

def stop(signum, frame):
    global running
    running = False
    print(f"{me}: получен сигнал {signum}, завершаю после текущей задачи")

signal.signal(signal.SIGTERM, stop)
signal.signal(signal.SIGINT, stop)

while running:
    item = r.brpop("tasks", timeout=1)
    if item is None:
        continue
    _, n = item
    time.sleep(1)
    r.lpush("results", f"{n}^2 = {int(n) ** 2}  (выполнил {me})")

print(f"{me}: остановлен")
